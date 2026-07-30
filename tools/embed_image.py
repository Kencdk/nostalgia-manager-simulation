#!/usr/bin/env python3
"""
Convert an image file to a C++ byte array for embedding.
Usage: python embed_image.py input.png output.cpp
"""
import sys
import os

def embed_image(input_path, output_path):
    """Convert image file to C++ byte array."""
    if not os.path.exists(input_path):
        print(f"Error: Input file '{input_path}' not found")
        return False

    # Read the image file
    with open(input_path, 'rb') as f:
        data = f.read()

    # Get filename for variable name
    basename = os.path.splitext(os.path.basename(input_path))[0]
    var_name = f"g_{basename}_data"
    size_name = f"g_{basename}_size"

    # Write C++ file
    with open(output_path, 'w') as f:
        f.write('// Auto-generated embedded image data\n')
        f.write(f'// Source: {os.path.basename(input_path)}\n')
        f.write(f'// Size: {len(data)} bytes\n\n')
        f.write('#include "PlayerDetailBg.h"\n\n')
        f.write('namespace nm {\n\n')
        f.write(f'const unsigned char {var_name}[] = {{\n')

        # Write bytes in rows of 16
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_values = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f'    {hex_values},\n')

        f.write('};\n\n')
        f.write(f'const size_t {size_name} = sizeof({var_name});\n\n')
        f.write('}  // namespace nm\n')

    print(f"Successfully embedded {len(data)} bytes from '{input_path}' to '{output_path}'")
    return True

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python embed_image.py <input_image> <output.cpp>")
        print("Example: python embed_image.py Playerdetails.png PlayerDetailBg.cpp")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    if not embed_image(input_file, output_file):
        sys.exit(1)
