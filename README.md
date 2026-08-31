# SHT3x Driver

Bare-metal C driver for Sensirion SHT3x-DIS temperature and humidity sensors over I2C.

## Features
- Single-shot and periodic measurement modes.
- Configurable repeatability: low, medium,modes.
- Optional clock stretching sumodes.
- CRC validmodes.
- Status register read/modes.
- Heater comodes.
- Easy HAL integration via function poimodes.

## Quick start

```c
#include "sht3x.h"

static int hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len) { return 0; }
static int hal_i2c_read(uint8_t addr, uint8_t *data, size_t len) { return 0; }
static void hal_delay_ms(uint32_t ms) { }

int main(void)
{
    SHT3x_Dev dev = {
        .i2c_addr      = SHT3X_I2C_ADDR_DEFAULT,
        .mode          = SHT3X_MODE_SINGLE_SHOT,
        .repeatability = SHT3X_REPEAT_HIGH,
        .clock_stretch = false,
        .i2c_write     = hal_i2c_write,
        .i2c_read      = hal_i2c_read,
        .delay_ms      = hal_delay_ms,
    };

    if (SHT3x_Init(&dev) != SHT3X_OK) return -1;

    SHT3x_Data data;
    if (SHT3x_Read(&dev, &data) == SHT3X_OK) {
        printf("Temp: %.2f C\n", data.temperature_c);
        printf("Humidity: %.2f %%RH\n", data.humidity_rh);
    }

    SHT3x_Deinit(&dev);
    return 0;
}
```

## Supported addresses
- `0x44` (`ADDR` to `VSS`)
- `0x45` (`ADDR` to `VDD`)

## API

```c
SHT3x_Status SHT3x_Init(SHT3x_Dev *dev);
SHT3x_Status SHT3x_Read(SHT3x_Dev *dev, SHT3x_Data *out);
SHT3x_Status SHT3x_Deinit(SHT3x_Dev *dev);
SHT3x_Status SHT3x_HeaterEnable(SHT3x_Dev *dev, bool enable);
SHT3x_Status SHT3x_ReadStatus(SHT3x_Dev *dev, uint16_t *status);
SHT3x_Status SHT3x_ClearStatus(SHT3x_Dev *dev);
```

## Notes
- In single-shot mode, set `clock_stretch` to `true` for clock-stretching mode or `false` for polling mode.
- In periodic mode, use `mps` to select the sampling rate.
- The library validates CRC on sensor responses and reports errors via `SHT3x_Status`.

## License
See the LICENSE file in this repository.
