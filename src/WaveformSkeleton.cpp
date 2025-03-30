#include <iostream>
#include <cmath>
#include <TCanvas.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TH1F.h>
#include <TLegend.h>
#include <TFile.h>
#include <TTree.h>
#include "TSystem.h"
#include <Math/Minimizer.h>
#include <Math/Factory.h>
#include <Math/Functor.h>
#include <Math/Interpolator.h>

#include "ReferenceManager.hpp"
#include "ConfigManager.hpp"
#include "DataTypes.hpp"

// ---- Thread-local Fit Context ----
struct FitContext {
    const ROOT::Math::Interpolator* interpolator = nullptr;
    int nPeaks = 0;
    double xmin = 1.0;
    double xmax = 109.0;
};

thread_local FitContext fitContext;

// ---- Raw Fit Function ----
double FitFuncRaw(double* x, double* params) {
    const double sampleTime = x[0];
    const double constantOffset = params[0];
    double totalSignal = 0.0;

    const int nPeaks = fitContext.nPeaks;
    const double xmin = fitContext.xmin;
    const double xmax = fitContext.xmax;
    const ROOT::Math::Interpolator* interpolator = fitContext.interpolator;

    for (int peakIndex = 0; peakIndex < nPeaks; ++peakIndex) {
        const int timeParamIndex = 1 + 2 * peakIndex;
        const int amplitudeParamIndex = 2 + 2 * peakIndex;

        const double peakTime = params[timeParamIndex];
        const double peakAmplitude = params[amplitudeParamIndex];

        const double shiftedTime = sampleTime - peakTime;
        const double inDomain = (shiftedTime > xmin && shiftedTime < xmax);
        const double safeTime = std::clamp(shiftedTime, xmin, xmax);

        totalSignal += inDomain * peakAmplitude * interpolator->Eval(safeTime);
    }

    return totalSignal + constantOffset;
}

