#!/usr/bin/env python3
import sys

import serial
import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore


class LinePlot:
  """ Line Plot """
  def __init__(self, win, title, x_key, y_keys, x_label, y_label, **kwargs):
    self.plot = win.addPlot(title=title)
    self.plot.addLegend()
    self.plot.setLabel("bottom", x_label)
    self.plot.setLabel("left", y_label)

    y_min = kwargs.get("y_min")
    y_max = kwargs.get("y_max")
    padding_y = kwargs.get("padding_y", 10)
    if y_min and y_max:
      self.plot.setLimits(yMin=y_min, yMax=y_max)
      self.plot.setYRange(y_min, y_max, padding=padding_y)

    self.win_size = kwargs.get("win_size", 500)
    self.colors = kwargs.get("colors", ["r", "g", "b", "w"])
    self.x_key = x_key
    self.y_keys = y_keys
    self.data = {}
    self.curves = {}

    self.data[x_key] = []
    for i, y_key in enumerate(self.y_keys):
      self.data[y_key] = []
      self.curves[y_key] = self.plot.plot(pen=self.colors[i], name=y_key)

  def update(self, data):
    """ Update """
    self.data[self.x_key].append(data[self.x_key])

    x_window = self.data[self.x_key][-self.win_size:]
    x_min = x_window[0]
    x_max = x_window[-1]
    self.plot.setXRange(x_min, x_max)

    for y_key in self.y_keys:
      self.data[y_key].append(data[y_key])
      y_window = self.data[y_key][-self.win_size:]
      self.curves[y_key].setData(x_window, y_window)


class FirmwareDebugger:
  """ Firmware Debugger """
  def __init__(self):
    self.s = None  # Serial
    self.app = None
    self.win = None

    self._setup_uart()
    self._setup_gui()
    self._start_plots()

  def _setup_uart(self):
    """ Setup UART comms """
    # Setup serial communication
    self.s = serial.Serial()
    self.s.port = '/dev/ttyACM0'
    self.s.baudrate = 115200
    self.s.timeout = 10
    self.s.open()

    if self.s.is_open is True:
      print("Connected to FCU ...")
    else:
      print("Failed to connect to FCU ...")
      sys.exit(-1)

  def _setup_gui(self):
    """ Setup GUI """
    self.app = pg.mkQApp("Firmware Debugger")
    self.win = pg.GraphicsLayoutWidget(show=True, title="Firmware Debugger")
    self.win.resize(640, 480)
    pg.setConfigOptions(antialias=True)

    title = "SBUS"
    x_key = "ts"
    y_keys = ["ch[0]", "ch[1]", "ch[2]", "ch[3]"]
    x_label = "Time [s]"
    y_label = "SBUS Value"
    kwargs = {"y_min": 175, "y_max": 1850}
    self.sbus_plot = LinePlot(
        self.win,
        title,
        x_key,
        y_keys,
        x_label,
        y_label,
        **kwargs,
    )

    title = "Attitude"
    x_key = "ts"
    y_keys = ["roll", "pitch", "yaw"]
    x_label = "Time [s]"
    y_label = "Attitude [deg]"
    kwargs = {"y_min": -60.0, "y_max": 60.0}
    self.imu_plot = LinePlot(
        self.win,
        title,
        x_key,
        y_keys,
        x_label,
        y_label,
        **kwargs,
    )

  def _start_plots(self):
    """ Start plots """
    timer = QtCore.QTimer()
    timer.setInterval(0)
    timer.timeout.connect(self._update_plots)
    timer.start()
    pg.exec()

  def _update_plots(self):
    """ Update plots """
    data = self.parse_serial_data(self.s.readline())
    data['ts'] = data['ts'] * 1e-6
    self.sbus_plot.update(data)
    self.imu_plot.update(data)

  @staticmethod
  def parse_serial_data(line):
    """ Parse serial data """
    line = line.strip()

    data = {}
    for el in line.split(b" "):
      key = el.split(b":")[0].strip().decode("ascii")
      val = float(el.split(b":")[1].strip())
      data[key] = val

    return data


if __name__ == "__main__":
  FirmwareDebugger()
