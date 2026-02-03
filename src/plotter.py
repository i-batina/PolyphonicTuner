import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import sys

# --- CONFIGURATION ---
SERIAL_PORT = 'COM3'  # Windows: 'COM3', Mac: '/dev/tty.usbmodem...'
BAUD_RATE = 2000000   # Must match Teensy code
BUFFER_SIZE = 500     # Number of points to show on screen

# --- SETUP SERIAL ---
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT}")
except Exception as e:
    print(f"Error connecting to serial: {e}")
    sys.exit(1)

# --- SETUP PLOT ---
data_buffer = deque([0] * BUFFER_SIZE, maxlen=BUFFER_SIZE)

fig, ax = plt.subplots()
line, = ax.plot(data_buffer)

# Set static Y-axis limits (10k to 20k for 16-bit audio)
ax.set_ylim(10000, 20000)
ax.set_title("Real-Time ADC Waveform")
ax.grid(True)

def update(frame):
    # Read all available data from buffer to avoid lag
    while ser.in_waiting:
        try:
            line_data = ser.readline().decode('utf-8').strip()
            if line_data:
                value = int(line_data)
                data_buffer.append(value)
        except ValueError:
            pass  # Ignore bad packets
    
    line.set_ydata(data_buffer)
    return line,

# Update every 20ms (50fps)
ani = FuncAnimation(fig, update, interval=20, blit=True)

plt.show()
ser.close()
