# STM32 Signal Processing Project

**Target:** ST NUCLEO-F446RE / STM32F446RE  
**Host development environment:** Ubuntu 20.04.6 LTS (64-bit)  
**Repository:** <https://github.com/srmt1/STM32_Signal_Processing>

## 1. Overview

This is a hands-on embedded systems and digital signal processing (DSP) learning project for the STM32F446RE. It brings together timer-driven sampling, DMA, FFT-based frequency estimation, analogue waveform generation, PWM, UART diagnostics, I²C environmental sensors, an SPI TFT display, and FreeRTOS tasks using the CMSIS-RTOS2 API.

Current functionality includes:

- Samples an external analogue signal on ADC1, with conversions triggered by TIM2.
- Uses circular DMA to move ADC samples into RAM with limited CPU involvement.
- Processes sample blocks using Arm CMSIS-DSP to estimate the dominant input frequency.
- Removes DC offset, applies a Hann window, performs a real FFT, calculates magnitudes and estimates the dominant spectral peak.
- Generates a test waveform through DAC channel 1 using TIM6-triggered DMA.
- Generates a PWM test signal using TIM3 channel 1.
- Reports frequency and sensor measurements over USART2.
- Reads temperature and relative humidity from an AHT20, and temperature and pressure from a BMP280, over I²C.
- Displays sensor readings and the estimated frequency on a 128 × 160 ST7735 TFT over SPI2.
- Uses FreeRTOS through CMSIS-RTOS2 to separate work into tasks and communicate via message queues.
- Demonstrates peripheral interrupts, HAL callbacks, DMA, queues, mutexes, and shared-resource management.

This is a learning project, not a production or safety-certified design. The `.ioc` file and source code in the checked-out revision should be treated as authoritative if any details below differ.

## 2. Hardware

### Main equipment

- **Development board:** ST NUCLEO-F446RE, STM32F446RE Arm Cortex-M4 with FPU.
- **Signal source and measurement:** PicoScope 2204A USB oscilloscope with arbitrary waveform generator.
- **Environmental sensor:** Youmile combined AHT20 + BMP280 I²C module.
- **Display:** 1.8-inch 128 × 160 ST7735 SPI TFT module. The SD-card socket on the module is not used.
- **Host:** Ubuntu 20.04.6 LTS PC, connected to the Nucleo through USB/ST-LINK.

### Electrical precautions

- Keep the ADC input within the STM32 pin's permitted range. For this setup, use a signal between **0 V and 3.3 V** unless the circuit has been designed and verified to provide suitable protection and scaling.
- Do not connect a negative-going waveform directly to PA0. Check the waveform's amplitude **and offset**, including its peaks.
- Connect the signal source ground to Nucleo GND when using a single-ended input.
- Power the sensor and TFT from 3.3 V and connect their grounds to Nucleo GND.
- The TFT backlight LED is connected to 3.3 V through a 100 Ω series resistor in the current setup.
- Check the module pin labels before connecting power. The TFT pin marked `SDA` is SPI MOSI data, not I²C SDA.

## 3. Software and dependencies

Versions recorded for the development setup:

| Component | Version / details |
|---|---|
| Host OS | Ubuntu 20.04.6 LTS, 64-bit |
| STM32CubeMX | 6.15.0 |
| STM32CubeIDE | 1.16.1, Linux x86-64 |
| STM32CubeF4 firmware package | V1.28.3 |
| MCU / CMSIS core | STM32F446RE, Cortex-M4F |
| STM32 HAL and device CMSIS | Provided by the STM32CubeF4 package |
| RTOS | FreeRTOS, accessed through CMSIS-RTOS2 |
| DSP library | Arm CMSIS-DSP |
| Compiler / debugger | ARM GNU toolchain and ST-LINK supplied/configured with STM32CubeIDE |
| Serial console | USART2, 115200 baud, 8 data bits, no parity, 1 stop bit |

### Required software components

