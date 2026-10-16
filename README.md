# STM32 Signal Processing --- NUCLEO-F446RE

## Overview

This project is a learning platform for embedded signal processing on
the **ST NUCLEO-F446RE** development board. It samples an analogue
input, processes sample buffers with an FFT, generates an analogue test
waveform, and reports results over UART.

The project has been developed on Ubuntu 20.04 and uses
STM32CubeMX-generated configuration, STM32 HAL drivers,
FreeRTOS/CMSIS-RTOS2, and CMSIS-DSP.

> **Status note:** This README records the features and measurements
> established during development. Pin mappings and configuration should
> be checked against the current `.ioc` file and source before wiring a
> different setup. The project was recently restored to a known-good Git
> version after experiments with additional semaphore/mutex tasks and
> the IWDG; those experiments are not assumed to be part of the current
> baseline.

## Hardware

### Main equipment

-   ST NUCLEO-F446RE development board (STM32F446RE, ARM Cortex-M4 with
    FPU)
-   PicoScope 2204A USB oscilloscope with arbitrary waveform generator
-   Ubuntu 20.04 host PC
-   Jumper wires and test load components as required

Planned/future hardware includes an ST7735 1.8-inch SPI TFT display and
additional I²C sensors (AHT20 humidity/temperature and BMP280
temperature/pressure). Treat these as future extensions unless the
current source tree shows that their drivers have been integrated.

## Pin map

The following table captures the project pin assignments used during
development. Confirm alternate-function assignments in
`STM32_Signal_Processing.ioc` if CubeMX configuration has changed.

  --------------------------------------------------------------------------------
  Function       STM32 pin      Board          Direction       Purpose
                                connector /                    
                                signal                         
  -------------- -------------- -------------- --------------- -------------------
  ADC input      PA0            Arduino A0     Input           ADC1 channel 0;
                                                               receives the
                                                               PicoScope AWG test
                                                               signal

  DAC output     PA4            Arduino A2 on  Output          DAC channel 1
                                common Nucleo                  waveform output
                                Arduino                        
                                mapping\*                      

  PWM output     PA6            Arduino D12    Output          TIM3 channel 1,
                                                               approximately 1
                                                               kHz, 50% duty cycle

  UART TX        PA2            USART2 TX;     Output          Sends
                                ST-LINK                        diagnostic/FFT data
                                virtual COM                    to the host
                                connection on                  
                                the board                      

  UART RX        PA3            USART2 RX      Input           Configured as the
                                                               USART2 receive pin;
                                                               use if receiving
                                                               data

  GPIO timing    PB0            Arduino A3     Output          Goes high/low
  marker                                                       around a measured
                                                               code region for
                                                               timing with the
                                                               PicoScope

  I²C (planned   PB8 / PB9      I²C1 SCL / SDA Bidirectional   Proposed connection
  sensor                                       bus             for AHT20 and
  extension)                                                   BMP280; not part of
                                                               the confirmed
                                                               signal-processing
                                                               baseline
  --------------------------------------------------------------------------------

\* **Check the physical header labels:** Arduino connector labels and
peripheral alternate functions can be easy to mix up. In the project's
configuration, DAC channel 1 is on PA4. On the NUCLEO-F446RE PA4 is
normally Arduino A2; PA6 is Arduino D12. Verify against the board user
manual and `.ioc` before wiring.

### Signal levels and safety

-   The STM32 ADC input must remain within its permitted input range
    (normally 0--3.3 V on this board; do not apply a negative voltage).
-   The PicoScope 2204A AWG was used for low-voltage test waveforms.
    Configure the generator offset and amplitude so the complete
    waveform stays within the ADC input range.
-   Connect instrument and board grounds together for a single-ended
    test.
-   Do not connect the PicoScope generator directly to a pin if the
    configured waveform can exceed the pin's electrical limits.

## Peripheral configuration

### ADC and timer trigger

-   ADC: **ADC1 channel 0**, pin **PA0**.

-   Trigger: **TIM2 TRGO**.

-   Timer configuration used:

    -   TIM2 prescaler: `83`
    -   TIM2 auto-reload value: `61`
    -   Timer clock assumed to be 84 MHz

-   Approximate sampling rate:

    `84 MHz / ((83 + 1) × (61 + 1)) ≈ 16,129 samples/s`

    This is approximately **16.13 kHz** (often referred to as 16 kHz in
    the project notes).

-   ADC data is transferred to memory using **DMA in circular mode**.

-   The FFT input buffer contains **2048 samples**, split into two
    **1024-sample halves**.

-   DMA half-transfer and transfer-complete callbacks notify the
    processing task when a half-buffer is ready. Each half represents
    approximately 63.5 ms of samples at 16.13 kHz.

### DAC waveform generation

