#!/usr/bin/env python3
import argparse
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import UnivariateSpline
from matplotlib.backends.backend_pdf import PdfPages

def parse_spec_line(line):
    """
    Parses a line of the format:
    block_number, timeRef, tdc_offset, v1, v2, ..., vN
    Returns block_number (int), timeRef (float), tdc_offset (float), values (list of floats)
    """
    parts = line.strip().split(',')
    if len(parts) < 4:
        return None  # Not a data line
    block_number = int(parts[0].strip())
    time_ref = float(parts[1].strip())
    tdc_offset = float(parts[2].strip())
    values = [float(v) for v in parts[3:]]
    return block_number, time_ref, tdc_offset, values

def plot_waveform(block, time_ref, tdc_offset, values, pdf, bin_start=0.5, bin_end=109.5, num_bins=110):
    x = np.linspace(bin_start, bin_end, num_bins)
    y = np.array(values)

    # Create figure
    fig, ax = plt.subplots(figsize=(8, 5))

    # Scatter plot
    ax.plot(x, y, 'bo', label='Data')

    # Spline fit (smooth curve)
    try:
        spline = UnivariateSpline(x, y, s=0.0001)
        x_fine = np.linspace(x[0], x[-1], 1000)
        y_smooth = spline(x_fine)
        ax.plot(x_fine, y_smooth, 'r-', label='Spline Fit')
    except Exception as e:
        print(f"Could not fit spline for block {block}: {e}")

    # Titles and labels
    ax.set_title(f"Block {block} | timeRef = {time_ref:.3f} | tdcOffset = {tdc_offset:.2f}")
    ax.set_xlabel("Time [bins]")
    ax.set_ylabel("Amplitude")
    ax.grid(True)
    ax.legend()

    # Save to PDF
    pdf.savefig(fig)
    plt.close(fig)

def main():
    parser = argparse.ArgumentParser(description="Plot waveform spec file into multi-page PDF.")
    parser.add_argument("input_file", help="Input specification file (text format)")
    parser.add_argument("output_pdf", help="Output PDF file with waveform plots")
    args = parser.parse_args()

    with open(args.input_file, 'r') as f:
        lines = f.readlines()

    with PdfPages(args.output_pdf) as pdf:
        for line in lines:
            if line.strip().startswith("#") or not line.strip():
                continue  # skip comments and blank lines
            parsed = parse_spec_line(line)
            if parsed is None:
                continue
            block, time_ref, tdc_offset, values = parsed
            plot_waveform(block, time_ref, tdc_offset, values, pdf)

    print(f"✅ All plots saved to {args.output_pdf}")

if __name__ == "__main__":
    main()