1. **STM32CubeIDE 1.16.1** — project import, compilation, linking, flashing and debugging.
2. **STM32CubeMX 6.15.0** — peripheral/pin configuration stored in the `.ioc` file. CubeMX is integrated with CubeIDE workflows, but the standalone installation is useful for editing configuration.
3. **STM32CubeF4 firmware package V1.28.3** — STM32F4 HAL drivers, CMSIS device/core files, and the middleware components used by the project.
4. **FreeRTOS** — kernel sources/configuration included through the STM32CubeF4/CubeIDE project setup. Application code uses the **CMSIS-RTOS2 API** (`osThread...`, `osMessageQueue...`, `osMutex...`) rather than calling the FreeRTOS API directly for ordinary task/queue/mutex operations.
5. **Arm CMSIS-DSP** — used for FFT and vector/magnitude processing. The project uses the CMSIS-DSP headers and source files available in the installed STM32CubeF4 package / project configuration. The build must include the relevant DSP include path and compile/link the DSP sources or library used by the project.
6. **STM32 HAL** — included with STM32CubeF4 and used for ADC, DMA, DAC, timers, UART, I²C, SPI and GPIO.
7. **GNU Arm Embedded toolchain** — normally installed and selected by STM32CubeIDE; a separate system-wide compiler installation is not required when the IDE's bundled toolchain is used.
8. **Serial terminal** — for example `screen`, `minicom`, or another serial-terminal application.

The precise GNU Arm toolchain version and exact CMSIS-DSP / FreeRTOS source revisions should be recorded from the installed IDE/package if strict build reproducibility is required. Avoid adding a second, unrelated CMSIS-DSP or FreeRTOS version to the build without updating include paths and build settings consistently.

### CMSIS-DSP implementation notes

The FFT code uses the floating-point real FFT API (`arm_rfft_fast_f32`) and associated CMSIS-DSP functions. The target is Cortex-M4F, with floating-point compiler settings configured for the MCU (FPv4-SP-D16 / hard-float ABI in the recorded build). Ensure that the compiler flags, CMSIS-DSP build and linked objects agree on the target architecture and floating-point ABI.

## 4. Pin configuration

The table below records the wiring and configuration established during development. Check `STM32_Signal_Processing.ioc` and the generated peripheral setup if the project configuration has changed.

### Signal processing and debug

| Function | MCU pin | Peripheral / mode | Direction | Purpose |
|---|---|---|---|---|
| Analogue input | PA0 | ADC1 channel 0 | Input | Signal from PicoScope AWG or another suitable source |
| Analogue output | PA4 | DAC channel 1 | Output | DAC test waveform |
| PWM output | PA6 | TIM3 channel 1 | Output | Timer/PWM test signal, approximately 1 kHz and 50% duty cycle in the recorded setup |
| UART TX | PA2 | USART2_TX | Output | Frequency, sensor and diagnostic output to host |
| UART RX | PA3 | USART2_RX | Input | Configured receive pin; available for future host commands |
| Timing marker | PB0 | GPIO output | Output | Optional marker for measuring code execution time with an oscilloscope |

On the NUCLEO-F446RE, PA2/PA3 are connected to the ST-LINK virtual COM interface. Use the MCU pin names and the board's schematic/header markings when wiring.

### I²C environmental sensors

| Signal | MCU pin / connection | Peripheral | Purpose |
|---|---|---|---|
| SCL | PB8 - 10 kΩ resistor - 3.3 V | I2C1_SCL | I²C clock for AHT20 and BMP280 |
| SDA | PB9 - 10 kΩ resistor - 3.3 V | I2C1_SDA | I²C data for AHT20 and BMP280 |

Observed 7-bit addresses:

- AHT20: `0x38`
- BMP280: `0x77`

Both sensors share the same I²C bus.

### ST7735 TFT

| TFT module pin | MCU pin / connection | Purpose |
|---|---|---|
| SCK | PB13 | SPI2 clock |
| SDA | PB15 | SPI2 MOSI data — **not** I²C SDA |
| AO / D-C | PB10 | Selects command versus data |
| RESET | PB2 | Display-controller reset |
| CS | PB12 | TFT chip select, controlled by GPIO |
| LED | 3.3 V through 100 Ω resistor | Backlight |
| VCC | 3.3 V | Power |
| GND | GND | Ground |
| SD socket | Not connected/used | SD-card functionality is not implemented |

SPI2 is configured as master, two-line full duplex, software NSS, Motorola mode, 8-bit data, MSB first, clock polarity low, first-edge clock phase, and prescaler 8. PC2/SPI2_MISO remains enabled in the CubeMX configuration but is not connected to this write-only display module.