-   DAC: **DAC channel 1**, pin **PA4**.
-   Trigger: **TIM6 TRGO**.
-   Timer configuration used:
    -   TIM6 prescaler: `104`
    -   TIM6 auto-reload value: `49`
    -   With an 84 MHz timer clock, the update rate is approximately 16
        kHz.
-   A circular DMA buffer containing **256 sine-wave samples** is used.
-   At a 16 kHz DAC update rate, a 256-sample waveform produces
    approximately **62.5 Hz**.

### PWM

-   Pin: **PA6 / TIM3 channel 1**.
-   Configured output: approximately **1 kHz**, **50% duty cycle**.
-   This provides a simple timer/PWM signal for oscilloscope
    experiments.

### UART

-   Peripheral: **USART2**.
-   TX: **PA2**; RX: **PA3**.
-   Format: **115200 baud, 8 data bits, no parity, 1 stop bit (115200
    8N1)**.
-   UART is used to report measurements and diagnostics to the host PC.
    On the NUCLEO board, the ST-LINK USB connection typically provides
    the virtual COM port.

Example terminal command (adjust the device name if necessary):

``` bash
screen /dev/ttyACM0 115200
```

To exit `screen`, press `Ctrl-A`, then `K`, then confirm.

### GPIO timing marker

-   Pin: **PB0**.
-   Used as a timing marker by toggling or setting it around a code
    section.
-   A PicoScope measurement of the pulse width can estimate the
    execution time of that section.
-   A 3.3 V GPIO is above a 2 V peak range in some PicoScope
    configurations; use a suitable divider or configure the scope input
    range appropriately. A 2:1 voltage divider was used during testing.

## FFT and signal-processing routine

The application uses the **CMSIS-DSP** library for Cortex-M DSP
operations.

The processing flow is:

1.  TIM2 triggers ADC conversions at approximately 16.13 kHz.
2.  ADC DMA fills a 2048-sample circular buffer.
3.  DMA callbacks signal when either 1024-sample half is ready.
4.  The FFT processing task removes the DC mean and applies a Hann
    window.
5.  The real-valued FFT is calculated.
6.  Magnitudes are examined to locate the dominant spectral peak.
7.  Peak interpolation is used to refine the frequency estimate between
    FFT bins.
8.  The calculated frequency is made available to the UART reporting
    task.

For a 2048-point transform at approximately 16 kHz, the nominal bin
spacing is:

`16,000 / 2048 = 7.8125 Hz`

Some earlier project measurements/notes used a 1024-sample FFT, for
which the bin spacing at 16 kHz is:

`16,000 / 1024 = 15.625 Hz`

The current FFT buffer length and transform length should be checked in
source code before relying on either value. With a 2048-sample FFT at
the actual configured sampling rate (about 16.129 kHz), nominal bin
spacing is about 7.88 Hz.

### CMSIS-DSP

CMSIS-DSP source files were copied into the project under:

`Core/Src/CMSIS_DSP/`

The code is built for the Cortex-M4 with hardware floating-point
support. Check the project compiler settings and CMSIS-DSP source
directory if moving the project to a different toolchain.

## DMA interrupts and FreeRTOS

### ADC DMA callbacks

The ADC DMA half-transfer and transfer-complete callbacks run in
interrupt context. They signal the processing task that a buffer half is
ready; FFT processing itself is performed in task context rather than
inside the interrupt callback.

A queue-based event mechanism was implemented during development to
preserve half/full events and avoid losing notifications when the
processing task was delayed:

-   Queue capacity: **8 events**
-   Queue item: `uint32_t`
-   Event values: `1` for half-transfer, `2` for transfer-complete
-   Queue overflow is counted for diagnostics.
-   The callback uses a FreeRTOS ISR-safe queue send API.
-   The FFT task blocks waiting for events, then processes the
    corresponding buffer half.

The ISR priority must comply with FreeRTOS interrupt-priority rules when
calling `...FromISR()` APIs. The project used a DMA callback priority of
5 and `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` of 5; verify these
values in the current generated configuration.

### FreeRTOS/CMSIS-RTOS2 tasks

The project uses FreeRTOS through the CMSIS-RTOS2 API.

The established application structure includes:

-   **Default/UART task:** handles serial reporting and diagnostics.
-   **FFT task (`StartTask02`):** waits for ADC DMA events and processes
    ready sample blocks.
-   **Frequency queue:** carries `float32_t` frequency results to the
    reporting task.

The FFT task stack was increased during development to **1024 words**
after smaller settings caused a HardFault. The default/UART task used
**512 words** in the established configuration. Confirm actual current
values in `main.c` / generated task attributes.

Additional semaphore, mutex, diagnostic-task, priority-inheritance and
watchdog experiments were started during development, but the project
was later restored to an earlier Git version after CubeMX regeneration
disrupted source files. These experiments should be treated as optional
exercises, not guaranteed current features.

