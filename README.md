# Bare-Metal STM32 BLDC/DC Motor Controller

Register-level embedded systems project on STM32F401RE (Nucleo-64).
No ST HAL/LL drivers — direct register manipulation against RM0368,
UM1724, and PM0214. CMSIS core headers (NVIC/SysTick intrinsics,
device register definitions) are used; ST's abstraction driver layer
is not.

Status: in progress. Start date 2026-07-01.

> **Keep this file current.** Whenever something important comes up —
> a bug and its fix, a hardware change, a pin reassignment, a decision
> reversal, a newly verified or newly broken feature, a lesson learned
> the hard way — update this README immediately. Append to the cheat sheet below in
> full; do not summarize old entries away. Nothing here should be
> allowed to go stale or get lost between sessions.

## What this is

Register-level embedded firmware on a Nucleo-F401RE controlling a brushed DC
gear motor. No ST HAL/LL drivers, direct register writes only against
RM0368/PM0214. CMSIS core and device headers are permitted and used. Goal:
demo-ready by end of August 2026 for summer 2027 embedded internship
applications (TI, ADI, Microchip, NXP, Infineon, SiLabs, ST, Garmin, Tesla;
NVIDIA Ignite as reach).

## Hardware

- Nucleo-F401RE (STM32F401RE, Cortex-M4)
- JGB37-520 12V DC gear motor with quadrature encoder
- TB6612FNG dual H-bridge driver
- 8-ch USB logic analyzer, UNI-T UT33A+ multimeter
- Flashing via onboard ST-LINK over USB (Mini-B cable)

## Toolchain

- STM32CubeIDE on Apple Silicon Mac
- Direct register manipulation, no HAL/LL. CMSIS headers only.
- CMSIS headers manually pulled via curl from STMicroelectronics/cmsis-device-f4
  and ARM-software/CMSIS_5 (STM32 firmware pack was never installed; headers
  live in `firmware/Drivers/CMSIS/`)
- Include paths added manually in CubeIDE project settings:
  - `../Drivers/CMSIS/Core/Include`
  - `../Drivers/CMSIS/Device/ST/STM32F4xx/Include`
- `screen` for UART, Python for telemetry

## Reference docs (verbatim register lookups only, no secondary sources)

- RM0368 (STM32F401xB/C/D/E Reference Manual)
- UM1724 (Nucleo-64 User Manual)
- PM0214 (Cortex-M4 Programming Manual)
- STM32F401xD/E datasheet

## Repo structure

github.com/leozh0u/stm32-bldc-motor-controller (SSH, main branch)
```
stm32-bldc-controller/            # git root
├── README.md
├── .gitignore
├── firmware/                     # CubeIDE project
│   ├── Src/main.c
│   ├── Inc/
│   ├── Startup/startup_stm32f401retx.s
│   ├── Drivers/CMSIS/Core/Include/                  # core_cm4.h, cmsis_gcc.h, etc.
│   ├── Drivers/CMSIS/Device/ST/STM32F4xx/Include/   # stm32f401xe.h, etc.
│   └── *.ld
└── telemetry/                    # Python host-side
    ├── protocol.py
    ├── csv_logger.py
    ├── mock_stream.py
    ├── live_plot.py
    └── tests/test_protocol.py
```

## Done and verified on hardware

1. Git/SSH/GitHub setup complete
2. Register-level GPIO blink (PA5/LD2)
3. SysTick 1ms delay, HSI 16MHz default clock, wraparound-safe delay pattern
4. TIM1 PWM on PA8 (CH1, AF1), 20kHz, direction pins PA9/PA10, verified on
   logic analyzer, motor confirmed spinning
5. Python telemetry protocol layer, unit tests passing

## Done, compile/test-verified, NOT yet hardware-verified (written 2026-07-06)

Full firmware rewrite into modules under `firmware/Src` + `firmware/Inc`:
`systick` (1ms tick), `motor` (TIM1 PWM, signed duty ±100), `encoder`
(TIM2 hardware encoder mode x4 on PA0/PA1), `uart` (USART2 TX 115200,
BRR=0x8B), `telemetry` (14-byte frame packer), `pid` (float, anti-windup,
derivative-on-measurement), `main` (100Hz control / 20Hz telemetry loop).

Verified WITHOUT hardware:
- Compiles clean (`-Wall -Wextra`, zero warnings) and links against
  `STM32F401RETX_FLASH.ld` with CubeIDE's own arm-none-eabi-gcc 14.3
- C `telemetry_pack()` output matches Python `pack_frame()` byte-for-byte
  on edge-case vectors (host-compiled cross-check)
- `live_plot.py` gap CLOSED: `--mock` flag runs the full pipeline on
  synthetic frames; headless integration test exercises parse → CSV → plot
  (6/6 tests pass)

Still needs hardware: everything above. `main.c` defaults to
`MODE_OPEN_LOOP` (duty-step profile + live telemetry) on purpose — first
board session should validate encoder counts and UART frames before
switching `CONTROL_MODE` to closed loop.

**`ENCODER_CPR` in `encoder.h` is a placeholder (1320).** Measure it:
mark the output shaft, rotate exactly one turn, read the count delta.
RPM numbers are meaningless until this is set correctly.

