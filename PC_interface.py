import sys
import serial
import serial.tools.list_ports
import threading
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QPushButton, QComboBox, QVBoxLayout, QWidget, QTextEdit, QHBoxLayout
from PyQt5.QtCore import QTimer, pyqtSignal, QObject
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
import re

class Worker(QObject):
    """Worker class for handling serial data in a separate thread"""
    data_received = pyqtSignal(str)  # Signal to pass data back to the main thread

    def __init__(self, ser):
        super().__init__()
        self.ser = ser

    def read_data(self):
        """Read data from the serial port in a loop and emit signal to update GUI"""
        try:
            while True:
                if self.ser.is_open:
                    data = self.ser.readline().decode('utf-8').strip()
                    if data:
                        self.data_received.emit(data)  # Emit signal to update GUI with the data
        except serial.SerialException:
            pass

class SerialPlotter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Serial Data Plotter")
        self.setGeometry(100, 100, 800, 600)
        
        # Initialize serial data storage
        self.ser = None
        self.data_x = []
        self.data_y = []
        self.data_z = []
        self.gyro_pssi = []
        self.gyro_phi = []
        self.gyro_rho = []
        
        # Initialize UI
        self.init_ui()
        
        # Start timer for updating plot
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(100)  # Update plot every 100 ms
        
    def init_ui(self):
        # Create main layout
        main_layout = QHBoxLayout()
        
        # Create layout for the graph (left side)
        graph_layout = QVBoxLayout()
        
        # Plot setup with two subplots for ACC and GYRO
        self.figure, (self.acc_ax, self.gyro_ax) = plt.subplots(2, 1, figsize=(6, 6), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        graph_layout.addWidget(self.canvas)
        
        # Add graph layout to main layout, occupying 70% of the window
        main_layout.addLayout(graph_layout, 7)
        
        # Create layout for the controls (right side)
        controls_layout = QVBoxLayout()
        
        # Port selection dropdown
        self.port_label = QLabel("Select Serial Port:")
        controls_layout.addWidget(self.port_label)
        
        self.port_combobox = QComboBox()
        self.populate_ports()
        controls_layout.addWidget(self.port_combobox)
        
        # Connect button
        self.connect_button = QPushButton("Connect")
        self.connect_button.clicked.connect(self.on_connect_button_click)
        controls_layout.addWidget(self.connect_button)
        
        # Status label
        self.status_label = QLabel("Status: Not connected")
        controls_layout.addWidget(self.status_label)
        
        # Data display area (Show only the most recent packet)
        self.data_display = QTextEdit(self)
        self.data_display.setReadOnly(True)
        controls_layout.addWidget(self.data_display)
        
        # Add controls layout to main layout, occupying 30% of the window
        main_layout.addLayout(controls_layout, 3)
        
        # Create a container for the layout
        container = QWidget()
        container.setLayout(main_layout)
        self.setCentralWidget(container)

        
    def populate_ports(self):
        """Populate the serial port combobox"""
        ports = self.list_ports()
        if not ports:
            self.status_label.setText("No serial ports found.")
        else:
            self.port_combobox.addItems(ports)
    
    def list_ports(self):
        """Lists all available COM ports"""
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]
    
    def connect(self, selected_port):
        """Connects to the selected serial port"""
        baud_rate = 9600  # Set your baud rate here
        try:
            self.ser = serial.Serial(selected_port, baud_rate, timeout=1)
            self.status_label.setText(f"Connected to {selected_port}")
            
            # Create a worker object to read data from the serial port
            self.worker = Worker(self.ser)
            self.worker.data_received.connect(self.display_latest_packet)  # Connect the signal to the slot
            
            # Start the worker thread
            self.worker_thread = threading.Thread(target=self.worker.read_data)
            self.worker_thread.daemon = True
            self.worker_thread.start()
        except serial.SerialException as e:
            self.status_label.setText(f"Could not connect to {selected_port}: {str(e)}")
    
    def display_latest_packet(self, data):
        """Display the most recent data packet in the text display"""
        self.data_display.append(f"Received data: {data}")
        if len(self.data_display.toPlainText().splitlines()) > 10:
            lines = self.data_display.toPlainText().splitlines()[1:]
            self.data_display.setPlainText('\n'.join(lines))
        
        self.parse_data(data)
    
    def parse_data(self, data):
        """Parse the incoming data and extract X, Y, Z, and gyro values"""
        try:
            parts = data.split()
            # Acceleration data
            accel_x = int(parts[1].strip(':')) / 16384  
            accel_y = int(parts[3].strip(':')) / 16384
            accel_z = int(parts[5].strip(':')) / 16384
            # Gyroscope data
            gyro_pssi = int(parts[7].strip(':')) / 131
            gyro_phi = int(parts[9].strip(':')) / 131
            gyro_rho = int(parts[11].strip(':')) / 131
            
            # Append data to lists
            self.data_x.append(accel_x)
            self.data_y.append(accel_y)
            self.data_z.append(accel_z)
            self.gyro_pssi.append(gyro_pssi)
            self.gyro_phi.append(gyro_phi)
            self.gyro_rho.append(gyro_rho)
            
            # Keep only the latest 100 values
            if len(self.data_x) > 100:
                self.data_x.pop(0)
                self.data_y.pop(0)
                self.data_z.pop(0)
                self.gyro_pssi.pop(0)
                self.gyro_phi.pop(0)
                self.gyro_rho.pop(0)
        except Exception as e:
            self.status_label.setText(f"Error parsing data: {str(e)}")

    def update_plot(self):
        """Update the plot with the latest data"""
        if self.ser and self.ser.is_open:
            # Clear previous plots
            self.acc_ax.clear()
            self.gyro_ax.clear()
            
            # Plot acceleration data
            self.acc_ax.plot(self.data_x, label='X Acceleration', color='r')
            self.acc_ax.plot(self.data_y, label='Y Acceleration', color='g')
            self.acc_ax.plot(self.data_z, label='Z Acceleration', color='b')
            self.acc_ax.set_title('X, Y, Z Acceleration')
            self.acc_ax.set_xlabel('Time (s)')
            self.acc_ax.set_ylabel('Acceleration (g)')
            self.acc_ax.set_ylim(-2, 2)
            self.acc_ax.legend()
            
            # Plot gyroscope data
            self.gyro_ax.plot(self.gyro_pssi, label='Pssi (X)', color='c')
            self.gyro_ax.plot(self.gyro_phi, label='Phi (Y)', color='m')
            self.gyro_ax.plot(self.gyro_rho, label='Rho (Z)', color='y')
            self.gyro_ax.set_title('Gyro Pssi, Phi, Rho')
            self.gyro_ax.set_xlabel('Time (s)')
            self.gyro_ax.set_ylabel('Angular Velocity (°/s)')
            self.gyro_ax.set_ylim(-250, 250)
            self.gyro_ax.legend()
            
            # Redraw the canvas
            self.canvas.draw()

    def on_connect_button_click(self):
        """Triggered when the connect button is clicked"""
        selected_port = self.port_combobox.currentText()
        if selected_port:
            self.connect(selected_port)
        else:
            self.status_label.setText("Please select a valid serial port.")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SerialPlotter()
    window.show()
    sys.exit(app.exec_())