int main(int argc, char** argv) {
    gSystem->Load("libDataBlockDict.so");
    if (argc < 3) {
        std::cerr << "Usage: ./waveform_fit path/to/file.root <run> <event>\n";
        return 1;
    }
    int event = std::stoi(argv[3]);
    int run = std::stoi(argv[2]);
    std::string configFile = "config/config.json";
    ConfigManager config(configFile, run);
    const ReferenceConfig& refCfg = config.GetReferenceConfig();
    ReferenceManager refManager(refCfg);
    refManager.ApplyConfig(run);

    // ---- Open ROOT File ----
    TFile* file = TFile::Open(argv[1]);
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening file: " << argv[1] << "\n";
        return 1;
    }

    TTree* tree = dynamic_cast<TTree*>(file->Get("TOUT"));
    if (!tree) {
        std::cerr << "Could not find TTree 'TOUT'\n";
        return 1;
    }

    // ---- Declare pointers to vectors ----
    std::vector<DataBlockFloat>* dataVec = nullptr;
    std::vector<BlockFitParameters>* fitParamsVec = nullptr;

    tree->SetBranchAddress("float_blocks", &dataVec);
    tree->SetBranchAddress("fit_params_blocks", &fitParamsVec);

    // ---- Load first entry ----
    tree->GetEntry(event);

    if (fitParamsVec->empty() || dataVec->empty()) {
        std::cerr << "Empty data or parameters in entry\n";
        return -1;
    }
    
    const auto& fitParams = fitParamsVec->at(0);
    
    // Check for valid peaks
    bool foundPeak = false;
    for (int i = 0; i < fitParams.nPeaks; ++i) {
        if (fitParams.peak_parameters[i].amplitude > 2.0) {
            foundPeak = true;
            break;
        }
    }
    
    if (!foundPeak) return -1;
    
    const auto& dataBlock = dataVec->at(0);

    // ---- Create interpolator from waveform ----
    std::vector<double> x_vals, y_vals;
    for (int i = 0; i < NumSamples; ++i) {
        x_vals.push_back(i);
        y_vals.push_back(static_cast<double>(dataBlock.data[i]));
    }

    std::cout << "Getting Interpolation for block: " << dataBlock.block_id << '\n';
    auto* interp = refManager.GetInterpolator(dataBlock.block_id);
    //ROOT::Math::Interpolator interp(x_vals, y_vals, ROOT::Math::Interpolation::kCSPLINE);

    // ---- Fit context ----
    fitContext.interpolator = interp;
    fitContext.nPeaks = fitParams.nPeaks;
    fitContext.xmin = 1.0;
    fitContext.xmax = 109.0;

    const int nParams = 1 + 2 * fitParams.nPeaks;

    // ---- Build Functor for Minimizer ----
    ROOT::Math::Functor f(
        [&](const double* p) {
            double chi2 = 0.0;
            for (int i = 0; i < NumSamples; ++i) {
                double x = static_cast<double>(i);
                double fx = FitFuncRaw(const_cast<double*>(&x), const_cast<double*>(p));
                double residual = dataBlock.data[i] - fx;
                double weight = 1.0 / (dataBlock.errors[i] * dataBlock.errors[i]);
                chi2 += residual * residual * weight;
            }
            return chi2;
        },
        nParams
    );

    auto minimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");
    minimizer->SetFunction(f);
    minimizer->SetMaxFunctionCalls(500*fitParams.nPeaks);
    minimizer->SetMaxIterations(500);
    minimizer->SetTolerance(1e-3);

    // ---- Set parameters ----
    minimizer->SetLimitedVariable(0, "pedestal",
                                  fitParams.block_pedestal.pedestal,
                                  0.01,
                                  fitParams.block_pedestal.ped_lower_limit,
                                  fitParams.block_pedestal.ped_upper_limit);
    std::vector<double> original_parameters;
    original_parameters.push_back(fitParams.block_pedestal.pedestal);
    for (int i = 0; i < fitParams.nPeaks; ++i) {
        const auto& peak = fitParams.peak_parameters[i];
        int tIdx = 1 + 2 * i;
        int aIdx = 2 + 2 * i;
        original_parameters.push_back(peak.time);
        original_parameters.push_back(peak.amplitude);
        printf("nPeak: %d Time: %f,%f,%f\n",i,peak.time_lower_limit,peak.time,peak.time_upper_limit);
        minimizer->SetLimitedVariable(tIdx, "time", peak.time, 0.01,
                                      peak.time_lower_limit, peak.time_upper_limit);
        printf("nPeak: %d Amp: %f,%f,%f\n",i,peak.amplitude_lower_limit,peak.amplitude,peak.amplitude_upper_limit);
        minimizer->SetLimitedVariable(aIdx, "amplitude", peak.amplitude, 0.01,
                                      peak.amplitude_lower_limit, peak.amplitude_upper_limit);
    }


    minimizer->Minimize();

    std::cout << "Fit Results:\n";
    for (int i = 0; i < nParams; ++i) {
        std::cout << "  Param " << i << " = " << minimizer->X()[i] << "\n";
    }

    file->Close();

    // --- 1. Create a histogram from the data ---
    auto h_data = new TH1F("h_data", Form("Block: %4.f, Event: %d, Waveform: 0;Sample;Amplitude",dataBlock.block_id, event), NumSamples, 0, NumSamples);
    auto g_fit = new TGraph(NumSamples);

    for (int i = 0; i < NumSamples; ++i) {
        h_data->SetBinContent(i + 1, dataBlock.data[i]);
        //h_data->SetBinError(i + 1, dataBlock.errors[i]);

        double x = static_cast<double>(i);
        double y_fit = FitFuncRaw(&x, const_cast<double*>(minimizer->X()));
        g_fit->SetPoint(i, x, y_fit);
    }

    // --- 2. Style the histogram and graph ---
    h_data->SetMarkerStyle(20);
    h_data->SetMarkerColor(kBlack);
    h_data->SetLineColor(kBlack);

    g_fit->SetLineColor(kRed);
    g_fit->SetLineWidth(2);

    // --- 3. Create canvas and draw ---
    auto c = new TCanvas("c","", 800, 600);
    h_data->Draw("E");        // 'E' = draw with error bars
    //g_fit->Draw("L SAME");    // 'L' = line; 'SAME' = overlay

    auto legend = new TLegend(0.6, 0.75, 0.88, 0.88);
    legend->AddEntry(h_data, "Data", "lep");
    //legend->AddEntry(g_fit, "Fit", "l");
    //legend->Draw();

    TGraph* g_orignial = new TGraph(NumSamples);
    for (int i = 0; i < NumSamples; ++i) {
        double x = static_cast<double>(i);
        double y_fit = FitFuncRaw(&x, const_cast<double*>(original_parameters.data()));
        g_orignial->SetPoint(i, x, y_fit);
    }
    //g_orignial->Draw("SAME");

    // --- 4. Save or display ---
    //c->SaveAs("fit_result.png"); // or .pdf or .root


    // ---- 3. Create a TF1-compatible wrapper for FitFuncRaw ----
    // auto tf1_wrapper = [](double* x, double* p) {
    //     return FitFuncRaw(x, p);
    // };

    auto tf1_wrapper = [](double* x, double* par) -> double {
        double val = 0.0;
        const int nPeaks = fitContext.nPeaks;
        const double xmin = fitContext.xmin;
        const double xmax = fitContext.xmax;
        const auto* interp = fitContext.interpolator;
    
        for (int p = 0; p < nPeaks; ++p) {
            double t = par[1 + 2 * p];   // time param
            double a = par[2 + 2 * p];   // amplitude param
            double shifted = x[0] - t;
            if (shifted > xmin && shifted < xmax) {
                val += a * interp->Eval(shifted);
            }
        }
    
        return val + par[0]; // pedestal at par[0]
    };

    // ---- 4. Create TF1 object with dynamic parameters ----
    TF1* f_tf1 = new TF1("f_tf1", tf1_wrapper, 0.0, NumSamples, nParams);
    f_tf1->SetNpx(1000);  // more points for smoother drawing

    // ---- 5. Set initial parameters (same as original Minuit2 setup) ----
    f_tf1->SetParameter(0, fitParams.block_pedestal.pedestal);
    for (int i = 0; i < fitParams.nPeaks; ++i) {
        const auto& peak = fitParams.peak_parameters[i];
        int tIdx = 1 + 2 * i;
        int aIdx = 2 + 2 * i;

        f_tf1->SetParameter(tIdx, peak.time);
        f_tf1->SetParameter(aIdx, peak.amplitude);
        f_tf1->SetParLimits(tIdx,peak.time_lower_limit, peak.time_upper_limit);
        f_tf1->SetParLimits(aIdx,peak.amplitude_lower_limit, peak.amplitude_upper_limit);

    }

    // ---- 6. Set FitContext again (thread_local, ensure correct context) ----
    fitContext.interpolator = interp;
    fitContext.nPeaks = fitParams.nPeaks;
    fitContext.xmin = 1.0;
    fitContext.xmax = 109.0;

    // ---- 7. Fit the histogram using ROOT's Fit ----
    h_data->Fit(f_tf1, "R0");  // "R" = fit in range, "0" = quiet mode

    // ---- 8. Overlay TF1 Fit ----
    f_tf1->SetLineColor(kBlue);
    f_tf1->SetLineStyle(2);
    f_tf1->Draw("SAME");

    // ---- 9. Update legend ----
    legend->AddEntry(f_tf1, "TF1 Fit", "l");
    legend->Draw();

    // --- 4. Save or display ---
    c->SaveAs(Form("./fits2/fit_result_4458_%d_%d.png",event,static_cast<int>(dataBlock.block_id))); // or .pdf or .root

    return 0;
}