## Not started

- Closed-loop PID bring-up/tuning on hardware (code written, gains are
  guesses; 2-week hard timebox; cut FreeRTOS/CAN if not solid by then)
- FreeRTOS port (committed real scope, not stretch)
- CAN bus (committed real scope, not stretch)

## Hardware bring-up checklist (next board session)

1. Wire encoder A/B → PA0/PA1 (Arduino A0/A1), encoder VCC/GND; STBY high
2. Build in CubeIDE (regenerates stale makefiles — see cheat sheet), flash
3. `cd telemetry && .venv/bin/python live_plot.py` (auto-finds usbmodem VCP)
4. Verify duty steps in plot, RPM nonzero and sign-correct both directions
5. Measure real ENCODER_CPR (one hand-turn of output shaft), update encoder.h
6. Flip `CONTROL_MODE` to `MODE_CLOSED_LOOP`, tune PID gains

## Telemetry protocol spec

14-byte little-endian frame:
```
[0]     START 0xAA
[1]     LEN 10
[2:6]   uint32 timestamp_ms
[6:8]   int16 rpm
[8:10]  int16 pwm_duty
[10:12] int16 setpoint_rpm
[12]    XOR checksum of payload
[13]    END 0x55
```
Parser is an incremental state machine handling split reads and corruption
resync.

## Pin map (current)

| Function | Pin | Notes |
|----------|-----|-------|
| LD2 LED | PA5 | onboard, blink test |
| PWM (PWMA) | PA8 | TIM1_CH1, AF1, 20kHz |
| Direction AIN1 | PA9 | GPIO out |
| Direction AIN2 | PA10 | GPIO out |
| UART TX | PA2 | USART2 AF7, 115200 8N1, ST-LINK VCP, firmware written |
| UART RX | PA3 | USART2 AF7, pin claimed, RX not enabled yet |
| Encoder A | PA0 | TIM2_CH1 AF1, encoder mode x4, pull-up, firmware written |
| Encoder B | PA1 | TIM2_CH2 AF1, encoder mode x4, pull-up, firmware written |
| STBY | GPIO/HIGH | must be high or TB6612FNG outputs nothing |

(TIM2 chosen for the encoder because it is the only 32-bit timer here —
count deltas via unsigned subtraction are wraparound-safe with no overflow
handling. PA0/PA1 = Arduino A0/A1 on the Nucleo header.)

## Bugs and lessons logged (cheat sheet, preserve in full)

- TIM1 dead output: advanced-control timers (TIM1, TIM8) need `BDTR |= MOE` or
  the pin stays dead despite all other config being correct.
- Lost code across iterations: files never saved to disk before moving the
  project folder; CubeIDE held unsaved buffer state. Rule: `cat` every file
  after editing to confirm disk write.
- Project outside git: CubeIDE workspace was in a separate directory from the
  repo. Fixed by moving the project into `firmware/`.
- SSH keygen failures: pasted subsequent commands into prompts instead of
  pressing Enter; pasted literal `pbcopy` command text into GitHub's key field
  instead of running it in terminal first.
- Push rejected: GitHub auto-generated README caused diverged histories;
  diagnosed with `git show --stat`, fixed with `git push --force`.
- Missing CMSIS: no firmware pack installed; pulled headers individually via
  curl, added include paths manually.
- Stale generated makefiles after moving the project: `firmware/Debug/makefile`
  hardcodes the linker script at the OLD absolute path
  (`/Users/leo/stm32-bldc-controller/...`), so headless `make` fails to link
  until CubeIDE regenerates the makefiles (any IDE build does this). Same
  applies after adding new `Src/*.c` files — the IDE auto-discovers them but
  the generated `subdir.mk` doesn't know them until an IDE build runs.
- `pid_t` name collision: newlib's `<sys/types.h>` owns `pid_t`; firmware
  struct is named `pid_ctrl_t` to avoid it.
- zsh does not word-split unquoted `$FLAGS` variables (bash does) — compile
  scripts with flag variables must run under `bash`, or every flag lands in
  one argument and gcc errors incomprehensibly.

## Working rules

- Code delivered step by step, no HAL/LL
- Verify every file write with `cat` before proceeding
- Commit and push after each verified step
- Nothing marked done until verified on hardware
- Cheat sheet is cumulative: every bug, fix, step, and code snippet logged in
  full, nothing dropped or summarized away
- Resume bullet withheld until project complete (current draft has false
  dates, unbuilt SPI/I2C drivers, unverified PID figures, and stretch goals
  stated as done)

## Telemetry quick reference

```
cd telemetry
.venv/bin/python -m pytest tests/          # all tests (6)
.venv/bin/python live_plot.py --mock       # plotter on synthetic data, no board
.venv/bin/python live_plot.py              # live from ST-LINK VCP (usbmodem)
```
`.venv` holds matplotlib/pyserial/pytest (system python3 lacks matplotlib;
recreate with `python3 -m venv .venv && .venv/bin/pip install matplotlib
pyserial pytest` if missing — it's gitignored).
