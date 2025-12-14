# Airsoft Bomb Timer

A countdown timer for airsoft games built for M5Stack Core.

## Features

- **Timer Setup**: Set timer up to 24 hours (hours, minutes, seconds)
- **Countdown**: Visual countdown display
- **Disarm Function**: Hold Button C for 10 seconds to disarm (countdown pauses while holding)
- **BOOM State**: Red flashing screen when timer reaches zero
- **State Management**: Clean state machine for all game states

## Controls

### Setup Mode
- **Button A**: Cycle through hours/minutes/seconds selection
- **Button B**: Increment selected value (hold to decrement)
- **Button C**: Start countdown

### Countdown Mode
- **Button C (Hold)**: Disarm button - hold for 10 seconds to disarm
  - Countdown pauses while button is held
  - If released before 10 seconds, countdown continues
  - If held for full 10 seconds, bomb is disarmed

### Disarmed/BOOM States
- **Any Button**: Return to setup menu

## States

1. **SETUP**: Configure timer (hours, minutes, seconds)
2. **COUNTDOWN**: Timer is running
3. **DISARMING**: Disarm button is being held (countdown paused)
4. **DISARMED**: Successfully disarmed
5. **BOOM**: Timer reached zero (red flashing screen)

## Hardware

- M5Stack Core (ESP32-based development board)

## Building and Uploading

1. Connect your M5Stack Core via USB
2. Build and upload:
   ```bash
   python -m platformio run --target upload
   ```
3. Monitor serial output (optional):
   ```bash
   python -m platformio device monitor
   ```

## Usage

1. Power on the M5Stack Core
2. Use buttons to set your desired timer
3. Press Button C to start the countdown
4. During countdown, hold Button C for 10 seconds to disarm
5. If timer reaches zero, screen flashes red with "BOOM!"
6. Press any button after BOOM or DISARMED to return to setup

