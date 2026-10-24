# PC Gauge Cluster

An Arduino-powered, automotive-style PC gauge cluster that displays real-time CPU, RAM, and GPU usage using analog gauges driven by SG90 micro servos.

## Features

- Real-time CPU usage monitoring
- RAM utilization monitoring
- GPU utilization monitoring
- Three independent analog-style gauges
- Arduino and Python serial communication
- Smooth gauge movement using software filtering
- 3D-printable components

## Hardware

- Arduino Uno
- 3 × Tower Pro SG90 9g positional micro servos
- External regulated 5V power supply for the servos
- USB cable for Arduino communication
- 3D-printed gauge enclosure and components
- Jumper wires

## Software Requirements

- Windows PC
- Arduino IDE
- Python 3
- Python packages required by the monitoring script

The monitoring software uses Python to collect system information and sends the readings to the Arduino over a serial connection.

## Wiring

| Component | Arduino Pin |
|---|---|
| CPU gauge signal | D5 |
| RAM gauge signal | D6 |
| GPU gauge signal | D7 |

**Power notes:**

- Power the servos from a suitable external regulated 5V supply.
- Connect the external power supply's GND to Arduino GND.
- Do not power all three servos from the Arduino's 5V pin.
- Check the wiring and servo orientation before installing the gauges.

## How It Works

1. The Python script reads CPU and RAM utilization and obtains GPU utilization using the configured monitoring method.
2. The readings are sent to the Arduino through USB serial communication.
3. The Arduino processes the incoming values and maps them to servo positions.
4. The servos move the gauge needles to indicate the current system usage.

The serial protocol and monitoring method must match the versions of the Arduino and Python scripts included in the repository.

## Installation

### 1. Download the Project

Clone the repository or download it as a ZIP file.

```bash
git clone https://github.com/YOUR_USERNAME/pc-gauge-cluster.git
cd pc-gauge-cluster
```

Replace `YOUR_USERNAME` with your GitHub username.

### 2. Install Python Dependencies

Install the packages listed in `requirements.txt`, if provided.

```bash
python -m pip install -r requirements.txt
```

### 3. Upload the Arduino Code

1. Open the Arduino sketch in Arduino IDE.
2. Select **Arduino Uno** as the board.
3. Select the correct serial port.
4. Upload the sketch.

### 4. Run the Monitoring Script

Connect the Arduino to the PC and run the Python monitoring script.

```bash
python monitor.py
```

The script filename and configuration must match the files included in the repository. Close the Arduino Serial Monitor before starting the Python script if both programs would otherwise use the same serial port.

## 3D Printing

The printable parts are available on the project's Printables page.

Suggested file names:

- `Cover.stl`
- `Base.stl`
- `Dial.stl`
- `Servo_Mount.stl`

Print settings, assembly details, and mechanical calibration instructions can be added as the design is finalized.

## Project Status

This project is under development. Software, wiring, and mechanical components may change as the design is tested and improved.

## License

The software license and the 3D model license may differ. Refer to the license files and the Printables project page for the applicable terms.

## Acknowledgments

Built as a DIY electronics and PC hardware project using Arduino, Python, SG90 micro servos, and 3D printing.
