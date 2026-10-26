#include <Servo.h>
#include <stdlib.h>
#include <string.h>

// Three servos: CPU usage on D5, RAM usage on D6, GPU usage on D7.
Servo servos[3];
const byte SERVO_PINS[3] = {5, 6, 7};

// Data order: CPU usage, RAM usage, GPU usage.
const byte CPU_USAGE = 0;
const byte RAM_USAGE = 1;
const byte GPU_USAGE = 2;

// Serial line buffer.
char inputLine[32];
byte inputLength = 0;
bool handshakeVerified = false;

// Latest values from each valid serial packet.
int rawValues[3] = {0, 0, 0};
float filteredValues[3] = {0.0, 0.0, 0.0};
bool newDataAvailable = false;
bool firstMeasurement = true;

// State of each gauge servo.
float currentAngles[3] = {160.0, 160.0, 160.0};
float targetAngles[3] = {160.0, 160.0, 160.0};
int lastStep[3] = {-1, -1, -1};
int lastServoCommand[3] = {-1, -1, -1};

// Timing.
unsigned long previousMovementTime = 0;
const unsigned long MOVEMENT_INTERVAL = 60;  // Approximately 16.7 degrees per second.

// One fixed filter coefficient for each new serial packet.
// The filter is updated only when new data arrives, not repeatedly using the same value.
const float FILTER_COEFFICIENT = 0.20;

// Angle limits and deadband.
const int ANGLE_MIN = 20;
const int ANGLE_MAX = 160;
const float ANGLE_DEADBAND = 2.0;

// Display ranges: CPU, RAM, and GPU values are all 0-100%.
const int VALUE_MIN[3] = {0, 0, 0};
const int VALUE_MAX[3] = {100, 100, 100};

void processLine() {
  inputLine[inputLength] = '\0';

  if (inputLength == 0) return;

  if (strcmp(inputLine, "KADRAN?") == 0) {
    handshakeVerified = true;
    Serial.println("KADRAN_OK");
    return;
  }

  if (!handshakeVerified) return;

  // Expected format: cpu_usage,ram_usage,gpu_usage
  int values[3];
  byte fieldCount = 0;
  char *token = strtok(inputLine, ",");

  while (token != NULL && fieldCount < 3) {
    // Validate that each field contains digits only.
    if (*token == '\0') return;

    for (byte i = 0; token[i] != '\0'; i++) {
      if (token[i] < '0' || token[i] > '9') return;
    }

    values[fieldCount] = atoi(token);
    fieldCount++;
    token = strtok(NULL, ",");
  }

  // Exactly three fields must be present.
  if (fieldCount != 3 || token != NULL) return;

  // Validate the range of each measurement.
  if (values[CPU_USAGE] < 0 || values[CPU_USAGE] > 100) return;
  if (values[RAM_USAGE] < 0 || values[RAM_USAGE] > 100) return;
  if (values[GPU_USAGE] < 0 || values[GPU_USAGE] > 100) return;

  // Store the raw values for all three gauges.
  // The filter, percentage steps, and servo movement limit help smooth brief spikes.
  for (byte i = 0; i < 3; i++) {
    rawValues[i] = values[i];
  }

  newDataAvailable = true;
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < 3; i++) {
    servos[i].attach(SERVO_PINS[i]);
    servos[i].write(ANGLE_MAX);
  }
}

void loop() {
  // Read serial data line by line.
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      processLine();
      inputLength = 0;
    } else if (inputLength < sizeof(inputLine) - 1) {
      inputLine[inputLength++] = c;
    } else {
      // Discard an overlong line.
      inputLength = 0;
    }
  }

  if (!handshakeVerified) return;

  // Update the filter only when a new measurement packet arrives.
  // This prevents the same raw value from being filtered repeatedly every 50 ms.
  if (newDataAvailable) {
    newDataAvailable = false;

    for (byte i = 0; i < 3; i++) {
      // Move toward the actual measurement on the first packet; smooth later packets.
      if (firstMeasurement) {
        filteredValues[i] = rawValues[i];
      } else {
        // Apply the same fixed filter coefficient to CPU, RAM, and GPU.
        filteredValues[i] +=
            (rawValues[i] - filteredValues[i]) * FILTER_COEFFICIENT;
      }

      filteredValues[i] = constrain(
          filteredValues[i], VALUE_MIN[i], VALUE_MAX[i]);

      // Convert the percentage to an angle using 5% steps.
      // Each step sets a new target angle.
      float percentage = filteredValues[i];
      if (percentage < 0.0) percentage = 0.0;
      if (percentage > 100.0) percentage = 100.0;

      int step = (int)(percentage / 5.0);
      if (step > 20) step = 20;

      if (step != lastStep[i]) {
        targetAngles[i] = map(step, 0, 20, ANGLE_MAX, ANGLE_MIN);
        lastStep[i] = step;
      }
    }

    firstMeasurement = false;
  }

  // Move each servo gradually, maintaining its independent state.
  if (millis() - previousMovementTime >= MOVEMENT_INTERVAL) {
    previousMovementTime = millis();

    for (byte i = 0; i < 3; i++) {
      float difference = targetAngles[i] - currentAngles[i];

      if (difference >= ANGLE_DEADBAND) {
        currentAngles[i] += 1.0;
      } else if (difference <= -ANGLE_DEADBAND) {
        currentAngles[i] -= 1.0;
      } else {
        currentAngles[i] = targetAngles[i];
      }

      currentAngles[i] = constrain(currentAngles[i], ANGLE_MIN, ANGLE_MAX);
      int command = (int)(currentAngles[i] + 0.5);

      if (command != lastServoCommand[i]) {
        servos[i].write(command);
        lastServoCommand[i] = command;
      }
    }
  }
}
