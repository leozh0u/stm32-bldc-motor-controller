# STM32 Bare-Metal DC Motor Controller

Bare-metal firmware for an STM32F401RE (Nucleo-64) driving a brushed DC gear
motor with quadrature-encoder speed feedback, PID control, and live binary
telemetry to a Python plotting stack on the host.

No ST HAL or LL drivers — all peripheral access is direct register
manipulation written against the reference manuals (RM0368, PM0214, UM1724).
CMSIS core and device headers (register definitions, NVIC/SysTick intrinsics)
are the only abstraction layer used.

Started 2026-07-01. Status: firmware complete and compile-verified; encoder,
UART telemetry, and PID awaiting hardware bring-up (see
[Project status](#project-status)).

## Hardware

- Nucleo-F401RE (STM32F401RE, Cortex-M4 @ 16MHz HSI)
- JGB37-520 12V brushed DC gear motor with quadrature encoder
- TB6612FNG dual H-bridge driver
- Flash/debug via onboard ST-LINK (USB Mini-B); UART telemetry rides the
  ST-LINK virtual COM port — no extra cabling

### Bill of materials

| Part | Detail |
|------|--------|
| MCU board | Nucleo-F401RE (STM32F401RE) |
| Motor | JGB37-520, 12V, **gear ratio 56** (178 rpm no-load), AB Hall quadrature encoder on rear cover, 6-pin PH2.0 interface |
| Motor driver | TB6612FNG dual H-bridge module (headers pre-soldered) |
| Motor supply | 12V / 2A DC adapter → DC5.5×2.1 barrel-to-screw-terminal adapter → breadboard rail (motor stall ≈ 2.4A; fine for brief starts, avoid sustained stall) |
| Prototyping | MB-102 830-point breadboard; male-male / male-female / female-female jumpers |
| Instrumentation | 8-channel 24MHz USB logic analyzer; multimeter |
| Cabling | USB Mini-B (ST-LINK); 6-pin PH2.0 motor/encoder lead |

The encoder's 6-pin PH2.0 connector carries **both** motor power (Motor+/−) and
the encoder signals: pin 1 Motor+, 2 Vcc, 3 Ch A, 4 Ch B, 5 GND, 6 Motor−.
Power the encoder Vcc from **3.3V** (not 5V) so the A/B outputs — pulled up to
Vcc on-board — swing at the MCU's logic level.

### Pin map

| Function | Pin | Notes |
|----------|-----|-------|
| LD2 LED | PA5 | onboard, blink test |
| PWM (PWMA) | PA8 | TIM1_CH1, AF1, 20kHz |
| Direction AIN1 / AIN2 | PA9 / PA10 | GPIO outputs |
| UART TX / RX | PA2 / PA3 | USART2 AF7, 115200 8N1, ST-LINK VCP (TX-only enabled) |
| Encoder A / B | PA0 / PA1 | TIM2_CH1/CH2, AF1, encoder mode x4, pull-ups on |
| TB6612FNG STBY | tied HIGH | driver outputs nothing unless STBY is high |

TIM2 was chosen for the encoder because it is the only 32-bit timer available
on these pins: count deltas via unsigned subtraction are wraparound-safe and
sign-correct with no overflow handling.

## Architecture

```
firmware/                 STM32CubeIDE project, C, register-level
├── Src/main.c            control loop: 100Hz control / 20Hz telemetry
├── Src/systick.c         1ms tick, wraparound-safe millis()/delay_ms()
├── Src/motor.c           TIM1 CH1 PWM @ 20kHz + direction pins, signed duty ±100
├── Src/encoder.c         TIM2 hardware encoder mode (x4 decode), signed RPM
├── Src/uart.c            USART2 TX, blocking, + text helpers
├── Src/telemetry.c       14-byte frame packer (mirrors telemetry/protocol.py)
├── Src/pid.c             float PID: anti-windup, derivative-on-measurement
├── Startup/…             vector table / reset handler
└── Drivers/CMSIS/        core + device headers only (no ST firmware pack)

telemetry/                Python host tools (independent of the IDE project)
├── protocol.py           frame pack/unpack + incremental resyncing parser
├── serial_source.py      ST-LINK VCP auto-detect + mock serial source
├── check_serial.py       link smoke test: frame/error counts, no matplotlib
├── live_plot.py          live RPM vs setpoint plot + CSV logging
├── mock_stream.py        synthetic frames w/ injectable corruption
├── csv_logger.py         append-only CSV output
└── tests/                pytest suite (parser, pipeline, smoke tool)
```

`main.c` builds in one of three modes via `CONTROL_MODE`:

- **`MODE_OPEN_LOOP`** (default) — steps through a fixed duty profile while
  streaming telemetry; validates encoder counting and UART framing before any
  PID output is trusted.
- **`MODE_CLOSED_LOOP`** — PID on encoder RPM at 100Hz.
- **`MODE_CALIBRATE_CPR`** — motor off, prints the raw encoder count as text
  every 250ms; used to measure the encoder's counts-per-revolution by hand.

## Building and flashing

Build with STM32CubeIDE (Project > Build) and flash via the run/debug launch
config over ST-LINK. Required include paths (already in the project
settings): `../Drivers/CMSIS/Core/Include` and
`../Drivers/CMSIS/Device/ST/STM32F4xx/Include`.

Headless build: `cd firmware/Debug && make -j` works only after an IDE build
has regenerated the makefiles — they are generated artifacts that hardcode
the linker-script path and don't learn about new `Src/*.c` files until the
IDE runs. For an IDE-free sanity check, compile with CubeIDE's bundled
`arm-none-eabi-gcc` using the flags from `firmware/Debug/Src/subdir.mk`
(run such scripts under `bash`; zsh doesn't word-split flag variables).

## Telemetry protocol

14-byte little-endian frame, 20Hz (~2.4% of the 115200-baud wire):

```
[0]     START 0xAA
[1]     LEN 10
[2:6]   uint32 timestamp_ms
[6:8]   int16 rpm
[8:10]  int16 pwm_duty
[10:12] int16 setpoint_rpm
[12]    XOR checksum over the 10-byte payload
[13]    END 0x55
```

The host parser is an incremental byte-at-a-time state machine: it tolerates
frames split across serial reads and resyncs after corruption by discarding
back to the start-byte search on any framing or checksum mismatch. The C
packer (`telemetry.c`) and Python parser (`protocol.py`) were cross-checked
byte-for-byte by compiling the packer on the host.

## Host tools

```
cd telemetry
.venv/bin/python -m pytest tests/            # full test suite
.venv/bin/python check_serial.py             # link smoke test (live board)
.venv/bin/python check_serial.py --mock      # same, synthetic data
.venv/bin/python live_plot.py                # live plot from ST-LINK VCP
.venv/bin/python live_plot.py --mock         # full plot pipeline, no hardware
```

Everything auto-detects the ST-LINK VCP (`usbmodem*`); pass `--port` to
override. `--mock` runs the identical parse → CSV → plot pipeline on
synthetic (optionally corrupted) frames. Set up the venv with
`python3 -m venv .venv && .venv/bin/pip install matplotlib pyserial pytest`.

## Project status

**Verified on hardware:**
- GPIO blink (PA5), SysTick 1ms timebase
- TIM1 20kHz PWM on PA8 + direction control — confirmed on a logic analyzer,
  motor spinning both directions

**Written and compile-verified, awaiting hardware bring-up:**
- Encoder decode, UART telemetry, PID, and the mode-switchable main loop —
  compiles `-Wall -Wextra` clean in all three modes and links against the
  real linker script; never run on the board
- Host tooling verified end-to-end against mock streams (headless
  integration tests), not yet against a live board
- `ENCODER_CPR` in `encoder.h` is computed from the motor's gear ratio
  (11 PPR × 4 × 56 = 2464) but not yet confirmed on hardware — RPM values are
  provisional until the calibrate-mode check reconciles against it (see checklist)

**Planned:** closed-loop PID tuning on hardware, then a FreeRTOS port. (CAN
bus was descoped — the F401 has no bxCAN peripheral, and adding an external
MCP2515/SPI bridge wasn't worth the scope for this build.)

### Hardware bring-up checklist

1. Wire encoder A/B → PA0/PA1, encoder VCC/GND; TB6612FNG STBY high
2. Build in CubeIDE (regenerates the stale makefiles) and flash — default
   mode is open-loop duty steps
3. `check_serial.py` — confirm valid frames, near-zero error count
4. `live_plot.py` — duty steps visible, RPM nonzero and sign-correct in both
   directions
5. Rebuild with `CONTROL_MODE = MODE_CALIBRATE_CPR`, flash, open the VCP in a
   terminal, mark the output shaft, rotate exactly one turn by hand, read the
   printed count → reconcile against the computed `ENCODER_CPR` (2464) in
   `encoder.h` and correct it if the measurement disagrees
6. Rebuild with `MODE_CLOSED_LOOP`, tune PID gains

## Register-level notes and gotchas

- **TIM1/TIM8 dead output:** advanced-control timers output nothing on their
  pins unless `BDTR.MOE` is set, even with every other register correct.
- **FPU off at reset:** ST's startup calls `SystemInit` but declares it
  weak; with no definition the linker silently turns the call into a NOP,
  the FPU never gets enabled (`SCB->CPACR`), and the first float instruction
  under `-mfloat-abi=hard` takes a UsageFault. `main.c` defines `SystemInit`
  to enable CP10/CP11 (it runs before `.data`/`.bss` init, so it must not
  touch static storage).
- **USART2 baud:** APB1 = 16MHz HSI, OVER8=0: USARTDIV = 16e6/(16·115200) =
  8.68 → mantissa 8, fraction 11 → `BRR = 0x08B` (actual 115108 baud,
  −0.08% error).
- **Encoder inputs:** hall-encoder outputs are often open-collector, so
  PA0/PA1 run with internal pull-ups; encoder mode 3 (SMS=011) counts both
  edges of both channels (x4).
- **`pid_t` collision:** newlib's `<sys/types.h>` owns `pid_t`; the firmware
  struct is `pid_ctrl_t`.
- **No ST firmware pack:** CMSIS headers were pulled individually (from
  STMicroelectronics/cmsis-device-f4 and ARM-software/CMSIS_5) into
  `firmware/Drivers/CMSIS/` and include paths added manually.
- **Overflow margins:** RPM math is `delta·60000 / (CPR·period_ms)` in
  int32 — at the motor's 178 rpm ceiling and CPR 2464, worst-case delta over
  10ms is ~73 counts and the `delta·60000` numerator is ~4.4M, nowhere near
  the int32 limit; results are clamped to int16 for the wire.

## References

- RM0368 — STM32F401xB/C/D/E reference manual
- PM0214 — Cortex-M4 programming manual
- UM1724 — Nucleo-64 user manual
- STM32F401xD/E datasheet
