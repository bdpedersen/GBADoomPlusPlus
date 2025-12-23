#!/usr/bin/env python3
import re
import sys
from pathlib import Path

def extract_c_array_bytes(c_text, array_name):
    """
    Extract byte values from a C array definition.
    Returns a list of integers (0–255).
    """

    # Find the array body: array_name[...] = { ... };
    pattern = rf"{array_name}\s*\[.*?\]\s*=\s*\{{(.*?)\}};"
    match = re.search(pattern, c_text, re.S)

    if not match:
        raise ValueError(f"Array '{array_name}' not found")

    body = match.group(1)

    # Remove C comments
    body = re.sub(r"//.*?$", "", body, flags=re.M)
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)

    # Find numbers: hex or decimal
    tokens = re.findall(r"0x[0-9a-fA-F]+|\d+", body)

    bytes_out = []
    for t in tokens:
        value = int(t, 16) if t.startswith("0x") else int(t)
        if not 0 <= value <= 255:
            raise ValueError(f"Value out of byte range: {value}")
        bytes_out.append(value)

    return bytes_out


def main():
    if len(sys.argv) != 4:
        print("Usage:")
        print("  c_array_to_bin.py <input.c> <array_name> <output.bin>")
        sys.exit(1)

    input_c = Path(sys.argv[1])
    array_name = sys.argv[2]
    output_bin = Path(sys.argv[3])

    c_text = input_c.read_text(encoding="utf-8", errors="ignore")
    data = extract_c_array_bytes(c_text, array_name)

    output_bin.write_bytes(bytes(data))
    print(f"Wrote {len(data)} bytes to {output_bin}")


if __name__ == "__main__":
    main()
