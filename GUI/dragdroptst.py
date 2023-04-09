#!/usr/bin/python

import sys

from PyQt6.QtCore import Qt, QMimeData
from PyQt6.QtGui import QDrag, QPixmap
from PyQt6.QtWidgets import QApplication, QHBoxLayout, QWidget, QPushButton

{
# class DragButton(QPushButton):

#     def mouseMoveEvent(self, e):
        
#         if e.buttons() == Qt.MouseButton.LeftButton:
#             drag = QDrag(self)
#             mime = QMimeData()
#             drag.setMimeData(mime)

#             pixmap = QPixmap(self.size())
#             self.render(pixmap)
#             drag.setPixmap(pixmap)

#             drag.exec(Qt.DropAction.MoveAction)

# class Window(QWidget):

#     def __init__(self):
#         super().__init__()
#         self.setAcceptDrops(True)

#         self.blayout = QHBoxLayout()
#         for l in ['A', 'B', 'C', 'D']:
#             btn = DragButton(l)
#             self.blayout.addWidget(btn)

#         self.setLayout(self.blayout)

#     def dragEnterEvent(self, e):
#         e.accept()

#     def dropEvent(self, e):
#         pos = e.position().toPoint()
#         widget = e.source()

#         for n in range(self.blayout.count()):
#             # Get the widget at each index in turn.
#             w = self.blayout.itemAt(n).widget()
#             if pos.x() < w.x() + w.size().width() // 2:
#                 # We didn't drag past this widget.
#                 # insert to the left of it.
#                 self.blayout.insertWidget(n-1, widget)
#                 break

#         e.accept()


# app = QApplication([])
# w = Window()
# w.show()

# app.exec()
}

#{
class DragButton(QPushButton):

    def mousePressEvent(self, event):
        self.__mousePressPos = None
        self.__mouseMovePos = None
        if event.button() == Qt.MouseButton.LeftButton:
            self.__mousePressPos = event.globalPosition().toPoint()
            self.__mouseMovePos = event.globalPosition().toPoint()
        
        super(DragButton, self).mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.MouseButton.LeftButton:
            # adjust offset from clicked point to origin of widget
            currPos = self.mapToGlobal(self.pos())
            globalPos = event.globalPosition().toPoint()
            diff = globalPos - self.__mouseMovePos
            newPos = self.mapFromGlobal(currPos + diff)
            self.move(newPos)

            self.__mouseMovePos = globalPos

        super(DragButton, self).mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self.__mousePressPos is not None:
            moved = event.globalPosition().toPoint() - self.__mousePressPos 
            if moved.manhattanLength() > 3:
                event.ignore()
                return

        super(DragButton, self).mouseReleaseEvent(event)

def clicked():
    print ("click as normal!")

if __name__ == "__main__":
    app = QApplication([])
    w = QWidget()
    w.resize(800,600)

    button = DragButton("Drag", w)
    button.clicked.connect(clicked)

    w.show()
    app.exec()
#}

{
# class Button(QPushButton):

#     def __init__(self, title, parent):
#         super().__init__(title, parent)


#     def mouseMoveEvent(self, e):

#         if e.buttons() != Qt.MouseButton.RightButton:
#             return

#         mimeData = QMimeData()

#         drag = QDrag(self)
#         drag.setMimeData(mimeData)

#         pixmap = QPixmap(self.size())
#         self.render(pixmap)
#         drag.setPixmap(pixmap)

#         drag.setHotSpot(e.position().toPoint() - self.rect().topLeft())

#         dropAction = drag.exec(Qt.DropAction.MoveAction)


#     def mousePressEvent(self, e):

#         super().mousePressEvent(e)

#         if e.button() == Qt.MouseButton.LeftButton:
#             print('press')


# class Example(QWidget):

#     def __init__(self):
#         super().__init__()

#         self.initUI()


#     def initUI(self):

#         self.setAcceptDrops(True)

#         self.button = Button('Button', self)
#         self.button.move(100, 65)

#         self.setWindowTitle('Click or Move')
#         self.setGeometry(300, 300, 550, 450)


#     def dragEnterEvent(self, e):

#         e.accept()


#     def dropEvent(self, e):

#         position = e.position()
#         self.button.move(position.toPoint())

#         e.setDropAction(Qt.DropAction.MoveAction)
#         e.accept()


# def main():
    
#     app = QApplication(sys.argv)
#     ex = Example()
#     ex.show()
#     app.exec()


# if __name__ == '__main__':
#     main()
}