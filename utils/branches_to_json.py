# Improved script with explicit debugging and correct tracking of `is_array: true` branches

import re
import json

# Input block of C++ code
cpp_code = """
Int_t NSampWaveForm;
tree->SetBranchAddress("Ndata.NPS.cal.fly.adcSampWaveform", &NSampWaveForm);
Double_t SampWaveForm[Ndata];
tree->SetBranchAddress("NPS.cal.fly.adcSampWaveform", &SampWaveForm);
"""

# Regex pattern to match variable declarations and `SetBranchAddress` calls
pattern = re.compile(r'(?P<type>(Double_t|Int_t))\s+(?P<var>\w+);\s*\n\s*tree->SetBranchAddress\("(?P<name>[\w\d\.\_]+)", &(?P=var)\);')

# Corrected script ensuring `is_array: true` is properly assigned to corresponding branches
# Fully corrected script ensuring proper `is_array: true` handling and fixing `NPS.NPS.` duplication

# Reset data structures
branches = []
ndata_map = {}  # Maps base branch names to their Ndata size indicators
processed_branches = set()  # Track added branches to avoid duplicates

# First pass: Extract Ndata indicators and track them
for match in pattern.finditer(cpp_code):
    branch_type = match.group("type")
    branch_name = match.group("name")
    variable_name = match.group("var")

    # If it's an Ndata indicator (size variable), store it
    if branch_name.startswith("Ndata."):
        base_name = branch_name.replace("Ndata.", "")  # Extract base name for tracking
        ndata_map[base_name] = variable_name  # Store size variable
        branches.append({
            "name": branch_name,
            "type": branch_type,
            "is_array": False,  # Ndata size indicators are scalars
            "variable": variable_name
        })
        processed_branches.add(branch_name)

# Second pass: Identify corresponding data arrays and mark them as `is_array: true`
for match in pattern.finditer(cpp_code):
    branch_type = match.group("type")
    branch_name = match.group("name")
    variable_name = match.group("var")

    if branch_name in processed_branches:
        continue  # Already processed in first pass

    # Extract base name to match against known Ndata indicators
    base_name = branch_name.replace("NPS.", "")  # Remove "NPS." prefix if present
    is_array = base_name in ndata_map  # Ensure proper mapping

    branches.append({
        "name": branch_name,
        "type": branch_type,
        "is_array": is_array,
        "variable": variable_name
    })
    processed_branches.add(branch_name)

# Ensure every `NPS.` branch is explicitly added when an `Ndata.` entry exists
for base_name, size_var in ndata_map.items():
    clean_base_name = base_name.replace("NPS.", "")  # Ensure no double prefixing
    corresponding_branch = f"NPS.{clean_base_name}"  # Correct prefixing

    if corresponding_branch not in processed_branches:
        branches.append({
            "name": corresponding_branch,
            "type": "Double_t",  # Assuming data arrays are Double_t
            "is_array": True,
            "variable": clean_base_name  # Keep the base name as the variable
        })
        processed_branches.add(corresponding_branch)

# Convert extracted branches into formatted JSON
json_output = "[\n" + ",\n".join(
    f'    {{ "name": "{b["name"]}", "type": "{b["type"]}", "is_array": {"true" if b["is_array"] else "false"}, "variable": "{b["variable"]}" }}'
    for b in branches
) + "\n]"

# Print the extracted JSON output
print(json_output)
