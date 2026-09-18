
/**
 * @file    sht3x.c
 * @brief   Bare-metal driver implementation for Sensirion SHT3x-DIS.
 * @version 2.0
 *
 * @details Datasheet: SHT3x-DIS, August 2016 – Version 3.
 */
#include "sht3x.h"
#include <stdint.h>
#include <stdbool.h>

#include "sht3x_defs.h"


SHT3x_Status SHT3x_CreateDefaultSensor(SHT3x_Sensor *s) {
    if(!s) {
        return SHT3X_ERROR_INVALID_ARGS;
    }
    s->config->mode                             = SHT3X_MODE_PERIODIC;
    s->config->i2c_address                      = SHT3X_I2C_ADDR_VSS;
    s->config->repeatability                    = SHT3X_REPEAT_HIGH;
    s->config->mode_cfg.periodic.meas_per_sec   = SHT3X_MPS_1;
    s->config->mode_cfg.periodic.art_enabled    = false;
    s->hal->i2c_write                           = NULL;
    s->hal->i2c_read                            = NULL;
    s->hal->delay_ms                            = NULL;
    return SHT3X_OK;
}

void SHT3x_SetMode(SHT3x_Sensor *s, SHT3x_Mode m) {
    s->config->mode = m;
}

void SHT3x_ConfigSetI2cAddress(SHT3x_Sensor *s, SHT3x_I2cAddress a) {
    s->config->i2c_address = a;
}

void SHT3x_ConfigSetRepeatability(SHT3x_Sensor *s, SHT3x_Repeatability r) {
    s->config->repeatability = r;
}

SHT3x_Status SHT3x_ConfigSetMps(SHT3x_Sensor *s, SHT3x_MPS mps) {
    if(s->config->mode != SHT3X_MODE_PERIODIC) {
        return SHT3X_ERROR_INVALID_MODE;
    }
    s->config->mode_cfg.periodic.meas_per_sec = mps;
    return SHT3X_OK;
}

SHT3x_Status SHT3x_ConfigSetArtEnable(SHT3x_Sensor *s, bool on_off) {
    if(s->config->mode != SHT3X_MODE_PERIODIC) {
        return SHT3X_ERROR_INVALID_MODE;
    }
    s->config->mode_cfg.periodic.art_enabled = on_off;
    return SHT3X_OK;
}

SHT3x_Status SHT3x_ConfigSetClockStretch(SHT3x_Sensor *s, bool on_off) {
    if(s->config->mode != SHT3X_MODE_SINGLE_SHOT) {
        return SHT3X_ERROR_INVALID_MODE;
    }
    s->config->mode_cfg.singleshot.clock_stretch = on_off;
    return SHT3X_OK;
}

void SHT3x_HalSetI2cWrite(SHT3x_Sensor *s, SHT3x_I2cWrite i2c_write) {
    s->hal->i2c_write = i2c_write;
}

void SHT3x_HalSetI2cRead(SHT3x_Sensor *s, SHT3x_I2cRead i2c_read) {
    s->hal->i2c_read = i2c_read;
}

void SHT3x_HalSetDelayMs(SHT3x_Sensor *s, SHT3x_DelayMs delay_ms) {
    s->hal->delay_ms = delay_ms;
}


/* =========================================================================
 * @defgroup SHT3X_CRC CRC-8 Parameters
 * @brief    CRC parameters for data integrity verification (Table 19).
 *           Polynomial: 0x31 (x^8 + x^5 + x^4 + 1), Initial value: 0xFF.
 * @{
 * ========================================================================= */

#define SHT3X_CRC_POLY                  0x31u /**< CRC-8 polynomial: x^8 + x^5 + x^4 + 1 */
#define SHT3X_CRC_INIT                  0xFFu /**< CRC-8 initial value     */

/* =========================================================================
 * @defgroup SHT3X_PRIVATE_DEFS Private Constants
 * @brief    Internal buffer sizing constants, not exposed in the public API.
 * @{
 * ========================================================================= */

#define SHT3X_MAX_WORDS  2u                         /**< Max data words per read transaction         */
#define SHT3X_BUF_SIZE   (SHT3X_MAX_WORDS * 3u)     /**< Raw byte buffer size: 2 bytes + 1 CRC/word  */

/** @} */ /* end group SHT3X_PRIVATE_DEFS */

