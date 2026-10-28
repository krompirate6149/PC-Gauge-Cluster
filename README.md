# PC Gauge Cluster

An Arduino-powered, automotive-style PC gauge cluster that displays real-time CPU, RAM, and GPU utilization using three SG90 micro servos.

## Features

- Real-time CPU utilization monitoring
- RAM utilization monitoring
- GPU utilization monitoring
- Three independent analog-style gauges
- USB serial communication between Python and Arduino
- Smooth needle movement using software filtering
- 3D-printable enclosure and components
- Optional background operation on Windows

## 1. Hardware Requirements

- 1 × Arduino Nano
- 3 × Tower Pro SG90 9g positional micro servos
- 1 × regulated 5V external power supply suitable for the servos
- USB cable for Arduino communication
- 3 x red led
- 3 x 330 Ω resistor
- Jumper wires
- 3D-printed gauge components
- Windows PC

## 2. Arduino Wiring

Each servo has three wires:

- **Signal:** usually orange, yellow, or white
- **VCC:** usually red
- **GND:** usually brown or black

Wire the three servos as follows:

| Gauge | Servo Signal | Servo VCC | Servo GND |
|---|---|---|---|
| CPU | Arduino D5 | External +5V | External GND |
| RAM | Arduino D6 | External +5V | External GND |
| GPU | Arduino D7 | External +5V | External GND |
| GPU_LED | Arduino D4 |
| RAM_LED | Arduino D3 |
| CPU_LED | Arduino D2 |

### Important Power Connections

1. Connect all three servo signal wires to their assigned Arduino pins.
2. Connect all servo red wires to the positive terminal of the regulated 5V supply.
3. Connect all servo ground wires to the supply's GND.
4. Connect the external supply's GND to an Arduino GND pin.
5. Connect the leds arduino with resistors
6. Connect the Arduino to the PC using USB.

**Do not power all three servos from the Arduino's 5V pin.** Servo startup and movement can cause voltage drops and reset the Arduino. Ensure the external supply can handle the combined current demand.

Verify the wire colors and connector polarity for your particular servos before powering the circuit.

## 3. Software Requirements

Install the following software on Windows:

- Arduino IDE
- Python 3
- The Python packages required by the monitoring script

The Python script collects system utilization data and sends it to the Arduino over a serial connection.

The Arduino sketch processes incoming readings and controls the three servo positions.

## 4. Repository Structure

A recommended repository layout is:

```text
pc-gauge-cluster/
├── arduino/
│   └── pc_gauge_cluster.ino
├── python/
│   ├── monitor.py
│   └── requirements.txt
├── models/
│   ├── Cover.stl
│   ├── Base.stl
│   ├── Dial.stl
│   └── Servo_Mount.stl
├── README.md
└── LICENSE
```

The filenames above are examples. Adjust them to match the files actually uploaded to the repository.

## 5. Install and Upload the Arduino Code

1. Connect the Arduino Uno to your PC using USB.
2. Open Arduino IDE.
3. Open the Arduino sketch from the `arduino` folder.
4. Install any libraries required by the sketch, such as the Servo library.
5. Select **Tools → Board → Arduino AVR Boards → Arduino Uno**.
6. Select the correct port under **Tools → Port**.
7. Click **Upload**.
8. Wait for the upload to complete successfully.

Keep the Arduino connected to the PC. The USB connection is used for serial communication.

## 6. Install Python and Its Dependencies

Open PowerShell or Command Prompt in the repository directory.

Check that Python is installed:

```powershell
python --version
```

Navigate to the Python folder:

```powershell
cd python
```

Create a virtual environment:

```powershell
python -m venv .venv
```

Activate it:

```powershell
.\.venv\Scripts\Activate.ps1
```

If PowerShell blocks script activation, use Command Prompt and run:

```bat
.venv\Scripts\activate.bat
```

Install the dependencies:

```powershell
python -m pip install -r requirements.txt
```

The `requirements.txt` file must list the packages actually used by the monitoring script. For example, `psutil` may be needed for CPU and RAM monitoring, while GPU monitoring may require an additional tool or library.

## 7. Normal Operation

### Step 1: Check the Serial Port

Open Arduino IDE and check which COM port is assigned to the Arduino.

Close the Arduino Serial Monitor before starting the Python program, because another program may prevent the monitoring script from opening the same serial port.

### Step 2: Start the Python Script

With the virtual environment activated, run:

```powershell
python monitor.py
```

If the script is stored under a different filename, use that filename instead.

### Step 3: Verify the Gauges

- The CPU gauge should respond to CPU utilization.
- The RAM gauge should respond to memory utilization.
- The GPU gauge should respond to GPU utilization.

If a gauge does not move, check the wiring, external power supply, COM port, Python dependencies, and serial protocol.

The Arduino and Python scripts must use matching message formats and communication settings.

## 8. Run the Python Monitor in the Background

The Python monitoring script runs on the PC; it is not uploaded to the Arduino.

### Method A: Run Without a Console Window

Windows provides `pythonw.exe`, which runs Python programs without opening a standard console window.

First, test the script normally using `python monitor.py`. Confirm that the gauges work before switching to background operation.

Then, from PowerShell in the `python` folder, run:

```powershell
Start-Process -FilePath ".\.venv\Scripts\pythonw.exe" -ArgumentList "`"$((Get-Location).Path)\monitor.py`""
```

This starts the script without a console window.

**Note:** Errors and printed messages may not be visible when using `pythonw.exe`. For troubleshooting, return to normal operation with `python monitor.py`. A separate log file can also be added to the script for background diagnostics.

### Method B: Start Automatically When You Sign In

After confirming that background operation works:

1. Press `Win + R`.
2. Enter:

   ```text
   shell:startup
   ```

3. Create a Windows shortcut in the Startup folder.
4. Set the shortcut's target to the full path of the virtual environment's `pythonw.exe`, followed by the full path to `monitor.py`.

Example target:

```text
"C:\Path\To\pc-gauge-cluster\python\.venv\Scripts\pythonw.exe" "C:\Path\To\pc-gauge-cluster\python\monitor.py"
```

Replace the example paths with the actual paths on your computer.

5. Save the shortcut.
6. Sign out and sign in again to test automatic startup.

The Arduino must be connected, and the script must be able to identify and open the correct serial port. If the script requires a fixed COM port, verify that the port remains consistent.

## 9. Troubleshooting

| Problem | What to check |
|---|---|
| Arduino keeps resetting | External 5V supply, current capacity, common ground, and wiring |
| Servo does not move | Signal pin, servo power, ground, and sketch pin assignments |
| Python cannot connect | COM port, serial settings, and whether another program has opened the port |
| Gauge readings are incorrect | Monitoring method, message format, filtering, and angle calibration |
| GPU usage stays at zero | GPU monitoring method, driver support, and permissions |
| Background script does not work | Run normally first, verify paths, and inspect logs |
| Script starts twice | Check the Startup folder and close duplicate Python processes |

## 10. 3D-Printed Parts

Suggested STL filenames:

- `Cover.stl`
- `Base.stl`
- `Dial.stl`
- `Servo_Mount.stl`

Printing settings, dimensions, and assembly details should be documented alongside the final tested model files.

## Project Status

This is a DIY electronics and 3D-printing project. Hardware, software, and assembly instructions may evolve as testing continues.

## License

See the repository's `LICENSE` file for software licensing terms. The 3D models may have separate licensing terms on Printables.

## Acknowledgments

Built using Arduino, Python, Tower Pro SG90 micro servos, PC hardware monitoring, and 3D printing.