### Timers and DMA

The current architecture uses:

- **TIM2 TRGO → ADC1 → circular DMA** for regular analogue sampling.
- **DMA half-transfer and transfer-complete events** to tell the FFT task when a buffer half is ready.
- **TIM6 TRGO → DAC channel 1 + DMA** for waveform generation.
- **TIM3 channel 1** for PWM.

Previously recorded timer settings were TIM2 prescaler 83 and auto-reload 61, and TIM6 prescaler 104 and auto-reload 49, assuming an 84 MHz timer clock. These correspond to approximately 16.13 ksample/s ADC triggering and 16 ksample/s DAC updating. Verify the current `.ioc`, clock tree and timer constants before using these figures for precision measurements.

The recorded ADC DMA buffer has 2048 samples, divided into two 1024-sample halves. At about 16.13 ksample/s, one half represents approximately 63.5 ms of input data. The exact sample rate, FFT length and frequency-bin spacing depend on the active constants and clock configuration.

## 5. Hardware and software architecture

### Data-flow overview

```text
External analogue signal
        |
        v
PA0 / ADC1 <--- TIM2 TRGO
        |
        v
Circular DMA sample buffer
        |
        +--> Half-transfer / transfer-complete callbacks
                         |
                         v
                   ADC event queue
                         |
                         v
                      FFT task
                         |
                         +--> frequencyQueueHandle
                         |           |
                         |           v
                         |     UART frequency output
                         |
                         +--> tftFrequencyQueueHandle
                                     |
                                     v
                                  TFT task

AHT20 + BMP280
        |
        v
I2C1 (PB8/PB9)
        |
        v
    Sensor task
        |
        +--> sensorDataQueueHandle --> sensor-print task --> USART2
        |
        +--> tftDataQueueHandle -------------------------> TFT task

TIM6 TRGO --> DAC DMA --> PA4 analogue output
TIM3 CH1 ----------------> PA6 PWM output
SPI2 (PB13/PB15) --------> ST7735 TFT
```

### ADC, DMA and FFT processing

TIM2 triggers ADC conversions at a regular hardware-defined interval. This avoids relying on task scheduling to determine the sampling instant. ADC samples are transferred into a circular DMA buffer, reducing the CPU work needed to move individual samples.

The buffer is split into two halves. When DMA has filled one half, a HAL callback signals that the corresponding block is ready. The callback should perform minimal work and notify the processing task rather than execute the FFT in interrupt context.

The FFT task processes the ready block using CMSIS-DSP. The processing path includes DC-mean removal, a Hann window, a real FFT, magnitude calculation and dominant-peak selection. Peak interpolation is used to refine the frequency estimate between FFT bins. The resulting frequency is sent to separate UART and TFT queues so that one consumer cannot remove a message intended for the other.

### DAC waveform generation

DAC channel 1 on PA4 is updated by DMA, with TIM6 TRGO providing the update trigger. A waveform lookup table supplies the DAC samples. The recorded implementation uses a 256-sample sine table; at a 16 kHz update rate, this produces approximately 62.5 Hz (`16000 / 256`). Confirm the current table and timer rate before relying on this value.

### PWM and timing measurements

TIM3 channel 1 generates PWM on PA6. The recorded setup is approximately 1 kHz with 50% duty cycle. PB0 is available as a GPIO timing marker: toggling it around a code region allows execution time to be measured with the PicoScope.

### I²C sensor subsystem

The sensor task reads the combined AHT20/BMP280 module over I2C1. AHT20 readings provide temperature and relative humidity; BMP280 readings provide temperature and pressure. The results and driver status are collected in a `SensorData` structure, including a millisecond timestamp.

The sensor task publishes copies to separate UART and TFT queues. Separate queues are important because a message queue normally delivers each message to one consumer; two consumers sharing one queue would compete for messages.

### TFT display subsystem

The ST7735 driver uses SPI2 for data and GPIO signals for reset, chip select and command/data selection. The application displays AHT20 temperature and humidity, BMP280 temperature and pressure, and the estimated dominant frequency.

The driver selects a rectangular address window in the display controller and transfers RGB565 pixel data. Characters use a small 5 × 7 bitmap font. The optimised character routine builds all 35 pixels (70 bytes in RGB565) in RAM and sends them in one SPI data transfer rather than repeatedly setting the address window for each pixel. The TFT's SD-card socket is not implemented.

