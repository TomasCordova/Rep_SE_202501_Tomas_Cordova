import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import re

# Configuración serial
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200

# Buffer para graficar
MAX_POINTS = 300
ecg_data = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)
press_data = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)
cpm_data = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)

# Inicializar serial
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
    print(f"[INFO] Conectado a {SERIAL_PORT} a {BAUD_RATE} bps")
except serial.SerialException as e:
    print(f"[ERROR] No se pudo abrir {SERIAL_PORT}: {e}")
    exit(1)

# Parsear línea serial
def parse_line(line):
    try:
        match = re.match(r"ECG:(\d+),PRESS:(\d+),CPM:([\d\.]+)", line)
        if match:
            ecg = int(match.group(1))
            press = int(match.group(2))
            cpm = float(match.group(3))
            return ecg, press, cpm
    except:
        pass
    return None

# Actualizar gráfica
def update(frame):
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    parsed = parse_line(line)
    if parsed:
        ecg, press, cpm = parsed
        ecg_data.append(ecg)
        press_data.append(press)
        cpm_data.append(cpm)

        ax1.clear()
        ax2.clear()
        ax3.clear()

        ax1.plot(ecg_data, label='ECG', color='blue')
        ax2.plot(press_data, label='Presión', color='green')
        ax3.plot(cpm_data, label='CPM', color='red')

        ax1.set_title("Señal ECG")
        ax2.set_title("Presión (Sensor)")
        ax3.set_title("CPM (Compresiones por minuto)")

        # Limitar ejes para evitar zoom automático
        ax1.set_ylim(0, 3500)   # rango típico ADC 12-bit
        ax2.set_ylim(0, 3)    # rango sensor presión
        ax3.set_ylim(0, 200)    # CPM máximo esperado

        # Líneas guía para CPM aceptable
        ax3.axhline(100, color='gray', linestyle='--', label='CPM límite inferior')
        ax3.axhline(120, color='gray', linestyle='--', label='CPM límite superior')

        for ax in (ax1, ax2, ax3):
            ax.legend(loc='upper right')

# Configurar plot
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))
ani = animation.FuncAnimation(fig, update, interval=50)

plt.tight_layout()
plt.show()
