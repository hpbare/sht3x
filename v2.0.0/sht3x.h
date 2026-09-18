#ifndef SHT3X_H_
#define SHT3X_H_

#include "sht3x_types.h"

typedef enum {
    SHT3X_OK                 =  0,  /**< Operation completed successfully        */
    SHT3X_ERROR_I2C          = -1,  /**< I2C bus communication error             */
    SHT3X_ERROR_CRC          = -2,  /**< CRC-8 validation failed on received data */
    SHT3X_ERROR_INVALID_ARGS = -3,  /**< Invalid or out-of-range parameter        */
    SHT3X_ERROR_NOT_INIT     = -4,  /**< Driver has not been initialised          */
    SHT3X_ERROR_INVALID_MODE = -5
} SHT3x_Status;

typedef struct {
    SHT3x_Config    *config;
    SHT3x_Hal       *hal;
    // SHT3x_Internal  internal;
} SHT3x_Sensor;

SHT3x_Status SHT3x_CreateSensor(SHT3x_Sensor *s);


/** 
 * @brief Set measurement mode for SHT3x.
 * @param s Pointer to sensor handle.
 * @param m SHT3x mode, `SHT3X_MODE_PERIODIC` by default.
 */
void SHT3x_SetMode(SHT3x_Sensor *s, SHT3x_Mode m);

/**
 * @brief Set SHT3x I2C address.
 * @param s Pointer to sensor handle.
 * @param a SHT3x I2C address.
 */
void SHT3x_ConfigSetI2cAddress(SHT3x_Sensor *s, SHT3x_I2cAddress a);

/**
 * @brief Set SHT3x repeatability.
 * @param s Pointer to sensor handle.
 * @param r Repeatability.
 */
void SHT3x_ConfigSetRepeatability(SHT3x_Sensor *s, SHT3x_Repeatability r);

/**
 * @brief Set SHT3x measurement per second in periodic mode.
 * @param s Pointer to sensor handle.
 * @param mps measurement per second.
 * @return `SHT3x_Status` code.
 * @retval `SHT3X_OK` on success.
 * @retval `SHT3X_ERROR_INVALID_MODE` if measurement mode was not `SHT3X_MODE_PERIODIC`.
 */
SHT3x_Status SHT3x_ConfigSetMps(SHT3x_Sensor *s, SHT3x_MPS mps);

/**
 * @brief Enable ART (Accelerated response time).
 * @param s Pointer to sensor handle.
 * @param on_off `true` to enable, `false` to disable.
 * @return `SHT3x_Status` code.
 * @retval `SHT3X_OK` on success.
 * @retval `SHT3X_ERROR_INVALID_MODE` if measurement mode was not `SHT3X_MODE_PERIODIC`.
 */
SHT3x_Status SHT3x_ConfigSetArtEnable(SHT3x_Sensor *s, bool on_off);

/**
 * @brief Enable clock stretch.
 * @param s Pointer to sensor handle.
 * @param on_off `true` to enable, `false` to disable.
 * @return `SHT3x_Status` code.
 * @retval `SHT3X_OK` on success.
 * @retval `SHT3X_ERROR_INVALID_MODE` if measurement mode was not `SHT3X_MODE_SINGLE_SHOT`.
 */
SHT3x_Status SHT3x_ConfigSetClockStretch(SHT3x_Sensor *s, bool on_off);

/**
 * @brief Set platform HAL I2C write.
 * @param s Pointer to sensor handle.
 * @param i2c_write
 */
void SHT3x_HalSetI2cWrite(SHT3x_Sensor *s, SHT3x_I2cWrite i2c_write);

/**
 * @brief Set platform HAL I2C read.
 * @param s Pointer to sensor handle.
 * @param i2c_read
 */
void SHT3x_HalSetI2cRead(SHT3x_Sensor *s, SHT3x_I2cRead i2c_read);

/**
 * @brief Set platform delay in ms.
 * @param s Pointer to sensor handle.
 * @param delay_ms
 */
void SHT3x_HalSetDelayMs(SHT3x_Sensor *s, SHT3x_DelayMs delay_ms);

#endif /* SHT3X_H_ */