/* =========================================================================
 * @defgroup SHT3X_PRIVATE Private Helper Functions
 * @brief    Internal functions not exposed through the public API.
 * @{
 * ========================================================================= */

/**
 * @brief   Compute CRC-8 checksum over a byte array.
 * @details Uses polynomial 0x31 (x^8 + x^5 + x^4 + 1) with initial value
 *          0xFF, as specified in datasheet Table 19.
 *
 * @param[in] data  Pointer to input byte array.
 * @param[in] len   Number of bytes to process.
 * @return          Computed CRC-8 value.
 */
static uint8_t _sht3x_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = SHT3X_CRC_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80u) ? ((uint8_t)(crc << 1) ^ SHT3X_CRC_POLY) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/**
 * @brief   Send a 16-bit command word over I2C (MSB first).
 *
 * @param[in] s    Pointer to sensor handle.
 * @param[in] cmd  16-bit command code to transmit.
 * @return         @ref SHT3X_OK on success, @ref SHT3X_ERROR_I2C on bus failure.
 */
static SHT3x_Status _sht3x_send_cmd(SHT3x_Sensor *s, uint16_t cmd)
{
    uint8_t buf[2] = {
        (uint8_t)(cmd >> 8),
        (uint8_t)(cmd & 0xFF)
    };
    return (s->hal->i2c_write(s->config->i2c_address, buf, 2) == 0) ? SHT3X_OK : SHT3X_ERROR_I2C;
}

/**
 * @brief   Send a 16-bit command word and block for a fixed execution delay.
 * @details Combines @ref _sht3x_send_cmd with a mandatory post-command wait.
 *          The delay is skipped if the command fails.
 *
 * @param[in] s        Pointer to sensor handle.
 * @param[in] cmd      16-bit command code to transmit.
 * @param[in] exec_ms  Execution time to wait after a successful send (ms).
 * @return             @ref SHT3X_OK on success, @ref SHT3X_ERROR_I2C on bus failure.
 */
static SHT3x_Status _sht3x_send_cmd_with_delay(SHT3x_Sensor *s, uint16_t cmd, uint32_t exec_ms)
{
    SHT3x_Status st = _sht3x_send_cmd(s, cmd);
    if (st == SHT3X_OK) {
        s->hal->delay_ms(exec_ms);
    }
    return st;
}

/**
 * @brief   Read @p n_words data words from the sensor, each followed by a
 *          CRC-8 byte, and verify integrity before storing.
 * @details Reads <tt>n_words × 3</tt> raw bytes, verifies the CRC of each
 *          16-bit word, and stores the decoded values in @p words.
 *
 * @param[in]  s        Pointer to sensor handle.
 * @param[out] words    Output buffer; must hold at least @p n_words elements.
 * @param[in]  n_words  Number of words to read (1 .. @ref SHT3X_MAX_WORDS).
 * @return              @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p n_words is 0 or exceeds @ref SHT3X_MAX_WORDS.
 * @retval  SHT3X_ERROR_I2C    if the I2C read transaction fails.
 * @retval  SHT3X_ERROR_CRC    if any word fails its CRC check.
 */
static SHT3x_Status _sht3x_read_words(SHT3x_Sensor *s, uint16_t *words, size_t n_words)
{
    if (n_words == 0u || n_words > SHT3X_MAX_WORDS) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    uint8_t buf[SHT3X_BUF_SIZE];

    if (s->hal->i2c_read(s->config->i2c_address, buf, n_words * 3u) != 0) {
        return SHT3X_ERROR_I2C;
    }

    for (size_t i = 0; i < n_words; i++) {
        uint8_t *p = &buf[i * 3u];
        if (_sht3x_crc8(p, 2) != p[2]) {
            return SHT3X_ERROR_CRC;
        }
        words[i] = ((uint16_t)p[0] << 8) | p[1];
    }

    return SHT3X_OK;
}

/**
 * @brief   Resolve the single-shot command code for the requested
 *          repeatability and clock-stretching configuration.
 *
 * @param[in] rep  Repeatability level (@ref SHT3x_Repeatability).
 * @param[in] clock_stretch_on   @c true to select the clock-stretching variant.
 * @return         16-bit command code corresponding to the requested settings.
 */