### FreeRTOS and CMSIS-RTOS2

FreeRTOS schedules the application tasks; application code uses the CMSIS-RTOS2 API. Message queues pass data between producers and consumers, while a mutex serialises access to UART output from multiple tasks.

The main tasks and objects are:

| Task / object | Responsibility |
|---|---|
| `StartDefaultTask` | Consumes frequency results and reports frequency/diagnostics over UART |
| `StartTask02` | Processes ADC DMA events, runs the FFT and publishes frequency results |
| `StartSensorTask` | Periodically reads the AHT20/BMP280 and publishes sensor samples |
| `StartSensorPrintTask` | Receives sensor samples and prints readings/status over UART |
| `StartTFTTask` | Consumes TFT-specific sensor/frequency queues and updates the display |
| `adcEventQueueHandle` | Signals which ADC DMA buffer half is ready for processing |
| `frequencyQueueHandle` | Frequency estimates for the UART consumer |
| `tftFrequencyQueueHandle` | Separate frequency queue for the TFT consumer |
| `sensorDataQueueHandle` | Sensor samples for UART printing |
| `tftDataQueueHandle` | Sensor samples for the TFT |
| `uartMutexHandle` | Protects UART output from concurrent task access |

The TFT paths use latest-value behaviour where appropriate. The frequency consumer drains pending TFT frequency messages and retains the newest estimate. If the TFT sensor queue is full, the producer resets its pending messages and inserts the newest sample. This avoids spending display time rendering stale readings. The UART sensor queue remains separate and uses a short put timeout; if the queue stays full, a sample can still be lost unless retry or overflow accounting is added.

Only `StartTFTTask` currently draws to the display, so a separate TFT mutex is not needed. Add one if future tasks also access the TFT driver. Queue operations should check return codes, and interrupt callbacks must remain short.

## 6. Important files and functions

Names and responsibilities below reflect the implementation established during development. Check the checked-out source for exact names if the project has since changed.

### Main and peripheral initialisation

| File / function | Responsibility |
|---|---|
| `Core/Src/main.c` — `main()` | Initialises HAL and peripherals, creates RTOS queues/mutexes/tasks, starts the scheduler and initialises the TFT |
| `SystemClock_Config()` | Configures the MCU clock tree |
| `MX_GPIO_Init()` | Configures GPIO, including TFT control pins and timing marker |
| `MX_DMA_Init()` | Enables DMA clocks and configures DMA interrupt priorities |
| `MX_ADC1_Init()` | Configures ADC1 |
| `MX_DAC_Init()` | Configures DAC channel 1 |
| `MX_I2C1_Init()` | Configures I²C1 for the sensors |
| `MX_SPI2_Init()` | Configures SPI2 for the TFT |
| `MX_TIM2_Init()` | Configures the ADC trigger timer |
| `MX_TIM3_Init()` | Configures PWM |
| `MX_TIM6_Init()` | Configures the DAC trigger timer |
| `MX_USART2_UART_Init()` | Configures USART2 |
| `Error_Handler()` | Handles fatal initialisation errors |

CubeMX-generated functions may be in `main.c` or generated source files, depending on the project settings.

### ADC and FFT

| Function / callback | Responsibility |
|---|---|
| `HAL_ADC_ConvHalfCpltCallback()` | Signals that the first half of the circular ADC DMA buffer is ready |
| `HAL_ADC_ConvCpltCallback()` | Signals that the second half of the circular ADC DMA buffer is ready |
| `StartTask02()` | Waits for ADC DMA events, processes the corresponding sample block and publishes the estimated frequency |
| FFT processing routines | Remove DC offset, apply the window, run CMSIS-DSP FFT operations, calculate magnitudes and estimate the dominant frequency |

The DMA callbacks execute in interrupt context. Do not run the complete FFT or use blocking RTOS calls in them. If an ISR uses a FreeRTOS `...FromISR()` API, its NVIC priority must comply with the project's FreeRTOS interrupt-priority configuration. When using CMSIS-RTOS2 from an ISR, use only APIs permitted in interrupt context by the CMSIS-RTOS2 implementation.

### Sensor drivers and tasks

