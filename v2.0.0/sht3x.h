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
    SHT3x_Config *config;
    SHT3x_Hal    *hal;
} SHT3x_Sensor;

SHT3x_Status SHT3x_CreateSensor(SHT3x_Sensor *s);

/** @brief  */
void SHT3x_SetMode(SHT3x_Sensor *s, SHT3x_Mode m);

/** @brief  */
void SHT3x_ConfigSetI2cAddress(SHT3x_Sensor *s, SHT3x_I2cAddress a);
/** @brief  */
void SHT3x_ConfigSetRepeatability(SHT3x_Sensor *s, SHT3x_Repeatability r);
/** @brief  */
SHT3x_Status SHT3x_ConfigSetMps(SHT3x_Sensor *s, SHT3x_MPS mps);
/** @brief  */
SHT3x_Status SHT3x_ConfigSetClockStretch(SHT3x_Sensor *s, bool on_off);

/** @brief  */
void SHT3x_HalSetI2cWrite(SHT3x_Sensor *s, SHT3x_I2cWrite i2c_write);
/** @brief  */
void SHT3x_HalSetI2cRead(SHT3x_Sensor *s, SHT3x_I2cRead i2c_read);
/** @brief  */
void SHT3x_HalSetDelayMs(SHT3x_Sensor *s, SHT3x_DelayMs delay_ms);

#endif /* SHT3X_H_ */