static uint16_t _sht3x_read_singleshot_cmd(SHT3x_Repeatability rep, bool clock_stretch_on)
{
    if (clock_stretch_on) {
        switch (rep) {
            case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_READ_SINGLESHOT_CS_HIGH;
            case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_READ_SINGLESHOT_CS_MED;
            default:                  return SHT3X_CMD_READ_SINGLESHOT_CS_LOW;
        }
    } else {
        switch (rep) {
            case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_READ_SINGLESHOT_HIGH;
            case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_READ_SINGLESHOT_MED;
            default:                  return SHT3X_CMD_READ_SINGLESHOT_LOW;
        }
    }
}

/**
 * @brief   Resolve the periodic-mode command code for the requested
 *          measurement rate and repeatability.
 *
 * @param[in] mps  Measurement rate (@ref SHT3x_MPS).
 * @param[in] rep  Repeatability level (@ref SHT3x_Repeatability).
 * @return         16-bit command code corresponding to the requested settings.
 */
static uint16_t _sht3x_start_periodic_cmd(SHT3x_MPS mps, SHT3x_Repeatability rep)
{
    switch (mps) {
        case SHT3X_MPS_05:
            switch (rep) {
                case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_START_PERIODIC_0_5_HIGH;
                case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_START_PERIODIC_0_5_MED;
                default:                  return SHT3X_CMD_START_PERIODIC_0_5_LOW;
            }
        case SHT3X_MPS_1:
            switch (rep) {
                case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_START_PERIODIC_1_HIGH;
                case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_START_PERIODIC_1_MED;
                default:                  return SHT3X_CMD_START_PERIODIC_1_LOW;
            }
        case SHT3X_MPS_2:
            switch (rep) {
                case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_START_PERIODIC_2_HIGH;
                case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_START_PERIODIC_2_MED;
                default:                  return SHT3X_CMD_START_PERIODIC_2_LOW;
            }
        case SHT3X_MPS_4:
            switch (rep) {
                case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_START_PERIODIC_4_HIGH;
                case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_START_PERIODIC_4_MED;
                default:                  return SHT3X_CMD_START_PERIODIC_4_LOW;
            }
        default: /* SHT3X_MPS_10 */
            switch (rep) {
                case SHT3X_REPEAT_HIGH:   return SHT3X_CMD_START_PERIODIC_10_HIGH;
                case SHT3X_REPEAT_MEDIUM: return SHT3X_CMD_START_PERIODIC_10_MED;
                default:                  return SHT3X_CMD_START_PERIODIC_10_LOW;
            }
    }
}

/**
 * @brief   Return the required post-trigger measurement wait time for
 *          no-clock-stretch single-shot mode.
 *
 * @param[in] rep  Repeatability level (@ref SHT3x_Repeatability).
 * @return         Required delay in milliseconds.
 */
static uint32_t _sht3x_meas_delay_ms(SHT3x_Repeatability rep)
{
    switch (rep) {
        case SHT3X_REPEAT_HIGH:   return SHT3X_EXEC_MEAS_HIGH_MS;
        case SHT3X_REPEAT_MEDIUM: return SHT3X_EXEC_MEAS_MED_MS;
        default:                  return SHT3X_EXEC_MEAS_LOW_MS;
    }
}

/**
 * @brief   Validate all fields of a device handle before use.
 * @details Checks that: HAL function pointers are non-NULL; the I2C address
 *          is one of the two valid values; and the mode, repeatability, and
 *          MPS enumerators are within their legal ranges.
 *
 * @param[in] s    Pointer to the device handle to validate.
 * @return         @c true if the handle is fully valid, @c false otherwise.
 */
static bool _sht3x_is_valid(SHT3x_Sensor *s)
{
    if (!s || !(s->hal) || !(s->hal->delay_ms) || !(s->hal->i2c_read) || !(s->hal->i2c_write)) {
        return false;
    }
    if(!(s->config)) {
        return false;
    }
    if (s->config->i2c_address != SHT3X_I2C_ADDR_VDD && s->config->i2c_address != SHT3X_I2C_ADDR_VSS) {
        return false;
    }
    if (s->config->mode != SHT3X_MODE_SINGLE_SHOT && s->config->mode != SHT3X_MODE_PERIODIC) {
        return false;
    }
    if (s->config->repeatability != SHT3X_REPEAT_LOW &&
        s->config->repeatability != SHT3X_REPEAT_MEDIUM &&
        s->config->repeatability != SHT3X_REPEAT_HIGH) {
        return false;
    }
    if (s->config->mode == SHT3X_MODE_PERIODIC && s->config->mode_cfg.periodic.meas_per_sec > SHT3X_MPS_10) {
        return false;
    }
    return true;
}

