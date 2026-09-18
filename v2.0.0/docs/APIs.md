# API Reference: SHT3x Driver

Bare-metal driver for the Sensirion SHT3x-DIS I2C temperature/humidity sensor. Supports single-shot and periodic measurement modes, clock-stretching and non-clock-stretching reads, on-chip heater control, and status register access. Communication is abstracted through platform-injected I2C/delay callbacks (HAL injection), making the driver portable across MCUs.

## Table of Contents
1. [Data Types & Structures](#data-types--structures)
2. [Function API Reference](#function-api-reference)
3. [Status & Return Codes](#status--return-codes)

## Data Types & Structures

### `SHT3x_Sensor`
Main driver handle. Aggregates a pointer to the user-owned configuration and HAL callback structures; does not own the memory itself.

```c
typedef struct {
    SHT3x_Config *config;
    SHT3x_Hal    *hal;
} SHT3x_Sensor;
```

### `SHT3x_Config`
Sensor configuration: operating mode, I2C address, repeatability, and mode-specific settings (periodic rate/ART, or single-shot clock stretch).

```c
typedef struct {
    SHT3x_Mode          mode;           /**< Measurement mode.                */
    SHT3x_I2cAddress    i2c_address;    /**< 7-bit I2C device address.        */
    SHT3x_Repeatability repeatability;  /**< Measurement repeatability level. */
    union {
        struct {
            SHT3x_MPS   meas_per_sec;   /**< Periodic measurement rate. Periodic mode only. */
            bool        art_enabled;    /**< Enable accelerated response time. Periodic mode only. */
        } periodic;
        struct {
            bool        clock_stretch;  /**< Clock stretching. Single-shot mode only. */
        } singleshot;
    } mode_cfg;
} SHT3x_Config;
```

### `SHT3x_Hal`
Platform HAL callback table. All three fields must be set before any I2C operation is performed.

```c
typedef struct {
    SHT3x_I2cRead   i2c_read;   /**< Platform I2C read. Must not be NULL.  */
    SHT3x_I2cWrite  i2c_write;  /**< Platform I2C write. Must not be NULL. */
    SHT3x_DelayMs   delay_ms;   /**< Platform delay in ms. Must not be NULL. */
} SHT3x_Hal;
```

### `SHT3x_MeasurementData`
Decoded measurement result populated by `SHT3x_ReadMeasurement`.

```c
typedef struct {
    float temperature_c;  /**< Temperature in degrees Celsius     */
    float humidity_rh;    /**< Relative humidity in percent (%RH) */
} SHT3x_MeasurementData;
```

### `SHT3x_StatusRegister`
Decoded view of the 16-bit status register. Bitfield layout is compiler/ABI dependent — prefer decoding `.raw` with the `SHT3X_SREG_*` bitmasks (in `sht3x_defs.h`) for portable code.

```c
typedef union {
    uint16_t raw;
    struct {
        uint16_t write_data_checksum_status : 1;
        uint16_t command_status             : 1;
        uint16_t reserved_3_2               : 2;
        uint16_t system_reset_detected      : 1;
        uint16_t reserved_9_5               : 5;
        uint16_t t_tracking_alert           : 1;
        uint16_t rh_tracking_alert          : 1;
        uint16_t reserved_12                : 1;
        uint16_t heater_status              : 1;
        uint16_t reserved_14                : 1;
        uint16_t alert_pending_status       : 1;
    } bits;
} SHT3x_StatusRegister;
```

### Enumerations

```c
typedef enum {
    SHT3X_I2C_ADDR_VSS = 0x44u,  /**< ADDR pin tied to VSS (default) */
    SHT3X_I2C_ADDR_VDD = 0x45u   /**< ADDR pin tied to VDD           */
} SHT3x_I2cAddress;

typedef enum {
    SHT3X_MODE_SINGLE_SHOT = 0,  /**< On-demand single measurement per read call */
    SHT3X_MODE_PERIODIC    = 1   /**< Sensor samples autonomously at configured MPS */
} SHT3x_Mode;

typedef enum {
    SHT3X_REPEAT_LOW    = 0,
    SHT3X_REPEAT_MEDIUM = 1,
    SHT3X_REPEAT_HIGH   = 2
} SHT3x_Repeatability;

typedef enum {
    SHT3X_MPS_05 = 0,  /**< 0.5 measurements/sec */
    SHT3X_MPS_1  = 1,
    SHT3X_MPS_2  = 2,
    SHT3X_MPS_4  = 3,
    SHT3X_MPS_10 = 4
} SHT3x_MPS;
```

### Callback Function Types
Hardware abstraction callbacks the application must provide, so the driver stays portable across I2C peripherals/platforms.

```c
/** Platform I2C write callback. Returns 0 on success. */
typedef int  (*SHT3x_I2cWrite)(uint8_t address, const uint8_t *data, size_t len);

/** Platform I2C read callback. Returns 0 on success. */
typedef int  (*SHT3x_I2cRead) (uint8_t address, uint8_t *data, size_t len);

/** Platform blocking delay callback, in milliseconds. */
typedef void (*SHT3x_DelayMs) (uint32_t ms);
```

## Function API Reference

### Initialization & Lifecycle

#### `SHT3x_CreateDefaultSensor`
```c
SHT3x_Status SHT3x_CreateDefaultSensor(SHT3x_Sensor *s);
```
- `Description`: Initializes a sensor handle with default configuration: periodic mode, I2C address `SHT3X_I2C_ADDR_VSS`, high repeatability, 1 measurement/sec, ART disabled. HAL callback pointers are cleared to `NULL`.
- `Parameters`:
  - `s`: Pointer to sensor handle; `s->config` and `s->hal` must already point to valid, caller-owned storage.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s`, `s->config`, or `s->hal` is `NULL`.
- `Note`: HAL write/read/delay callbacks must still be set via `SHT3x_HalSetI2cWrite`, `SHT3x_HalSetI2cRead`, and `SHT3x_HalSetDelayMs` before any operation is performed.

> No explicit deinit function is provided — the driver holds no dynamically allocated resources of its own (handle storage is caller-owned).

### Configuration APIs

#### `SHT3x_SetMode`
```c
void SHT3x_SetMode(SHT3x_Sensor *s, SHT3x_Mode m);
```
- `Description`: Sets the measurement mode.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `m`: Desired mode (`SHT3X_MODE_PERIODIC` by default).
- `Returns`: None.
- `Note`: Does not validate `s`/`s->config` for `NULL`; caller must ensure the handle is initialized first.

#### `SHT3x_ConfigSetI2cAddress`
```c
void SHT3x_ConfigSetI2cAddress(SHT3x_Sensor *s, SHT3x_I2cAddress a);
```
- `Description`: Sets the 7-bit I2C device address.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `a`: I2C address (`SHT3X_I2C_ADDR_VSS` or `SHT3X_I2C_ADDR_VDD`).
- `Returns`: None.

#### `SHT3x_ConfigSetRepeatability`
```c
void SHT3x_ConfigSetRepeatability(SHT3x_Sensor *s, SHT3x_Repeatability r);
```
- `Description`: Sets measurement repeatability (accuracy vs. conversion time trade-off).
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `r`: Repeatability level.
- `Returns`: None.

#### `SHT3x_ConfigSetMps`
```c
SHT3x_Status SHT3x_ConfigSetMps(SHT3x_Sensor *s, SHT3x_MPS mps);
```
- `Description`: Sets the periodic-mode measurement rate.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `mps`: Measurement rate.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_MODE` if the configured mode is not `SHT3X_MODE_PERIODIC`.

#### `SHT3x_ConfigSetArtEnable`
```c
SHT3x_Status SHT3x_ConfigSetArtEnable(SHT3x_Sensor *s, bool on_off);
```
- `Description`: Enables/disables ART (Accelerated Response Time), a fixed 4 Hz high-repeatability periodic mode.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `on_off`: `true` to enable, `false` to disable.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_MODE` if the configured mode is not `SHT3X_MODE_PERIODIC`.

#### `SHT3x_ConfigSetClockStretch`
```c
SHT3x_Status SHT3x_ConfigSetClockStretch(SHT3x_Sensor *s, bool on_off);
```
- `Description`: Enables/disables I2C clock stretching for single-shot reads.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `on_off`: `true` to enable, `false` to disable.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_MODE` if the configured mode is not `SHT3X_MODE_SINGLE_SHOT`.

#### `SHT3x_HalSetI2cWrite`
```c
void SHT3x_HalSetI2cWrite(SHT3x_Sensor *s, SHT3x_I2cWrite i2c_write);
```
- `Description`: Registers the platform I2C write callback.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `i2c_write`: Platform I2C write function. Must not be `NULL` before use.
- `Returns`: None.

#### `SHT3x_HalSetI2cRead`
```c
void SHT3x_HalSetI2cRead(SHT3x_Sensor *s, SHT3x_I2cRead i2c_read);
```
- `Description`: Registers the platform I2C read callback.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `i2c_read`: Platform I2C read function. Must not be `NULL` before use.
- `Returns`: None.

#### `SHT3x_HalSetDelayMs`
```c
void SHT3x_HalSetDelayMs(SHT3x_Sensor *s, SHT3x_DelayMs delay_ms);
```
- `Description`: Registers the platform blocking millisecond delay callback.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `delay_ms`: Platform delay function. Must not be `NULL` before use.
- `Returns`: None.

### Control & Operation APIs

#### `SHT3x_StartPeriodicMeasurement`
```c
SHT3x_Status SHT3x_StartPeriodicMeasurement(SHT3x_Sensor *s);
```
- `Description`: Powers up, soft-resets, then starts periodic measurement at the configured rate/repeatability (or ART, if enabled).
- `Parameters`:
  - `s`: Pointer to a fully configured sensor handle.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s` is `NULL`/not fully configured; `SHT3X_ERROR_INVALID_MODE` if mode is not `SHT3X_MODE_PERIODIC`; `SHT3X_ERROR_I2C` if any I2C transaction fails.

#### `SHT3x_StopPeriodicMeasurement`
```c
SHT3x_Status SHT3x_StopPeriodicMeasurement(SHT3x_Sensor *s);
```
- `Description`: Sends the break command, returning the sensor to idle/single-shot state.
- `Parameters`:
  - `s`: Pointer to sensor handle.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s` is `NULL`/not fully configured; `SHT3X_ERROR_I2C` if the I2C transaction fails.

#### `SHT3x_ReadMeasurement`
```c
SHT3x_Status SHT3x_ReadMeasurement(SHT3x_Sensor *s, SHT3x_MeasurementData *d);
```
- `Description`: Reads a temperature/humidity measurement. In periodic mode, fetches the latest pending result; in single-shot mode, triggers a new measurement and waits for completion before reading.
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `d`: Pointer to receive the decoded temperature/humidity.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s`/`d` is `NULL`; `SHT3X_ERROR_I2C` if any I2C transaction fails; `SHT3X_ERROR_CRC` if the response fails CRC verification.

#### `SHT3x_HeaterEnable`
```c
SHT3x_Status SHT3x_HeaterEnable(SHT3x_Sensor *s, bool on_off);
```
- `Description`: Enables or disables the on-chip heater (used for condensation testing/removal — not for regulating ambient readings).
- `Parameters`:
  - `s`: Pointer to sensor handle.
  - `on_off`: `true` to enable, `false` to disable.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s` is `NULL`/not fully configured; `SHT3X_ERROR_I2C` if the I2C transaction fails.

#### `SHT3x_SoftReset`
```c
SHT3x_Status SHT3x_SoftReset(SHT3x_Sensor *s);
```
- `Description`: Performs a soft reset via the I2C interface, without a full power cycle.
- `Parameters`:
  - `s`: Pointer to sensor handle.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s` is `NULL`/not fully configured; `SHT3X_ERROR_I2C` if the I2C transaction fails.

#### `SHT3x_ReadStatusRegister`
```c
SHT3x_Status SHT3x_ReadStatusRegister(SHT3x_Sensor *s, SHT3x_StatusRegister *status_register);
```
- `Description`: Reads the 16-bit device status register with CRC verification. Use the `SHT3X_SREG_*` bitmasks to decode individual flags from `status_register->raw`.
- `Parameters`:
  - `s`: Pointer to an initialized sensor handle.
  - `status_register`: Pointer to receive the 16-bit status register value.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s`/`status_register` is `NULL`; `SHT3X_ERROR_I2C` if the I2C transaction fails; `SHT3X_ERROR_CRC` if the response fails CRC verification.

#### `SHT3x_ClearStatusRegister`
```c
SHT3x_Status SHT3x_ClearStatusRegister(SHT3x_Sensor *s);
```
- `Description`: Clears all flags in the status register.
- `Parameters`:
  - `s`: Pointer to sensor handle.
- `Returns`: `SHT3X_OK` on success; `SHT3X_ERROR_INVALID_ARGS` if `s` is `NULL`/not fully configured; `SHT3X_ERROR_I2C` if the I2C transaction fails.

## Status & Return Codes

### `SHT3x_Status`

| Code / Value                | Value | Description                                         |
| :-------------------------- | :---- | :-------------------------------------------------- |
| `SHT3X_OK`                  | 0     | Operation completed successfully.                   |
| `SHT3X_ERROR_I2C`           | -1    | I2C bus communication error.                        |
| `SHT3X_ERROR_CRC`           | -2    | CRC-8 validation failed on received data.           |
| `SHT3X_ERROR_INVALID_ARGS`  | -3    | Invalid or out-of-range parameter (NULL/unconfigured handle). |
| `SHT3X_ERROR_INVALID_MODE`  | -4    | Operation not valid for the configured mode (e.g. calling a periodic-only setter in single-shot mode). |

### Status Register Bitmasks (`SHT3X_SREG_*`, in `sht3x_defs.h`)

| Bitmask                      | Bit | Description                          |
| :--------------------------- | :-- | :------------------------------------ |
| `SHT3X_SREG_ALERT_PENDING`   | 15  | At least one pending alert.           |
| `SHT3X_SREG_HEATER_ON`       | 13  | On-chip heater is enabled.            |
| `SHT3X_SREG_RH_ALERT`        | 11  | Humidity tracking alert active.       |
| `SHT3X_SREG_T_ALERT`         | 10  | Temperature tracking alert active.    |
| `SHT3X_SREG_RESET_DETECTED`  | 4   | Reset detected since last status clear. |
| `SHT3X_SREG_CMD_FAILED`      | 1   | Last command was not processed.       |
| `SHT3X_SREG_CRC_WRITE_FAIL`  | 0   | CRC mismatch on last write transfer.  |