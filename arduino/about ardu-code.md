# PC Gauge Cluster

An Arduino-based physical gauge cluster that displays **CPU, RAM, and GPU utilization** using three servo-driven needles.

The Arduino receives utilization percentages over Serial, smooths incoming measurements, converts them into gauge positions, and moves the needles gradually. The goal is to provide a simple, at-a-glance indication of overall system load rather than a perfectly instantaneous or laboratory-accurate measurement.

## Features

- Three independent analog-style gauges: CPU, RAM, and GPU.
- Serial communication at **9600 baud**.
- Handshake before measurement packets are accepted.
- Fixed exponential smoothing coefficient to reduce brief fluctuations.
- Gauge targets updated in **5% increments**.
- Gradual servo movement to avoid sudden needle jumps.
- Input validation for packet format and percentage ranges.

## Hardware

| Component | Connection / requirement |
|---|---|
| Arduino Uno (or compatible board) | Runs the gauge firmware |
| CPU gauge servo | Digital pin **D5** |
| RAM gauge servo | Digital pin **D6** |
| GPU gauge servo | Digital pin **D7** |
| Servo power supply | Use a suitable external supply if required by the servos |
| Ground | Connect external supply ground and Arduino ground together |

**Power note:** Do not assume the Arduino's 5 V pin can safely power multiple servos. Servo startup or stall current can reset the board or cause unstable behavior. Use a suitable regulated supply and share ground with the Arduino.

## How It Works

### 1. Serial handshake

The Arduino starts Serial communication at `9600` baud. Before it accepts measurements, the sender must transmit:

```text
KADRAN?
```

The Arduino replies:

```text
KADRAN_OK
```

Only after this handshake has been verified will the Arduino process measurement packets. The handshake strings are kept as-is for compatibility with the existing PC-side sender.

### 2. Measurement packet

Each measurement packet must contain exactly three comma-separated, non-negative integer percentages in this order:

```text
cpu_usage,ram_usage,gpu_usage
```

Example:

```text
42,67,31
```

The values represent CPU usage = 42%, RAM usage = 67%, and GPU usage = 31%.

Each field must contain digits only, and every value must be in the range `0–100`. Packets with missing fields, extra fields, invalid characters, or out-of-range values are rejected. Each packet should end with a newline (`\n` or `\r\n`).

### 3. Measurement smoothing

The firmware uses a fixed exponential moving average (EMA) coefficient:

```cpp
const float FILTER_COEFFICIENT = 0.20;
```

For each new packet after the first measurement:

```text
filtered = filtered + (raw - filtered) * 0.20
```

This makes the displayed value move gradually toward the latest measurement instead of following every short-lived spike. The filter is updated **only when a new valid packet arrives**; the same raw reading is not repeatedly processed in the main loop.

The first valid measurement initializes the filtered values directly, avoiding an unnecessary slow ramp from zero.

### 4. Percentage-to-angle mapping

Each filtered percentage is converted into one of 21 target positions, representing `0%, 5%, 10%, ... 100%`.

The firmware uses the following angle range:

| Utilization | Target angle |
|---:|---:|
| 0% | 160° |
| 50% | 90° |
| 100% | 20° |

The angle decreases as utilization rises, so the needle moves in the opposite angular direction to the percentage. The actual visual direction depends on how the servo and gauge face are mounted.

The target angle changes only when the filtered reading crosses into a different 5% step. This intentionally trades fine-grained responsiveness for a steadier, easier-to-read needle.

### 5. Gradual needle movement

The firmware updates servo movement every **60 ms**:

```cpp
const unsigned long MOVEMENT_INTERVAL = 60;
```

At each movement update, a servo moves approximately **1 degree** toward its target. This corresponds to a maximum commanded movement rate of roughly **16.7 degrees per second**. Each gauge keeps its own current angle and target angle, so the three needles can move independently.

A `2°` deadband prevents unnecessary small corrections near the target. Servo commands are sent only when the rounded commanded angle changes.

## Firmware Configuration

The main settings are defined near the top of `pc_gauge_cluster_english.ino`:

| Setting | Default | Purpose |
|---|---:|---|
| `SERVO_PINS` | `{5, 6, 7}` | CPU, RAM, and GPU servo pins |
| `FILTER_COEFFICIENT` | `0.20` | EMA smoothing strength |
| `MOVEMENT_INTERVAL` | `60 ms` | Delay between needle movement updates |
| `ANGLE_MIN` | `20°` | Angle used at 100% utilization |
| `ANGLE_MAX` | `160°` | Angle used at 0% utilization |
| `ANGLE_DEADBAND` | `2.0°` | Small-error threshold near the target |
| `VALUE_MIN` / `VALUE_MAX` | `0` / `100` | Valid utilization range |

A larger EMA coefficient follows new measurements more quickly; a smaller coefficient smooths more strongly. Changing the movement interval changes the needle's maximum movement rate. If you adjust these values, test the complete needle travel carefully to avoid mechanical binding.

## Setup

1. Connect the CPU, RAM, and GPU servos to pins D5, D6, and D7 respectively.
2. Provide suitable servo power and connect the power supply ground to Arduino ground.
3. Open `pc_gauge_cluster_english.ino` in the Arduino IDE.
4. Select the correct board and port.
5. Upload the firmware.
6. Start the PC-side sender using **9600 baud**.
7. Confirm that the sender performs the `KADRAN?` / `KADRAN_OK` handshake before sending measurement packets.

## Troubleshooting

- **Needles do not move:** Check servo power, common ground, pin assignments, and whether the sender completes the handshake.
- **Arduino resets when servos move:** The servos may be drawing more current than the Arduino can provide. Use a suitable external servo supply.
- **No readings are accepted:** Confirm that packets use the exact order `CPU,RAM,GPU`, contain exactly three integer fields from 0 to 100, and end with a newline.
- **Needles move in the wrong direction:** Check the servo mounting and gauge face. The firmware maps 0% to 160° and 100% to 20°.
- **Needles still react too quickly or too slowly:** Adjust `FILTER_COEFFICIENT` for measurement smoothing and `MOVEMENT_INTERVAL` for physical movement speed. These control different parts of the behavior.

## Design Intent

This project is intended as a **visual system-load indicator**, not a precision instrument. The filter, 5% target steps, and gradual servo movement prioritize readability and a calmer display during ordinary PC use. The displayed values naturally lag behind rapid changes because smoothing and physical needle movement take time.

