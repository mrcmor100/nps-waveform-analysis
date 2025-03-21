#!/usr/bin/env python3
import argparse
import glob
import os
import re

def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert waveform files and a TDC offset file into a specification file."
    )
    parser.add_argument("input_dir", help="Directory containing waveform files.")
    parser.add_argument("tdc_offset_file", help="File with 1080 TDC offsets.")
    parser.add_argument("output_file", help="Filename for the output specification file.")
    return parser.parse_args()

def read_tdc_offsets(filename):
    with open(filename, 'r') as f:
        contents = f.read()
    return [float(x) for x in contents.split()]

def extract_block_number(filename):
    base = os.path.basename(filename)
    m = re.search(r"_(\d+)\.txt$", base)
    if m:
        return int(m.group(1))
    else:
        raise ValueError(f"Cannot extract block number from {filename}")

def extract_run_range_from_dir(dir_path):
    m = re.search(r'RWF_(\d+_\d+)', dir_path)
    return m.group(1) if m else "unknown_range"

def read_waveform_file(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()
    if not lines:
        raise ValueError(f"Empty file: {filename}")
    time_ref = float(lines[0].split()[0])
    # Second column of each line after the first — treated as string
    values = [line.split()[1] for line in lines[1:] if len(line.split()) >= 2]
    return time_ref, values

# def format_value_field(s, width=13):
#     """Formats waveform string to be right-aligned in fixed width.
#        Negative signs preserved; no plus signs; space for positives."""
#     return s.rjust(width)

def format_tdc_offset(v):
    """Formats tdc_offset as 6-character field with 2 decimals, space-padding for positive."""
    return f"{v:6.2f}"

def main():
    args = parse_args()
    tdc_offsets = read_tdc_offsets(args.tdc_offset_file)
    run_range = extract_run_range_from_dir(args.input_dir)

    # Collect waveform files by block number
    pattern = os.path.join(args.input_dir, "ref_wf_*.txt")
    file_list = glob.glob(pattern)
    block_files = {}
    for fname in file_list:
        try:
            block_number = extract_block_number(fname)
            block_files[block_number] = fname
        except Exception as e:
            print(f"Skipping file {fname}: {e}")

    sorted_blocks = sorted(block_files.keys())

    with open(args.output_file, 'w') as outf:
        # Header
        outf.write("# Header for structure\n")
        outf.write("# Version 1.0\n")
        outf.write(f"# These waveforms were determined for elastic runs {run_range}.\n")
        outf.write("# Bining of References: min max step\n")
        outf.write("0.5 109.5 110\n")
        outf.write("# Actual block data:\n")
        outf.write("# block_number, timeRef, tdc_offset, values[]\n")

        for block_number in sorted_blocks:
            fname = block_files[block_number]
            try:
                time_ref, values = read_waveform_file(fname)
                tdc_offset = tdc_offsets[block_number] if block_number < len(tdc_offsets) else 0.0

                block_str = f"{block_number:>4}"
                time_str = f"{time_ref:7.3f}"  # Width 7, 3 decimal places
                tdc_str = format_tdc_offset(tdc_offset)

                # Format waveform values as aligned strings
                formatted_values = ", ".join((v) for v in values)

                # Final formatted line
                outf.write(f"{block_str}, {time_str}, {tdc_str}, {formatted_values}\n")
            except Exception as e:
                print(f"Failed to process block {block_number}: {e}")

    print("✅ Conversion complete:", args.output_file)

if __name__ == "__main__":
    main()