| Function / component | Responsibility |
|---|---|
| AHT20 driver | Initialises the AHT20 and reads temperature and relative humidity |
| BMP280 driver | Reads the chip ID, initialises the sensor and reads temperature and pressure |
| `StartSensorTask()` | Periodically reads sensors and publishes a `SensorData` sample |
| `StartSensorPrintTask()` | Receives sensor samples and prints readings/status over UART |
| `SensorData` | Holds AHT20/BMP280 readings, HAL status values and a timestamp |

Use the current AHT20/BMP280 header files as the source of truth for the exact public driver function names.

### UART output

| Function / object | Responsibility |
|---|---|
| `StartDefaultTask()` | Consumes frequency results and reports them over UART |
| `StartSensorPrintTask()` | Prints sensor readings and status |
| `__io_putchar()` | Retargets character output (including `printf`, where configured) to USART2 |
| `uartMutexHandle` | Serialises output from multiple tasks |
| `frequencyQueueHandle` | Transfers frequency estimates to the UART consumer |
| `sensorDataQueueHandle` | Transfers sensor samples to the UART consumer |

### ST7735 TFT driver

| Function | Responsibility |
|---|---|
| `ST7735_Init(&hspi2)` | Associates the SPI handle and initialises the display controller |
| `ST7735_FillScreen(colour)` | Fills the display with a solid colour |
| `ST7735_WriteString(x, y, text, foreground, background)` | Draws a string at the requested coordinates |
| `ST7735_DrawChar(...)` | Converts one font glyph into RGB565 pixels and transmits the character block |
| `ST7735_SetAddressWindow(...)` | Selects the rectangular region to update |
| `ST7735_WriteCommand(...)` | Sends a controller command |
| `ST7735_WriteData(...)` | Sends command parameters or pixel data |
| `StartTFTTask()` | Reads the TFT queues and updates sensor/frequency lines |

The display uses offsets `ST7735_XSTART = 2` and `ST7735_YSTART = 1` to align the controller address region with the visible pixels on the current module. The font is intentionally small and supports a limited set of punctuation; unsupported characters may be blank.

### Interrupts and callbacks

Timer hardware determines ADC/DAC timing. DMA interrupts indicate buffer progress, and HAL dispatches the interrupt to the relevant callback. Keep callbacks short and move time-consuming work—FFT calculation, string formatting, UART printing and TFT drawing—into tasks.

## 7. Build, flash and run

1. Clone the repository and open the STM32CubeIDE project.
2. Use STM32CubeIDE 2.2.0 and select the NUCLEO-F446RE / STM32F446RE target.
3. Check the `.ioc` file if modifying pin assignments, clocks or peripheral configuration.
4. Build the project in CubeIDE.
5. Connect the Nucleo over USB and flash/debug through ST-LINK.
6. Open the USART2 virtual COM port at **115200 baud, 8-N-1**.
7. Connect the AHT20/BMP280 module to PB8/PB9 and the TFT to SPI2/control pins as listed above.
8. For ADC tests, connect the signal source to PA0 and share ground. Ensure the full input waveform remains within the allowed input voltage range.
9. Observe the DAC waveform at PA4 and PWM signal at PA6 with an oscilloscope.

Example serial-terminal command (replace `/dev/ttyACM0` with the actual device):

```bash
screen /dev/ttyACM0 115200
```

To exit `screen`, press `Ctrl-A`, then `K`, and confirm.

### Re-generating CubeMX code

Before regenerating code, commit or back up the working source. Review the Git diff after generation and rebuild: custom code outside CubeMX user-code sections can be overwritten, and middleware/peripheral settings can change.

## 8. Known limitations and next steps

- The TFT font is small and supports only a limited set of characters.
- The TFT SD-card socket is not implemented.
- The TFT currently displays text values; a plotted FFT spectrum/bar graph is a possible extension.
- Queue overflow handling could be improved with counters and diagnostic messages.
- A more complete error strategy could track sensor failures, missed ADC processing deadlines and queue overflows.
- The hardware watchdog should only be enabled with a deliberate refresh and task-health strategy.
- Useful learning extensions include drawing primitives for the TFT, FFT spectrum plotting, measuring worst-case FFT execution time, and systematically testing queue/task timing.

## 9. Repository

<https://github.com/srmt1/STM32_Signal_Processing>
