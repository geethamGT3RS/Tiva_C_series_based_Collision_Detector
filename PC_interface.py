import sys
import serial
import serial.tools.list_ports
import threading
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QPushButton, QComboBox, QVBoxLayout, QWidget, QTextEdit
from PyQt5.QtCore import QTimer
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas

class SerialPlotter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Serial Data Plotter")
        self.setGeometry(100, 100, 800, 600)
        
        # Serial port settings
        self.ser = None
        self.data_x = []
        self.data_y = []
        self.data_z = []
        
        # Set up UI
        self.init_ui()
        
        # Start timer for updating plot
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(1000)  # Update plot every second
        
    def init_ui(self):
        # Create layout
        layout = QVBoxLayout()
        
        # Dropdown for serial ports
        self.port_label = QLabel("Select Serial Port:")
        layout.addWidget(self.port_label)
        
        self.port_combobox = QComboBox()
        self.populate_ports()
        layout.addWidget(self.port_combobox)
        
        # Connect button
        self.connect_button = QPushButton("Connect")
        self.connect_button.clicked.connect(self.on_connect_button_click)
        layout.addWidget(self.connect_button)
        
        # Status label
        self.status_label = QLabel("Status: Not connected")
        layout.addWidget(self.status_label)
        
        # Data display area
        self.data_display = QTextEdit(self)
        self.data_display.setReadOnly(True)
        layout.addWidget(self.data_display)
        
        # Plot setup
        self.figure, self.ax = plt.subplots(figsize=(6, 6), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        layout.addWidget(self.canvas)
        
        # Create a container for the layout
        container = QWidget()
        container.setLayout(layout)
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
            # Start reading data in a separate thread
            threading.Thread(target=self.read_data, args=(self.ser,), daemon=True).start()
        except serial.SerialException as e:
            self.status_label.setText(f"Could not connect to {selected_port}: {str(e)}")
    
    def read_data(self, ser):
        """Continuously reads data from the serial port"""
        try:
            while True:
                if ser.is_open:
                    data = ser.readline().decode('utf-8').strip()
                    if data:
                        self.data_display.append(f"Received data: {data}")
                        # Parse the incoming data
                        self.parse_data(data)
        except serial.SerialException:
            self.status_label.setText("Lost connection to the serial device.")
            ser.close()
    
    def parse_data(self, data):
        """Parse the incoming data and extract X, Y, Z values"""
        try:
            parts = data.split()
            accel_x = int(parts[1].strip(':'))
            accel_y = int(parts[3].strip(':'))
            accel_z = int(parts[5].strip(':'))
            
            # Add the new data to the lists (for plotting)
            self.data_x.append(accel_x)
            self.data_y.append(accel_y)
            self.data_z.append(accel_z)
            
            # Limit the data size to 100 points for each axis
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
            
            # Plot X, Y, and Z acceleration values on the same graph
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
