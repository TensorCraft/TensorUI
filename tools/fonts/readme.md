### Font Tool for creating fonts that can be used in TensorUI
#### `.bfont` format:
+ first 128 char: containing the widths of ascii characters
+ 129th char: the height of this font
+ 130th char: the basic font size
+ the rest is the font data.
#### Requirements:
`pip3 install requirements.txt`
#### FontPreview.py
A tool to preview the `.bfont` files
#### FontToolGUI.py
A tool to convert `.ttf` files to `.bfont`
The `threshold` slider determines the threshold when turning the gray scale font pixel data into B&W.