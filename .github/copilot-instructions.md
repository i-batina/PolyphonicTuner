# Polyphonic Tuner - AI Coding Agent Instructions

## Project Overview
PolyphonicTuner is an embedded Arduino project (Teensy 4.1) that automatically tunes a 6-string guitar using stepper motors. The system reads audio pitch frequencies and adjusts motor speeds via a closed-loop P-controller to achieve target tuning frequencies.

## Architecture

### Core Components
1. **StringInput** (`src/StringInput.h`) - Pitch frequency sensor abstraction
   - Currently in simulation mode with random jitter (no hardware implementation yet)
   - `simulatePitch(hertz)` - Inject test frequencies without hardware
   - `getPitch()` - Reads current frequency (or simulated with ±10 centroid noise)
   - `isActive()` - Returns true if pitch > 20 Hz (string is being played)
   - **Future:** Replace with `AudioAnalyzeNoteFrequency` from Teensy Audio Library

2. **TunerLogic** (`src/TunerLogic.h`) - P-controller for motor speed calculation
   - `calculateMotorSpeed(currentFreq)` returns normalized speed: -1.0 (full reverse) to +1.0 (full forward)
   - **Control logic:** `motorOutput = (targetFreq - currentFreq) × kP`, clamped to ±1.0
   - **Deadband:** Stops motor if |error| < 0.3 Hz (tolerance)
   - **Silence filter:** Returns 0.0 if currentFreq < 20 Hz
   - **Tuning constant:** `kP = 0.5` (in-code; adjust if overshoot or sluggish response)

3. **main.cpp** - Hardware driver and Serial command handler
   - Motor abstraction: `setMotorSpeed(motorID, speed)` with -255 to +255 PWM clamping
   - Pin mapping: Motor N → `PIN_MN_PWM` (speed) + `PIN_MN_DIR` (direction)
   - Currently configured for 2 motors; expandable to 6 with additional pin pairs
   - **Serial protocol:** 2,000,000 baud; message format `"M <motorID> <speed>\n"` (newline-terminated)
   - Non-blocking loop: reads Serial, parses command, drives motor, repeats

### Data Flow
```
External input (Serial or local simulation)
  ↓
StringInput::getPitch() / simulatePitch()
  ↓
TunerLogic::calculateMotorSpeed(currentFreq)
  ↓
main.cpp::setMotorSpeed(motorID, motorOutput × 255)
  ↓
Motor PWM pin (speed) + DIR pin (direction)
  ↓
Physical motor adjusts string tension
```

## Developer Workflows

### Build & Upload
```bash
platformio run --target upload
# Compiles and uploads firmware to Teensy 4.1 via USB
```

### Testing Without Hardware
1. Instantiate `StringInput` and `TunerLogic` in a test sketch
2. Call `stringInput.simulatePitch(targetFreq)` to simulate string pitch
3. Call `tunerLogic.calculateMotorSpeed(simulatedFreq)` and verify output
4. Example: Target A = 110 Hz; simulate 115 Hz → expect negative motorOutput (tune down)

### Monitoring & Debugging
- Serial.println() is safe; loop runs fast enough to not block
- No breakpoint debugger available (typical for embedded)
- Use Serial Monitor at 2,000,000 baud; **note:** platformio.ini monitor_speed is separate (9600)
- Test edge cases: frequencies exactly at ±0.3 Hz boundary, silence (<20 Hz), extreme frequencies

## Key Conventions

### Motor Control Mapping
- **Speed value:** -255 (full reverse) to +255 (full forward)
- **Motor direction:** Set DIR pin HIGH for forward (increasing tension), LOW for reverse (decreasing tension)
- **PWM formula:** `analogWrite(PWM_pin, abs(speed))`; sign of speed determines DIR pin state

### Pin Pattern (Expandable)
```
Motor 1: PWM=2,  DIR=3   (Low E)
Motor 2: PWM=4,  DIR=5   (A)
Motor 3: PWM=6,  DIR=7   (D)   [add these for complete 6-string]
Motor 4: PWM=8,  DIR=9   (G)
Motor 5: PWM=10, DIR=11  (B)
Motor 6: PWM=12, DIR=13  (High E)
```

### Frequency Constants (Standard Guitar Tuning)
| String | Target Hz | Tolerance |
|--------|-----------|-----------|
| Low E  | 82.41     | ±0.3      |
| A      | 110.00    | ±0.3      |
| D      | 146.83    | ±0.3      |
| G      | 196.00    | ±0.3      |
| B      | 246.94    | ±0.3      |
| High E | 329.63    | ±0.3      |

**Special thresholds:** Silence < 20 Hz (ignored); in-tune zone: ±0.3 Hz around target

### P-Controller Tuning
- **kP = 0.5** (moderate responsiveness; error of 5 Hz → output of 2.5, clamped to 1.0)
- **Adjust:** Increase kP if response is sluggish; decrease if overshooting target
- **Deadband:** 0.3 Hz prevents motor jitter near target

## Integration Points & Dependencies

### Hardware
- **Teensy 4.1** (no alternatives tested; ARM Cortex-M7)
- **PWM pins:** 2–13 (16-bit PWM capable)
- **Serial:** Native USB (pins 0-1 reserved by framework)

### Software Stack
- **Arduino Framework** (via PlatformIO)
- **PlatformIO IDE** (VSCode extension) for build/upload
- **Future:** Teensy Audio Library (`AudioAnalyzeNoteFrequency`) for real hardware audio input

### Serial Communication
- **Baud rate:** 2,000,000 (NOT the 9600 monitor_speed in platformio.ini)
- **Protocol:** ASCII plaintext; `setMotorSpeed(motorID, speed)` in main.cpp calls `Serial.println()` (can be uncommented for ACK)
- **Latency:** ~5μs per byte at 2M baud; non-blocking design ensures real-time responsiveness

## Common Task Patterns

### Adding a 3rd Motor
1. Define `PIN_M3_PWM = 6` and `PIN_M3_DIR = 7` at top of main.cpp
2. Add case to `setMotorSpeed()` switch statement
3. Create `TunerLogic tuner3(2, 146.83)` in setup() for D-string

### Implementing Real Audio Input
1. Include Teensy Audio Library: `#include <Audio.h>`
2. Replace `StringInput::getPitch()` to use `AudioAnalyzeNoteFrequency`
3. Handle probability threshold (~0.8) in `isActive()` instead of 20 Hz silence check

### Testing P-Controller Response
- Set `simulatePitch()` to target + 5 Hz, observe motorOutput
- Vary kP (0.2, 0.5, 1.0) and observe settling time vs. overshoot
- Log motorOutput to Serial to plot tuning curve

## Files to Reference
- [platformio.ini](platformio.ini) — Board & build config (Teensy 4.1, framework settings)
- [src/main.cpp](src/main.cpp) — Motor driver, Serial parser, setup/loop
- [src/TunerLogic.h](src/TunerLogic.h) — P-controller implementation
- [src/StringInput.h](src/StringInput.h) — Pitch sensor abstraction layer77