## PicoScope 2204A measurements

The PicoScope 2204A was used as both an oscilloscope and an arbitrary
waveform source.

Measurements and observations recorded during development:

-   AWG test signal was connected to **PA0 / ADC1_IN0** and board
    ground.
-   A test waveform was configured as a **100 Hz sine wave, 1.0 V
    peak-to-peak, with +1.0 V DC offset**.
-   FFT tests showed dominant peaks at approximately **200 Hz, 400 Hz
    and 600 Hz** for corresponding test signals.
-   The FFT peak-detection routine reported approximately **62.62 Hz**
    for the DAC-generated nominal 62.5 Hz sine wave.
-   A GPIO timing marker on PB0 was measured using the PicoScope. A
    voltage divider was used to reduce the GPIO voltage for the selected scope
    input range.

These are recorded test observations, not guaranteed results for every
build. Actual estimates depend on the AWG setup, sample rate, buffer
length, windowing and signal-to-noise ratio.

## STM32CubeMX and STM32CubeIDE setup

Versions recorded for this project:

  Tool/component           Version or details
  ------------------------ -------------------------------------------------
  Host OS                  Ubuntu 20.04.6 LTS, amd64
  STM32CubeIDE             **1.16.1**
  STM32CubeMX              **6.15.0**
  STM32 firmware package   **STM32Cube FW_F4 V1.28.3**
  Target                   NUCLEO-F446RE / STM32F446RE
  RTOS API                 FreeRTOS via CMSIS-RTOS2
  DSP library              CMSIS-DSP sources included in the project
  Debug/programming        On-board ST-LINK
  Serial terminal          `screen` on Ubuntu (or another serial terminal)

The `.ioc` file is the CubeMX configuration source for pin assignments,
clocks, peripherals, DMA, FreeRTOS and middleware settings. Generated
files can be overwritten when code is regenerated. Put custom code
inside the relevant `/* USER CODE BEGIN ... */` /
`/* USER CODE END ... */` blocks where possible, and keep a Git commit
before changing the `.ioc` or regenerating.

### Dependencies and build configuration

The project depends on:

-   STM32Cube HAL and CMSIS files supplied by the STM32Cube F4 firmware
    package.
-   FreeRTOS kernel and CMSIS-RTOS2 adaptation supplied/configured by
    CubeMX.
-   CMSIS-DSP sources included in the repository.
-   A compatible ARM GNU toolchain configured by STM32CubeIDE.
-   ST-LINK support from STM32CubeIDE for flashing and debugging.

Exact compiler and FreeRTOS kernel version numbers were not captured in
the project notes; check the installed IDE package and project source if
a fully reproducible toolchain manifest is required.

## Build, flash and run

1.  Open STM32CubeIDE 1.16.1.
2.  Import or open this project.
3.  Confirm the target is NUCLEO-F446RE and review the `.ioc`
    configuration.
4.  Build the project.
5.  Connect the NUCLEO board over USB.
6.  Flash and run/debug using the ST-LINK configuration.
7.  Open the UART virtual COM port at 115200 8N1 to view output.
8.  Connect the PicoScope to the ADC input, DAC output or GPIO timing
    marker as appropriate, with a common ground and safe voltage levels.

If CubeMX code generation is needed, commit or back up the working
source first. After generation, inspect the Git diff and rebuild before
continuing.

## Git repository

The project repository recorded during development is:

https://github.com/srmt1/STM32_Signal_Processing.git

A previous known-good commit was recorded as `6bd4522` (CMSIS-DSP and
STM32 HAL sources included; 117 files). The current branch and commit
may have changed since then.

## Future extensions

-   Add the AHT20 and BMP280 I²C drivers and report humidity,
    temperature and pressure.
-   Display the FFT spectrum on the ST7735 SPI TFT.
-   Add a properly designed IWDG watchdog monitor with task-health
    checks.
-   Measure FFT worst-case execution time and deadline misses using the
    GPIO marker and PicoScope.
-   Add a second STM32 and CAN transceivers for a two-node CAN
    communications exercise.

## Troubleshooting notes

-   If FFT events are lost, inspect the ADC DMA callback counters and
    queue-overflow counter, and ensure the FFT task keeps up with the
    half-buffer arrival rate.
-   If a HardFault occurs after increasing FFT processing complexity,
    review the FFT task stack size.
-   If the UART port is absent, check the ST-LINK USB connection, Linux
    device name and serial permissions.
-   If code generation removes custom functions, use Git history to
    recover the last working version and move custom code into
    `USER CODE` sections where feasible.
-   If the watchdog is enabled without periodic refresh, the board will
    reset after the configured timeout. Do not enable it until a
    deliberate refresh/health-monitoring strategy is in place.
