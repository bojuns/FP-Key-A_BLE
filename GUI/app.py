# Only needed for access to command line arguments
import sys

from PyQt6.QtCore import QSize, Qt, QPoint, QPointF
from PyQt6.QtWidgets import *
from PyQt6.QtGui import * 

KEYNUM = 16
ROWS = 4
COLS = 4
KEYSIZE = 50
LAYERS = 2
# Subclass of QLineEdit representing a key with a keyboard shortcut linked
class Key(QLineEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(KEYSIZE,KEYSIZE)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def mousePressEvent(self, event):
        self.__mousePressPos = None
        self.__mouseMovePos = None
        if event.button() == Qt.MouseButton.LeftButton:
            self.__mousePressPos = event.globalPosition().toPoint()
            self.__mouseMovePos = event.globalPosition().toPoint()
        
        super(Key, self).mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.MouseButton.LeftButton:
            # adjust offset from clicked point to origin of widget
            currPos = self.pos()
            globalPos = event.globalPosition().toPoint()
            
            keyboardRect = self.parentWidget().geometry()
            keyboardRect.moveTopLeft(QPoint(0,0))
            
            # Calculating Move Amount in terms of parent coords
            diff = globalPos - self.__mouseMovePos
            # print("Diff: ", currPos, diff, currPos + diff, globalPos)
            newPos_parent = currPos + diff
            # Calculating Target Position Rect
            keyRect = self.geometry()
            keyRect.moveTopLeft(newPos_parent)

            # Force in bounds
            if (not keyboardRect.contains(keyRect, proper=True)):
                xmax = keyboardRect.width() - keyRect.width()
                ymax = keyboardRect.height() - keyRect.height()
                if keyRect.left() < 0:
                    keyRect.moveLeft(0)
                elif keyRect.left() > xmax:
                    keyRect.moveLeft(xmax)
                if keyRect.top() < 0:
                    keyRect.moveTop(0)
                elif keyRect.top() > ymax:
                    keyRect.moveTop(ymax)
                self.__mouseMovePos = self.parentWidget().mapToGlobal(keyRect.center())
            else:
                self.__mouseMovePos = globalPos

            newPos_parent = keyRect.topLeft()       
            self.move(newPos_parent)
        
        super(Key, self).mouseMoveEvent(event)


    def mouseReleaseEvent(self, event):
        if self.__mousePressPos is not None:
            moved = event.globalPosition().toPoint() - self.__mousePressPos
            if moved.manhattanLength() > 3:
                event.ignore()
                return
        
        super(Key, self).mouseReleaseEvent(event)

class KeyBoard(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.currLayer = 0
        self.keyList = []
        self.layers = []
        for i in range(LAYERS):
            self.layers.append(['\0']*KEYNUM)

    def setKeyList(self, keyList):
        self.keyList = keyList

    def resizeEvent(self, a0: QResizeEvent) -> None:
        return super().resizeEvent(a0)

# updates the keybind for the nth key
def updateKeybind(keyboard: KeyBoard, n):
    keyboard.layers[keyboard.currLayer][n] = keyboard.keyList[n].text()

def configure(keyboard):
    locations = "["
    keybinds = "["
    for key in keyboard.keyList:
        locations += ("(%d,%d)" % (key.pos().x(), key.pos().y())) + ", "
        keybinds += key.text() + ","
    locations = locations[:-2] + "]"
    keybinds = keybinds[:-1] + "]"
    print(locations)
    print(keybinds)
    file = open('locations.txt', 'w')
    file.writelines([locations])
    file.close()
    file = open('keybinds.txt', 'w')
    file.writelines([keybinds])
    file.close()

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

        layerOptions = QWidget(self)
        layerOptions_layout = QHBoxLayout()

        layerstr = QLabel("Layer: ", layerOptions)
        layerstr.setFixedSize(QSize(50, 30))
        layerChoice = QComboBox(layerOptions)
        layerChoice.setFixedSize(QSize(100, 30))
        for i in range(LAYERS):
            layerChoice.addItem(str(i))
        # layerNum = QLineEdit(layerOptions)

        layerOptions_layout.addWidget(layerstr)
        layerOptions_layout.addWidget(layerChoice)
        # layerOptions_layout.addWidget(layerNum)
        layerOptions.setLayout(layerOptions_layout)

        keyboard = KeyBoard(main_widget)
        layout = QGridLayout()
        layout.setSpacing(10)
        keyboard.setLayout(layout)
        keyboard.setStyleSheet("border: 1px solid blue")

        button = QPushButton("Configurate!")
        button.setFixedSize(QSize(200,100))
        button.setToolTip(self.tr("Configure your new keyboard shortcuts!"));
        button.clicked.connect(lambda: configure(keyboard))
        keys = [Key(keyboard) for i in range(KEYNUM)]
        keyboard.setKeyList(keys)

        for i in range(ROWS):
            for j in range(COLS):
                n = i*ROWS+j
                layout.addWidget(keys[n], i, j)
                keys[n].editingFinished.connect(lambda index=n: updateKeybind(keyboard, index))

        main_layout.addWidget(button, alignment=Qt.AlignmentFlag.AlignCenter)
        main_layout.addWidget(layerOptions)
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
    }
    QPushButton:hover:!pressed {
        border-width: 2px;
        border-style: solid;    /* Basically outline type - defaults to none */
        border-color: rgba(50, 255, 255, 75%);
    }
    QPushButton:pressed {
        background-color: rgba(100, 255, 150, 100%);
    }
    QLineEdit {
        background-color: rgba(100, 255, 255, 75%);
        color: "black";
        border-radius: 4px;
    }
    QLabel {
        font-size: 20px;
        color: rgb(100, 255, 255);
    }
    QComboBox {
        font-size: 20px;
        background-color: "black";
        color: rgb(100, 255, 255);
    }
""")

window.show()  # IMPORTANT!!!!! Windows are hidden by default.
# Start the event loop.
app.exec()


# Your application won't reach here until you exit and the event
# loop has stopped.