/** @} */ /* end group SHT3X_PRIVATE */

/* =========================================================================
 * @defgroup SHT3X_PUBLIC Public API
 * @brief    Functions exposed through sht3x.h for application use.
 * @{
 * ========================================================================= */



SHT3x_Status SHT3x_StartPeriodicMeasurement(SHT3x_Sensor *s) {
    if(!_sht3x_is_valid(s)) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    if(s->config->mode != SHT3X_MODE_PERIODIC) {
        return SHT3X_ERROR_INVALID_MODE;
    }
    
    SHT3x_Status st;

    /* Power up. */
    s->hal->delay_ms(SHT3X_EXEC_POWERUP_MS);

    /* Reset. */
    st = _sht3x_send_cmd_with_delay(s, SHT3X_CMD_SOFT_RESET, SHT3X_EXEC_SOFT_RESET_MS);
    if (st != SHT3X_OK) {
        return st;
    }

    /* Start periodic. */
    uint16_t cmd;
    if(s->config->mode_cfg.periodic.art_enabled) {
        cmd = SHT3X_CMD_START_PERIODIC_ART;
    } else {
        cmd = _sht3x_start_periodic_cmd(s->config->mode_cfg.periodic.meas_per_sec, s->config->repeatability);
    }
    return _sht3x_send_cmd_with_delay(s, cmd, SHT3X_MIN_CMD_INTERVAL_MS);
}

SHT3x_Status SHT3x_StopPeriodicMeasurement(SHT3x_Sensor *s) {
    if(!_sht3x_is_valid(s)) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    return _sht3x_send_cmd_with_delay(s, SHT3X_CMD_BREAK, SHT3X_EXEC_BREAK_MS);
}

static SHT3x_Status SHT3x_ReadPeriodicMeasurement(SHT3x_Sensor *s, SHT3x_Data *d) {
    if(!_sht3x_is_valid(s) || !d) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    return _sht3x_send_cmd_with_delay(s, SHT3X_CMD_FETCH_DATA, SHT3X_MIN_CMD_INTERVAL_MS);
}

static SHT3x_Status SHT3x_ReadSingleShotMeasurement(SHT3x_Sensor *s, SHT3x_Data *d) {
    if(!_sht3x_is_valid || !d) {
        return SHT3X_ERROR_INVALID_ARGS;
    }
    bool clock_stretch = s->config->mode_cfg.singleshot.clock_stretch;
    uint16_t cmd = _sht3x_read_singleshot_cmd(s->config->repeatability, clock_stretch);

    if(clock_stretch) {
        return _sht3x_send_cmd_with_delay(s, cmd, SHT3X_MIN_CMD_INTERVAL_MS);
    } else {
        uint32_t exec_ms = _sht3x_meas_delay_ms(s->config->repeatability);
        return _sht3x_send_cmd_with_delay(s, cmd, exec_ms);
    }

    return SHT3X_OK;
}

SHT3x_Status SHT3x_ReadMeasurement(SHT3x_Sensor *s, SHT3x_Data *d) {
    if(!s || !d) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    SHT3x_Status st = SHT3X_OK;
    if(s->config->mode == SHT3X_MODE_PERIODIC) {
        st = SHT3x_ReadPeriodicMeasurement(s, d);
    } else {
        st = SHT3x_ReadSingleShotMeasurement(s, d);
    }
    if(st != SHT3X_OK) {
        return st;
    }

    /* CRC check. */
    uint16_t words[SHT3X_MAX_WORDS];
    st = _sht3x_read_words(s, words, 2u);
    if(st != SHT3X_OK) {
        return st;
    }

    /* Convert raw data. */
    d->temperature_c = SHT3X_TEMP_C(words[0]);
    d->humidity_rh   = SHT3X_HUMID_RH(words[1]);

    return SHT3X_OK;
}

SHT3x_Status SHT3x_HeaterEnable(SHT3x_Sensor *s, bool on_off) {
    if(!s) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    return _sht3x_send_cmd_with_delay(s, on_off ? SHT3X_CMD_HEATER_ON : SHT3X_CMD_HEATER_OFF, SHT3X_MIN_CMD_INTERVAL_MS);
}


