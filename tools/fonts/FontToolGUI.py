import os
import platform
from PIL import Image, ImageDraw, ImageFont
import numpy as np
import sys
from PyQt6.QtWidgets import *
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QPixmap, QPainter, QColor
import struct

# Function to get font pixels for an ASCII character
def get_font_pixels(font_path, font_size, character, threshold):
    font = ImageFont.truetype(font_path, font_size)
    width = font.getlength(character)
    ascent, descent = font.getmetrics()
    height = ascent + descent

    # Create an image with the calculated size
    image = Image.new('L', (int(width), int(height)), color=0)
    draw = ImageDraw.Draw(image)

    # Draw the text at position (0, 0)
    draw.text((0, 0), character, font=font, fill=255)

    # Convert image pixels to a binary array (black & white based on threshold)
    pixel_data = np.array(image)
    binary_pixels = (pixel_data > threshold).astype(int)
    return binary_pixels, int(width), int(height)

# Function to detect the system font folder based on the OS
def get_system_font_folder():
    os_name = platform.system()
    if os_name == "Windows":
        return "C:\\Windows\\Fonts"
    elif os_name == "Darwin":  # macOS
        return "/System/Library/Fonts"
    elif os_name == "Linux":
        return "/usr/share/fonts"
    else:
        return None

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Font Tool")
        self.setGeometry(100, 100, 600, 400)

        # Main layout
        main_layout = QVBoxLayout()

        # Font List
        font_list_layout = QVBoxLayout()
        font_label = QLabel("Fonts:")
        self.font_list = QListWidget()
        font_list_layout.addWidget(font_label)
        font_list_layout.addWidget(self.font_list)

        # Load initial fonts from system folder
        self.load_fonts(get_system_font_folder())

        # Parameters
        parameter_layout = QVBoxLayout()
        param_label = QLabel("Parameters:")
        parameter_layout.addWidget(param_label)

        # Font Size Slider
        font_size_label = QLabel("Font Size: 15px")
        self.font_size_slider = QSlider(Qt.Orientation.Horizontal)
        self.font_size_slider.setRange(8, 72)
        self.font_size_slider.setValue(15)
        self.font_size_slider.valueChanged.connect(
            lambda value: font_size_label.setText(f"Font Size: {value}px")
        )
        self.font_size_slider.valueChanged.connect(self.generate_ascii_preview)
        parameter_layout.addWidget(font_size_label)
        parameter_layout.addWidget(self.font_size_slider)

        # Threshold Slider
        threshold_label = QLabel("Threshold: 128")
        self.threshold_slider = QSlider(Qt.Orientation.Horizontal)
        self.threshold_slider.setRange(0, 255)
        self.threshold_slider.setValue(128)
        self.threshold_slider.valueChanged.connect(
            lambda value: threshold_label.setText(f"Threshold: {value}")
        )
        self.threshold_slider.valueChanged.connect(self.generate_ascii_preview)
        parameter_layout.addWidget(threshold_label)
        parameter_layout.addWidget(self.threshold_slider)

        # Buttons
        button_layout = QHBoxLayout()
        choose_folder_button = QPushButton("Choose Folder")
        choose_folder_button.clicked.connect(self.choose_folder)
        generate_button = QPushButton("Generate Font")
        generate_button.clicked.connect(self.generateFont)
        button_layout.addWidget(choose_folder_button)
        button_layout.addWidget(generate_button)

        # Preview area
        preview_label = QLabel("Preview:")
        self.preview_area = QLabel()
        self.preview_area.setFixedHeight(200)
        self.preview_area.setStyleSheet("border: 1px solid black; background-color: white;")

        # Connect font list selection to preview update
        self.font_list.itemSelectionChanged.connect(self.generate_ascii_preview)

        # Add layouts to main layout
        main_layout.addLayout(font_list_layout)
        main_layout.addLayout(parameter_layout)
        main_layout.addLayout(button_layout)
        main_layout.addWidget(preview_label)
        main_layout.addWidget(self.preview_area)

        # Set central widget
        container = QWidget()
        container.setLayout(main_layout)
        self.setCentralWidget(container)
        
        self.font_folder = get_system_font_folder()

    def load_fonts(self, folder):
        """Load fonts from the specified folder into the font list."""
        self.font_list.clear()
        if folder:
            for font_name in os.listdir(folder):
                if font_name.endswith(".ttf"):
                    self.font_list.addItem(font_name)
            self.font_folder = folder

    def choose_folder(self):
        """Open a dialog to choose a folder and update the font list."""
        folder = QFileDialog.getExistingDirectory(self, "Select Font Folder")
        if folder:
            self.load_fonts(folder)

    def generate_ascii_preview(self):
        """Generate and display all ASCII characters in black and white based on the selected font."""
        selected_font_item = self.font_list.currentItem()
        if selected_font_item is None:
            return  # Exit if no font is selected

        font_name = selected_font_item.text()
        font_path = os.path.join(self.font_folder, font_name)

        font_size = self.font_size_slider.value()  # Get font size from slider
        threshold = self.threshold_slider.value()  # Get threshold from slider

        # Initial canvas size
        canvas_width = 640
        canvas_height = 480
        canvas = QPixmap(canvas_width, canvas_height)
        canvas.fill(Qt.GlobalColor.white)

        painter = QPainter(canvas)
        painter.setPen(QColor(0, 0, 0))

        x, y = 10, 100
        line_height = 0

        for i in range(32, 127):  # ASCII characters from 32 (space) to 126 (~)
            character = chr(i)
            try:
                pixels, width, height = get_font_pixels(font_path, font_size, character, threshold)
                
                # If the current y position is greater than the canvas height, expand the canvas
                if y + height > canvas.height() - 10:
                    # Create a new canvas with increased height
                    new_canvas_height = canvas.height() + height + 10
                    new_canvas = QPixmap(canvas.width(), new_canvas_height)
                    new_canvas.fill(Qt.GlobalColor.white)
                    
                    # Copy existing content to the new canvas
                    painter.end()  # End painting on the current canvas
                    painter = QPainter(new_canvas)
                    painter.drawPixmap(0, 0, canvas)
                    painter.setPen(QColor(0, 0, 0))

                    # Replace the old canvas with the new one
                    canvas = new_canvas

                # Draw the character pixels on the canvas
                for row in range(height):
                    for col in range(width):
                        if pixels[row, col] == 1:
                            painter.drawPoint(x + col, y + row)
                
                # Move x to the right for the next character, or start a new row if no space
                x += width + 10
                line_height = max(line_height, height)  # Keep track of the max height in the current line
                if x > canvas.width() - 50:
                    x = 10
                    y += line_height + 10
                    line_height = 0
            except OSError:
                print("Missing Bitmap for:", character)
                continue

        painter.end()

        # Set the QPixmap to the preview area
        self.preview_area.setPixmap(canvas)


    def generateFont(self):
        """Generate font data in binary format and save to a .bfont file."""
        selected_font_item = self.font_list.currentItem()
        if selected_font_item is None:
            QMessageBox.warning(self, "Warning", "Please select a font.")
            return

        font_name = selected_font_item.text()
        font_path = os.path.join(self.font_folder, font_name)

        font_size = self.font_size_slider.value()  # Get font size from slider
        threshold = self.threshold_slider.value()  # Get threshold from slider

        # Initialize data structures for widths and character pixel data
        widths = []
        character_data = []

        # Generate data for each ASCII character
        space_pixels, space_width, space_height = get_font_pixels(font_path, font_size, ' ', threshold)
        for i in range(128):  # ASCII range from 0 to 127
            try:
                character = chr(i)
                pixels, width, height = get_font_pixels(font_path, font_size, character, threshold)
            except Exception:
                # If character is missing, use the space character as fallback
                pixels, width, height = space_pixels, space_width, space_height
            
            widths.append(width)
            character_data.append(pixels.flatten())

        # Add height to the widths list
        widths.append(height)
        widths.append(font_size)

        # Save to binary file
        filename, _ = QFileDialog.getSaveFileName(self, "Save Font", font_name[:-4] + str(font_size), "Binary Font (*.bfont)")
        if filename:
            with open(filename, 'wb') as f:
                # Write widths as binary data
                f.write(struct.pack(f"{len(widths)}B", *widths))
                # Write character data as binary
                for data in character_data:
                    f.write(struct.pack(f"{len(data)}B", *data))

# Run the application
app = QApplication(sys.argv)
w = MainWindow()
w.show()
app.exec()
