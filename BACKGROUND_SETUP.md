# PC Gauge Cluster — Background Setup

This guide explains how to install the PC Gauge Cluster background monitor on Windows.

## What It Does

- Reads CPU, RAM, and GPU 3D usage and sends the values to the Arduino.
- Runs with `pythonw.exe`, so it does not open a console window.
- Starts automatically when you sign in to Windows.
- Searches for the Arduino again every 10 seconds if it is not detected.
- Attempts to reconnect 3 seconds after a connection error or a missed heartbeat.
- Writes events and errors to `%LOCALAPPDATA%\KadranArkaPlan\kadran.log`.
- Uses the existing serial protocol: `KADRAN?` / `KADRAN_OK` for the handshake and `CPU,RAM,GPU` for telemetry. The Arduino sketch does not need to be changed if it already supports this protocol.

## Requirements

- Windows
- Python 3 installed
- Python packages: `psutil` and `pyserial`
- The Arduino connected through a Windows COM port
- An Arduino sketch that supports the serial protocol described above

## Installation

1. Install Python 3 if it is not already installed. During installation, enable **Add Python to PATH**.
2. Extract the project files into a folder.
3. Make sure these files are together in the same folder:
   - `background_monitor.py`
   - `install_background.ps1`
4. Open PowerShell in that folder.
5. Run these commands:

   ```powershell
   Set-ExecutionPolicy -Scope Process Bypass
   .\install_background.ps1
   ```

6. Follow any prompts shown by the installer.
7. Once installation finishes, sign out and sign back in to Windows, or launch the shortcut created by the installer.

> Run the installer from the folder containing both files. The installer must be able to find `background_monitor.py`.

## Important Notes

- Make sure the Arduino's USB/serial or Bluetooth serial COM port is available in Windows.
- For Bluetooth, the module must already be paired and provide a COM port that supports 9600-baud serial communication. These project files do not perform Bluetooth pairing.
- The monitor runs in your Windows user session. It is **not** a Windows service and does not run before you sign in.
- Because it runs without a console window, check the log file if it does not appear to work.

## Log File

The background monitor writes its log to:

```text
%LOCALAPPDATA%\KadranArkaPlan\kadran.log
```

To open the folder, press `Win + R`, enter:

```text
%LOCALAPPDATA%\KadranArkaPlan
```

and press Enter.

## Troubleshooting

### The gauges do not move

- Check that the Arduino is powered and connected.
- Confirm that the correct Arduino sketch is running.
- Make sure no other application is using the same COM port.
- Open `kadran.log` and look for serial-port or handshake errors.

### The Arduino is not detected

- Check Windows Device Manager for the Arduino's COM port.
- Disconnect and reconnect the Arduino, then allow the monitor time to retry.
- If using Bluetooth, confirm that Windows has created a serial COM port.

### The GPU gauge stays at 0 or keeps its previous value

The monitor reads Windows GPU Engine 3D performance counters. If those counters are unavailable or return no valid data, CPU and RAM reporting may continue while the GPU gauge remains at 0 or at its last valid reading.

### Where are the logs?

Check:

```text
%LOCALAPPDATA%\KadranArkaPlan\kadran.log
```

## Uninstallation

1. Open **Task Manager** and select the **Details** tab.
2. Find the `pythonw.exe` process running `background_monitor.py` and end that process. Be careful not to stop unrelated Python applications.
3. Press `Win + R`, enter `shell:startup`, and press Enter.
4. Remove the PC Gauge Cluster startup shortcut created by the installer. The exact shortcut name depends on the installer script.
5. If you no longer need the logs, you can delete `%LOCALAPPDATA%\KadranArkaPlan`.

## Files

- `background_monitor.py` — background CPU/RAM/GPU monitor.
- `install_background.ps1` — Windows startup installation script.
- `monitor.py` — console-based monitor for manual testing.
