# Only needed for access to command line arguments
import sys
import serial

from PyQt6.QtCore import QSize, Qt, QPoint, QPointF
from PyQt6.QtWidgets import *
from PyQt6.QtGui import * 

KEYNUM = 16
ROWS = 4
COLS = 4
KEYSIZE = 50
LAYERS = 2 # this is also set in the microcontroller (must change both to edit) 

ser = None

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
        self.KEYCAPS = []

    def setKeycaps(self, keycaps: list):
        self.KEYCAPS = keycaps
        for n in range(KEYNUM):
            key = self.keyList[n]
            key.setToolTip(self.tr(self.KEYCAPS[n]));
    
    def setKeyList(self, keyList):
        self.keyList = keyList

    def setKeybinds(self, keybinds):
        self.layers = keybinds
    # updates the keybind for the nth key
    def updateKeybind(self, n):
        self.layers[self.currLayer][n] = self.keyList[n].text()

    def switchLayer(self, layer_index):
        print(f'Layer Switched to {layer_index}')
        self.currLayer = layer_index    # update current layer
        for i in range(len(self.keyList)):
            key = self.keyList[i]
            key.setText(self.layers[self.currLayer][i]) # update each key to disp new layer

    def resizeEvent(self, a0: QResizeEvent) -> None:
        if (a0.oldSize() == QSize(-1,-1)):
            return super().resizeEvent(a0)
        xscale = a0.size().width()/a0.oldSize().width()
        yscale = a0.size().height()/a0.oldSize().height()
        for key in self.keyList:
            key.move(round(key.pos().x()*xscale), round(key.pos().y()*yscale))
        return super().resizeEvent(a0)

# Send keymaps via UART
def sendKeymaps():
    return

def configure(keyboard: KeyBoard):
    # saving locations 
    locations = "["
    xscale = keyboard.width()
    yscale = keyboard.height()
    for key in keyboard.keyList:
        locations += ("(%f,%f)" % (key.pos().x()/xscale, key.pos().y()/yscale)) + ", "
    locations = locations[:-2] + "]"
    file = open('locations.txt', 'w')
    file.writelines([locations])
    file.close()

    # saving keybinds
    keybinds = keyboard.layers
    file = open('keybinds.txt', 'w')
    file.writelines(str(keybinds))
    file.close()

    #saving keycaps
    keycaps = keyboard.KEYCAPS
    file = open('keycaps.txt', 'w')
    file.writelines(str(keycaps))
    file.close()

    # Sending to board through serial
    keybindUART = "["
    for l in range(LAYERS):
        for k in range(KEYNUM):
            keybindUART += keybinds[l][k] + ','
    keybindUART = keybindUART[:-1]
    keybindUART += "]" 
    print(keybindUART)
    ser = serial.Serial('/dev/tty.usbmodem102', 9600, timeout=0.5)
    ser.write(keybindUART.encode())

    
class CustomDialog(QDialog):
    # widgets = list of qwidgets to add
    def __init__(self, parent=None, Btns = QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel, widgets = []):
        super().__init__(parent)

        self.buttonBox = QDialogButtonBox(Btns)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)

        self.layout = QVBoxLayout()
        for w in widgets:
            self.layout.addWidget(w)
        self.layout.addWidget(self.buttonBox)
        self.setLayout(self.layout)

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

        layerOptions_layout.addWidget(layerstr)
        layerOptions_layout.addWidget(layerChoice)
        layerOptions.setLayout(layerOptions_layout)

        self.keyboard = KeyBoard(main_widget)
        keyboard = self.keyboard
        keyboard.setStyleSheet("border: 1px solid blue")
        keyboard.setMinimumHeight(400)
        keyboard.resize(782, 400)

        button = QPushButton("Configurate!")
        button.setFixedSize(QSize(200,100))
        button.setToolTip(self.tr("Configure your new keyboard shortcuts!"));
        button.clicked.connect(lambda: configure(keyboard))

        layerChoice.currentIndexChanged.connect(keyboard.switchLayer)
        
        keys = [Key(keyboard) for i in range(KEYNUM)]
        keyboard.setKeyList(keys)

        f = open("locations.txt")
        keylocations = eval(f.read())
        f.close()
        f = open("keybinds.txt")
        keybinds = eval(f.read())   # [layer0array, layer1array, etc]
        f.close()
        keyboard.setKeybinds(keybinds)
        f = open("keycaps.txt")
        keycaps = eval(f.read())
        f.close()
        keyboard.setKeycaps(keycaps)

        xscale = keyboard.width()
        yscale = keyboard.height()
        for n in range(KEYNUM):
                keys[n].move(round(keylocations[n][0]*xscale), round(keylocations[n][1]*yscale))
                keys[n].setText(keybinds[keyboard.currLayer][n])
                keys[n].editingFinished.connect(lambda index=n: keyboard.updateKeybind(index))
                keys[n].setToolTip(self.tr(keyboard.KEYCAPS[n]));

        main_layout.addWidget(button, alignment=Qt.AlignmentFlag.AlignCenter)
        main_layout.addWidget(layerOptions)
        main_layout.addWidget(keyboard)
        main_layout.setStretchFactor(keyboard, 3)
        self.statusBar = QStatusBar()
        self.setStatusBar(self.statusBar)
        self.statusBar.showMessage("Hover over keys for corresponding keycap value!", 10000000)

        bar = self.menuBar()
        file = bar.addMenu("Menu")
        file.addAction("Edit")
        file.addAction("Help")
        file.triggered[QAction].connect(self.processMenu)
        self.setWindowTitle("FPKeyA Configuator")

    def processMenu(self,q):
        if (q.text() == "Help"):
            helptext = QTextEdit(self)
            helptext.setReadOnly(True)
            helptext.setText(
                "User Hints: \n"
                "1. Hovering over each key will show the label of the physical key's keycap it corresponds to. \n"
                "2. Modifier Keys avalible: Ctrl, Shift, Alt, Cmd \n"
            )
            dlg = CustomDialog(self, QDialogButtonBox.StandardButton.Ok, [helptext])
            dlg.setWindowTitle("Help")
            dlg.exec()
              
        if q.text() == "Edit":
            instructions = QLabel(self)
            instructions.setText(
                "Replace the List of Old Keycap Labels with your new ones!\n"
                "This will help you identify which key is which, so be sure to keep the original order. \n"
                "Replace the old keycap labels inbetween the 2 single quotes with your new keycap labels."
            )
            newKeycaps = QLineEdit(str(self.keyboard.KEYCAPS), self)
            dlg = CustomDialog(self, QDialogButtonBox.StandardButton.Ok, [instructions, newKeycaps])
            dlg.setWindowTitle("Edit Keycaps")
            if dlg.exec():
                self.keyboard.setKeycaps(eval(newKeycaps.text()))
            else:
                print("Cancel!")

def main():
    # You need one (and only one) QApplication instance per application.
    # Pass in sys.argv to allow command line arguments for your app.
    # If you know you won't use command line arguments QApplication([]) works too.
    app = QApplication(sys.argv)
    window = MainWindow()
    window.resize(800,600)
    app.setStyleSheet("""
        QMainWindow {
            background-color: "black";
            color: "white";
        }
        QMessageBox {
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
            font-size: 12px;
            color: black;
        }
        QStatusBar{
            padding-left:8px;
            background:rgba(40, 40, 100, 75%);
            color:white;
            font-weight:bold;
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

if __name__ == "__main__":
    main()