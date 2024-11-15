import sys
import struct
import numpy as np
from PyQt6.QtWidgets import *
from PyQt6.QtGui import QPixmap, QPainter, QColor
from PyQt6.QtCore import Qt

class FontViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Binary Font Viewer")
        self.setGeometry(100, 100, 640, 480)

        main_layout = QVBoxLayout()
        load_button = QPushButton("Load .bfont File")
        load_button.clicked.connect(self.load_bfont)
        main_layout.addWidget(load_button)

        self.canvas = QLabel()
        self.canvas.setFixedSize(640, 480)
        self.canvas.setStyleSheet("border: 1px solid black; background-color: white;")
        main_layout.addWidget(self.canvas)

        container = QWidget()
        container.setLayout(main_layout)
        self.setCentralWidget(container)

    def load_bfont(self):
        """Load the .bfont file and display the characters."""
        filename, _ = QFileDialog.getOpenFileName(self, "Open Binary Font", "", "Binary Font (*.bfont)")
        if not filename:
            return

        try:
            with open(filename, 'rb') as f:
                # Read widths (128 characters + 1 height + 1 font size)
                widths = struct.unpack("130B", f.read(130))

                char_height = widths[-2]
                char_widths = widths[:-2]  # The first 128 entries are character widths
                font_size = widths[-1]
                print("Font Size:", font_size)

                char_data = []
                for width in char_widths:
                    num_pixels = width * char_height
                    if num_pixels > 0:
                        data = struct.unpack(f"{num_pixels}B", f.read(num_pixels))
                        char_data.append(np.array(data).reshape((char_height, width)))
                    else:
                        char_data.append(np.zeros((char_height, width), dtype=int))  # Placeholder if width is zero

            self.render_characters(char_data, char_widths, char_height)
        
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to load .bfont file: {e}")

    def render_characters(self, char_data, char_widths, char_height):
        """Render characters on the canvas based on loaded font data."""
        canvas_width = 640
        canvas_height = 480
        x, y = 10, 10
        max_line_height = 0

        canvas = QPixmap(canvas_width, canvas_height)
        canvas.fill(Qt.GlobalColor.white)
        painter = QPainter(canvas)
        painter.setPen(QColor(0, 0, 0))

        for i in range(32, 127):  # Display printable ASCII characters (32 to 126)
            char_pixels = char_data[i]
            width = char_widths[i]

            if x + width > canvas.width() - 10:
                x = 10
                y += max_line_height + 10
                max_line_height = 0

            if y + char_height > canvas.height() - 10:
                new_canvas_height = canvas.height() + char_height + 10
                new_canvas = QPixmap(canvas.width(), new_canvas_height)
                new_canvas.fill(Qt.GlobalColor.white)
                painter.end()
                painter = QPainter(new_canvas)
                painter.drawPixmap(0, 0, canvas)
                canvas = new_canvas

            for row in range(char_height):
                for col in range(width):
                    if char_pixels[row, col] == 1:
                        painter.drawPoint(x + col, y + row)

            x += width + 10
            max_line_height = max(max_line_height, char_height)

        painter.end()
        self.canvas.setPixmap(canvas)

app = QApplication(sys.argv)
viewer = FontViewer()
viewer.show()
app.exec()
