import sys
import serial
import serial.tools.list_ports
import threading
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QPushButton, QComboBox, QVBoxLayout, QWidget, QTextEdit, QHBoxLayout
from PyQt5.QtCore import QTimer, pyqtSignal, QObject
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas

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
        
        # Plot setup
        self.figure, self.ax = plt.subplots(figsize=(6, 6), dpi=100)
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
        # Append the new data to the display and keep the latest 10 packets
        self.data_display.append(f"Received data: {data}")
        if len(self.data_display.toPlainText().splitlines()) > 10:
            # Remove the first (oldest) line if more than 10 lines
            lines = self.data_display.toPlainText().splitlines()[1:]
            self.data_display.setPlainText('\n'.join(lines))
        
        # Parse the data to extract X, Y, Z values
        self.parse_data(data)
    
    def parse_data(self, data):
        """Parse the incoming data and extract X, Y, Z values"""
        try:
            parts = data.split()
            accel_x = int(parts[1].strip(':')) / 16384  
            accel_y = int(parts[3].strip(':')) / 16384
            accel_z = int(parts[5].strip(':')) / 16384
            
            # Append the new data to the lists
            self.data_x.append(accel_x)
            self.data_y.append(accel_y)
            self.data_z.append(accel_z)
            
            # Keep only the latest 100 values
            if len(self.data_x) > 100:
                self.data_x.pop(0)
                self.data_y.pop(0)
                self.data_z.pop(0)
        except Exception as e:
            self.status_label.setText(f"Error parsing data: {str(e)}")

    def update_plot(self):
        """Update the plot with the latest data"""
        if self.ser and self.ser.is_open:
            # Clear previous plot
            self.ax.clear()
            
            # Plot the latest X, Y, and Z acceleration values
            self.ax.plot(self.data_x, label='X Acceleration', color='r')
            self.ax.plot(self.data_y, label='Y Acceleration', color='g')
            self.ax.plot(self.data_z, label='Z Acceleration', color='b')
            
            # Set titles and labels
            self.ax.set_title('X, Y, Z Acceleration')
            self.ax.set_xlabel('Time (s)')
            self.ax.set_ylabel('Acceleration (g)')
            self.ax.legend()
            
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
