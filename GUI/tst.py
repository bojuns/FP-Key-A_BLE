from datetime import datetime
# import serial
import matplotlib.pyplot as plt
import numpy as np
import time

def plotloop():
    # creating initial data values
    # of x and y
    x = np.linspace(0, 10, 100)
    y = np.sin(x)
    
    # to run GUI event loop
    plt.ion()
    
    # here we are creating sub plots
    figure, ax = plt.subplots(figsize=(10, 8))
    line1, = ax.plot(x, y)
    
    # setting title
    plt.title("Light", fontsize=20)
    
    # setting x-axis label and y-axis label
    plt.xlabel("sample number")
    plt.ylabel("light level")
    plt.show()
    
    i = 1
    # Loop
    while(True):
        # creating new Y values
        new_y = np.sin(x-0.5*i)
    
        # updating data values
        line1.set_xdata(x)
        line1.set_ydata(new_y)
    
        # drawing updated values
        figure.canvas.draw()
    
        # This will run the GUI event
        # loop until all UI events
        # currently waiting have been processed
        figure.canvas.flush_events()
    
        time.sleep(0.1)
        i += 1

plotloop()