SHT3x_Status SHT3x_SoftReset(SHT3x_Sensor *s) {
    if(!_sht3x_is_valid(s)) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    return _sht3x_send_cmd_with_delay(s, SHT3X_CMD_SOFT_RESET, SHT3X_EXEC_SOFT_RESET_MS);
}

/* Implement nRESET. */
// #define SHT3X_NRESET_EXEC_US    1
// typedef int  (*SHT3x_GpioWrite)(SHT3x_Gpio gpio, bool level);
// typedef void (*SHT3x_DelayUs)  (uint32_t *us);

// typedef struct {
//     struct {
//         SHT3x_Gpio      nreset;         /**< Can be NULL, used in case need hard reset. */
//     } gpio;
//     SHT3x_GpioWrite     gpio_write;     /**< Can be NULL, use for NReset. */
//     SHT3x_I2cRead       i2c_read;       /**< Platform I2C read. Must not be NULL. */
//     SHT3x_I2cWrite      i2c_write;      /**< Platform I2C write. Must not be NULL. */
//     SHT3x_DelayMs       delay_ms;       /**< Platform delay in ms. Must not be NULL. */
//     SHT3x_DelayUs       delay_us;
//     struct {
//         uint8_t nreset_active_level : 1; /**< nReset pin active low by default (set low level to reset) */
//     } flags;
// } SHT3x_Hal;

// SHT3x_Status SHT3x_HalSetNResetLevel(SHT3x_Sensor *s, uint8_t level) {
//     if(!s || (level != 0 && level != 1)) {
//         return SHT3X_ERROR_INVALID_ARGS;
//     }
//     s->hal->flags.nreset_active_level = level;
//     return SHT3X_OK;
// }

// /**
//  * @brief Reset through nRESET.
//  * @param s pointer to sensor handle.
//  * @return `SHT3x_Status` code.
//  * @retval `SHT3X_OK` on success.
//  * @retval `SHT3X_ERROR_INVALID_ARGS` in case invalid `s` handle or nRESET 
//  * pin was not configured through `SHT3x_HalSetNResetGpio`.
//  */
// SHT3x_Status SHT3x_NReset(SHT3x_Sensor *s) {
//     if(!s || (s->hal->gpio.nreset.pin == -1) || !(s->hal->gpio_write)) {
//         return SHT3X_ERROR_INVALID_ARGS;
//     }

//     s->hal->gpio_write(s->hal->gpio.nreset, s->hal->flags.nreset_active_level);
//     s->hal->delay_us(SHT3X_NRESET_EXEC_US);
//     s->hal->gpio_write(s->hal->gpio.nreset, !(s->hal->flags.nreset_active_level));
//     return SHT3X_OK;
// }

/**
 * @brief   Read the 16-bit device status register.
 * @details Sends @ref SHT3X_CMD_READ_STATUS, then reads one word with CRC
 *          verification. The raw register value is returned in @p status;
 *          use the @c SHT3X_SREG_* bitmasks from @ref sht3x_defs.h to decode
 *          individual flag bits.
 *
 * @param[in]  s       Pointer to an initialised @ref SHT3x_Sensor handle.
 * @param[out] status  Pointer to receive the 16-bit status register value.
 * @return             @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS    if @p dev or @p status is NULL.
 * @retval  SHT3X_ERROR_I2C             if any I2C transaction fails.
 * @retval  SHT3X_ERROR_CRC             if the response fails CRC verification.
 */
SHT3x_Status SHT3x_ReadStatusRegister(SHT3x_Sensor *s, SHT3x_StatusRegister *status_register)
{
    if (!s || !status_register) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    SHT3x_Status ret = _sht3x_send_cmd_with_delay(s, SHT3X_CMD_READ_STATUS, SHT3X_MIN_CMD_INTERVAL_MS);
    if (ret != SHT3X_OK) {
        return ret;
    }

    uint16_t words[1];
    ret = _sht3x_read_words(s, words, 1u);
    if (ret != SHT3X_OK) {
        return ret;
    }

    status_register->raw = words[0];
    return SHT3X_OK;
}

SHT3x_Status SHT3x_ClearStatusRegister(SHT3x_Sensor *s) {
    if(!s) {
        return SHT3X_ERROR_INVALID_ARGS;
    }

    return _sht3x_send_cmd_with_delay(s, SHT3X_CMD_CLEAR_STATUS, SHT3X_MIN_CMD_INTERVAL_MS);
}
