# Only needed for access to command line arguments
import sys

from PyQt6.QtCore import QSize, Qt
from PyQt6.QtWidgets import *
from PyQt6.QtGui import * 

KEYNUM = 16
ROWS = 4
COLS = 4

# Subclass of QLineEdit representing a key with a keyboard shortcut linked
class Key(QLineEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(50,50)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)

# Subclass QMainWindow to customize your application's main window
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("My App")        
        # Set the central widget of the Window.
        main_widget = QWidget(self)
        self.setCentralWidget(main_widget)
        # self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        main_layout = QVBoxLayout()
        main_widget.setLayout(main_layout)

        keyboard = QWidget(main_widget)
        layout = QGridLayout()
        layout.setSpacing(10)
        keyboard.setLayout(layout)

        button = QPushButton("Configurate!")
        button.setFixedSize(QSize(200,100))
        keys = [Key(keyboard) for i in range(KEYNUM)]

        for i in range(ROWS):
            for j in range(COLS):
                layout.addWidget(keys[i*ROWS+j], i, j)

        main_layout.addWidget(button, alignment=Qt.AlignmentFlag.AlignCenter)
        main_layout.addWidget(keyboard)

# You need one (and only one) QApplication instance per application.
# Pass in sys.argv to allow command line arguments for your app.
# If you know you won't use command line arguments QApplication([]) works too.
app = QApplication(sys.argv)

window = MainWindow()

app.setStyleSheet("""
    QMainWindow {
        background-color: "black";
        color: "white";
    }
    QPushButton {
        font-size: 16px;
        background-color: "darkgray";
        border-radius: 10px;
        border-width: 5px;
        border-color: "blue";
    }
    QPushButton:hover:!pressed {
        border-width: 2px;
        border-style: solid;    /* Basically outline type - defaults to none */
        border-color: rgba(50, 255, 255, 75%);
    }
    QPushButton:pressed {
        background-color: rgba(0, 255, 0, 75%);
    }
    QLineEdit {
        background-color: rgba(100, 255, 255, 75%);
        color: "black";
        border-radius: 4px;
    }
""")

window.show()  # IMPORTANT!!!!! Windows are hidden by default.
# Start the event loop.
app.exec()


# Your application won't reach here until you exit and the event
# loop has stopped.