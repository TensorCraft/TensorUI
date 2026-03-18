import sys
import os
from PIL import Image

def convert_to_bitmap(image_path, output_name):
    try:
        img = Image.open(image_path).convert('RGBA')
    except Exception as e:
        print(f"Error opening image: {e}")
        return

    width, height = img.size
    pixels = list(img.getdata())

    header_content = f"""#ifndef {output_name.upper()}_H
#define {output_name.upper()}_H

#include "TensorUI/Color/color.h"

#define {output_name.upper()}_WIDTH {width}
#define {output_name.upper()}_HEIGHT {height}

static const Color {output_name}_data[] = {{
"""

    for i, (r, g, b, a) in enumerate(pixels):
        is_transparent = "true" if a < 128 else "false"
        line_end = ",\n" if i < len(pixels) - 1 else "\n"
        header_content += f"    {{{r}, {g}, {b}, {is_transparent}}}{line_end}"


    header_content += f"""}};

#endif
"""
    
    output_file = f"{output_name}.h"
    with open(output_file, "w") as f:
        f.write(header_content)
    
    print(f"Successfully converted {image_path} to {output_file}")
    print(f"Width: {width}, Height: {height}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python img_to_bitmap.py <image_path> <array_name>")
    else:
        convert_to_bitmap(sys.argv[1], sys.argv[2])
