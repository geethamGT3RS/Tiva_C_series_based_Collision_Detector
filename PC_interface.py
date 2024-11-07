import serial
import serial.tools.list_ports

def list_ports():
    """Lists all available COM ports"""
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

def connect(selected_port):
    """Connects to the selected serial port"""
    baud_rate = 9600  # Set your baud rate here
    try:
        ser = serial.Serial(selected_port, baud_rate, timeout=1)
        read_data(ser)  # Start reading data
    except serial.SerialException as e:
        print(f"Could not connect to {selected_port}\n{str(e)}")

def read_data(ser):
    """Continuously reads data from the serial port"""
    try:
        while True:
            if ser.is_open:
                data = ser.readline().decode('utf-8').strip()
                if data:
                    print(f"Received data: {data}")
    except serial.SerialException:
        print("Lost connection to the serial device.")
        ser.close()

# List available ports
ports = list_ports()
if not ports:
    print("No serial ports found.")
else:
    print("Available ports:")
    for i, port in enumerate(ports, 1):
        print(f"{i}. {port}")

    # Ask user to select a port
    try:
        port_index = int(input("Select a port number: ")) - 1
        if 0 <= port_index < len(ports):
            selected_port = ports[port_index]
            connect(selected_port)
        else:
            print("Invalid selection.")
    except ValueError:
        print("Invalid input.")
