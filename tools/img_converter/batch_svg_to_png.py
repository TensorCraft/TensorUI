import os
from svglib.svglib import svg2rlg
from reportlab.graphics import renderPM

def svg_to_png(svg_path, png_path):
    print(f"Converting {svg_path} to {png_path}...")
    drawing = svg2rlg(svg_path)
    renderPM.drawToFile(drawing, png_path, fmt="PNG")

icons = [
    "icon_calc",
    "icon_settings",
    "icon_paint",
    "icon_snake", 
    "icon_about",
    "logo_md"
]

for icon in icons:
    svg_file = f"tmp/{icon}.svg"
    png_file = f"tmp/{icon}.png"
    if os.path.exists(svg_file):
        svg_to_png(svg_file, png_file)
    else:
        print(f"Warning: {svg_file} not found")
