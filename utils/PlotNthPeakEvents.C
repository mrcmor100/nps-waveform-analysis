#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TSystem.h>
#include <TString.h>
#include <TStyle.h>

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

constexpr int NumSamples = 110;

struct Peak {
    float amplitude = 0;
    float time = 0;
    bool good = false;
};

struct PeakContainer {
    static constexpr int maxPeaks = 12;
    int block = 0;
    int nPeaks = 0;
    int peakOverflow = 0;
    std::array<Peak, maxPeaks> peaks{};
};

template <typename T, size_t N>
struct DataBlock {
    T block_id;
    T n_samples;
    T data[N];

    DataBlock() : block_id(0), n_samples(0) {
        for (size_t i = 0; i < N; ++i)
            data[i] = T();
    }
};

using DataBlockFloat = DataBlock<float, NumSamples>;

// Plot one block+peak combo
void PlotWaveformWithPeaks(
    const DataBlockFloat &db,
    const PeakContainer &pc,
    const std::string &output_path)
{
    auto *gr = new TGraph();            // Full waveform
    auto *gr_all_peaks = new TGraph();  // Red peaks (not good)
    auto *gr_good_peaks = new TGraph(); // Green peaks

    for (int i = 0; i < NumSamples; i++) {
        gr->AddPoint(i, db.data[i]);
    }

    for (int i = 0; i < pc.nPeaks && i < PeakContainer::maxPeaks; i++) {
        const auto &peak = pc.peaks[i];
        if (peak.good) {
            gr_good_peaks->AddPoint(peak.time, peak.amplitude);
        } else {
            gr_all_peaks->AddPoint(peak.time, peak.amplitude);
        }
    }

    // Style settings
    gr->SetLineColor(kBlue);
    gr->SetMarkerColor(kBlue);
    gr->SetMarkerStyle(5); // "x"
    gr->SetMarkerSize(0.7);
    gr->SetLineWidth(1);

    gr_all_peaks->SetMarkerStyle(20);
    gr_all_peaks->SetMarkerSize(1.5);
    gr_all_peaks->SetMarkerColor(kRed);

    gr_good_peaks->SetMarkerStyle(20);
    gr_good_peaks->SetMarkerSize(1.5);
    gr_good_peaks->SetMarkerColor(kGreen+2);

    // Draw
    auto *c = new TCanvas("c", "Waveform with Peaks", 800, 600);
    gr->Draw("ALP");
    gr_all_peaks->Draw("P SAME");
    gr_good_peaks->Draw("P SAME");

    auto *legend = new TLegend(0.7, 0.75, 0.9, 0.9);
    legend->AddEntry(gr, "Waveform", "lp");
    legend->AddEntry(gr_good_peaks, "Good Peaks", "p");
    legend->AddEntry(gr_all_peaks, "Other Peaks", "p");
    legend->Draw();

    c->SaveAs(output_path.c_str());

    delete c;
    delete gr;
    delete gr_all_peaks;
    delete gr_good_peaks;
}

int main(int argc, char **argv)
{
    gSystem->Load("../libDataBlockDict.so");
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " input.root N_stride M_nPeaks\n";
        return 1;
    }

    const std::string input_file = argv[1];
    const int stride = std::stoi(argv[2]);   // Output every Nth match
    const int target_nPeaks = std::stoi(argv[3]); // Match only blocks with this many peaks

    const std::string out_dir = "output_plots";
    fs::create_directory(out_dir); // Creates if not exists

    TFile *file = TFile::Open(input_file.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Failed to open file: " << input_file << "\n";
        return 1;
    }

    TTree *tree = dynamic_cast<TTree *>(file->Get("TOUT"));
    if (!tree) {
        std::cerr << "Couldn't find tree named 'TOUT'\n";
        return 1;
    }

    std::vector<DataBlockFloat> *entry_blocks = nullptr;
    std::vector<PeakContainer> *peak_container = nullptr;

    tree->SetBranchAddress("float_blocks", &entry_blocks);
    tree->SetBranchAddress("block_peaks", &peak_container);

    const int nEntries = tree->GetEntries();
    int match_count = 0;
    int plot_count = 0;

    for (int entry = 0; entry < nEntries; ++entry) {
        tree->GetEntry(entry);

        for (size_t i = 0; i < entry_blocks->size(); ++i) {
            const auto &db = entry_blocks->at(i);
            const auto &pc = peak_container->at(i);

            if (pc.nPeaks == target_nPeaks) {
                if (match_count % stride == 0) {
                    std::string fname = out_dir + "/waveform_plot_" + std::to_string(plot_count) + ".png";
                    std::cout << "Plotting entry " << entry
                              << ", block_id = " << db.block_id
                              << " → " << fname << std::endl;

                    PlotWaveformWithPeaks(db, pc, fname);
                    ++plot_count;
                }
                ++match_count;
            }
        }
    }

    std::cout << "Plotted " << plot_count << " waveforms out of " << match_count
              << " matches with nPeaks == " << target_nPeaks << "\n";

    return 0;
}
