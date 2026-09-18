# SHT3x Driver

[![hpbare](https://img.shields.io/badge/hpbare-library-blue.svg)](https://github.com/hpbare)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Bare-metal C driver for the Sensirion SHT3x-DIS digital temperature and humidity sensor. Supports single-shot and periodic measurement modes, clock-stretching and non-clock-stretching reads, on-chip heater control, and status register access, with I2C/delay operations injected through a platform HAL so the driver stays portable across MCUs.

## Overview
| Attribute                 | Specification                           |
| :------------------------ | :-------------------------------------- |
| `Target Device / Module`  | `Sensirion SHT3x-DIS`                   |
| `Interface / Protocol`    | `I2C` (7-bit address `0x44` / `0x45`)   |
| `Operating Voltage`       | `2.15V – 5.5V`                          |

## Features
- `Platform Independent`: Pure C99 logic without direct hardware/MCU dependencies.
- `Zero Dynamic Allocation`: Handles and buffers are caller-allocated.
- `Clean API Interface`: Well-defined public headers and error status codes.
- `Build System Integration`: Generic CMake build system supporting multi-platform subprojects.
- `CRC-8 Verified Reads`: Every measurement/status word is CRC-checked (poly 0x31) before being returned.
- `Dual Mode Support`: Single-shot (with/without clock stretch) and periodic (0.5–10 mps, with ART) measurement.

## Driver Structure
```
sht3x/
├── docs/               # Datasheets, APIs.md, etc.
├── examples/           # Sample usage code.
├── CMakeLists.txt      # CMake build configuration.
├── sht3x.h             # Main public header.
├── sht3x.c             # Core implementation.
├── sht3x_types.h       # Public types, enums, handle structs.
├── sht3x_defs.h        # Register map, command codes, bitmasks, timing constants.
├── CHANGELOG.md        # Change log & release notes.
├── README.md           # Driver description & overview.
└── LICENSE             # MIT License.
```

## Quick Start
### 1. Integration
Add the driver library to your project build configuration:

```cmake
add_subdirectory(path/to/sht3x)
target_link_libraries(your_app PRIVATE sht3x)
```

### 2. Usage Example
```c
#include "sht3x.h"
#include <stdio.h>

int main(void) {
    /* Initialize configuration & handles */
    SHT3x_Config config;
    SHT3x_Hal    hal;
    SHT3x_Sensor sensor = { .config = &config, .hal = &hal };

    SHT3x_CreateDefaultSensor(&sensor);
    SHT3x_HalSetI2cWrite(&sensor, platform_i2c_write);
    SHT3x_HalSetI2cRead(&sensor, platform_i2c_read);
    SHT3x_HalSetDelayMs(&sensor, platform_delay_ms);

    /* Call library APIs */
    SHT3x_StartPeriodicMeasurement(&sensor);  /* required once before reading in periodic mode */

    SHT3x_MeasurementData data;
    while (1) {
        platform_delay_ms(1000);  /* wait for at least one measurement interval (1 mps by default) */
        if (SHT3x_ReadMeasurement(&sensor, &data) == SHT3X_OK) {
            printf("Temp: %.2f C, RH: %.2f %%\n", data.temperature_c, data.humidity_rh);
        }
    }

    return 0;
}
```

## API Reference
For detailed function index, data structures, and status codes, see [APIs.md](docs/APIs.md).

## Changelog
For version history and release notes, see [CHANGELOG.md](CHANGELOG.md).

## License
This library is released under the `MIT License`. See [LICENSE](LICENSE) for details.