#ifndef SHT3X_TYPES_H_
#define SHT3X_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** @brief 7-bit I2C device addresses. */
typedef enum {
    SHT3X_I2C_ADDR_VSS          = 0x44u,    /**< ADDR pin connected to VSS (default) */
    SHT3X_I2C_ADDR_VDD          = 0x45u     /**< ADDR pin connected to VDD */
} SHT3x_I2cAddress;

/** @brief Sensor operating mode. */
typedef enum {
    SHT3X_MODE_SINGLE_SHOT = 0,             /**< On-demand single measurement per @ref SHT3x_Read call  */
    SHT3X_MODE_PERIODIC    = 1,             /**< Sensor samples autonomously at the configured MPS rate  */
} SHT3x_Mode;

/**
 * @brief Measurement repeatability (precision) level.
 * @details Higher repeatability improves accuracy at the cost of longer
 *          conversion time and higher power consumption.
 */
typedef enum {
    SHT3X_REPEAT_LOW    = 0,  /**< Low repeatability - fastest, lowest accuracy  */
    SHT3X_REPEAT_MEDIUM = 1,  /**< Medium repeatability                          */
    SHT3X_REPEAT_HIGH   = 2,  /**< High repeatability - slowest, highest accuracy */
} SHT3x_Repeatability;

/**
 * @brief Periodic-mode measurement rate (measurements per second).
 * @details Only relevant when @ref SHT3x_Dev::mode is @ref SHT3X_MODE_PERIODIC.
 */
typedef enum {
    SHT3X_MPS_05 = 0,   /**< 0.5 measurements per second */
    SHT3X_MPS_1   = 1,  /**< 1 measurement per second    */
    SHT3X_MPS_2   = 2,  /**< 2 measurements per second   */
    SHT3X_MPS_4   = 3,  /**< 4 measurements per second   */
    SHT3X_MPS_10  = 4,  /**< 10 measurements per second  */
} SHT3x_MPS;

/** @brief Generic GPIO handle. */
typedef struct {
    void *ctx;      /**<! Platform specific (e.g. GPIO port struct). */
    int32_t pin;    /**<! Pin number/mask of platform. */
} SHT3x_Gpio;

/** @brief  */
typedef int  (*SHT3x_I2cWrite) (uint8_t address, const uint8_t *data, size_t len);
typedef int  (*SHT3x_I2cRead)  (uint8_t address, uint8_t *data, size_t len);
typedef void (*SHT3x_DelayMs)  (uint32_t *ms);

typedef struct {
    SHT3x_Mode          mode;           /**< Measurement mode. */
    SHT3x_I2cAddress    i2c_address;    /**< 7-bit I2C device address. */
    SHT3x_Repeatability repeatability;  /**< Measurement repeatability level. */
    union {
        struct {
            SHT3x_MPS   meas_per_sec;   /**< Periodic measurement rate. Periodic mode only. */
            bool        art_enabled;
        } periodic;
        struct {
            bool        clock_stretch;         /**< Clock stretching. Single-shot mode only. */
        } singleshot;
    } mode_cfg;
} SHT3x_Config;

typedef struct {
    SHT3x_I2cRead       i2c_read;       /**< Platform I2C read. Must not be NULL. */
    SHT3x_I2cWrite      i2c_write;      /**< Platform I2C write. Must not be NULL. */
    SHT3x_DelayMs       delay_ms;       /**< Platform delay in ms. Must not be NULL. */
} SHT3x_Hal;

/** @brief Measurement result container populated by @ref SHT3x_Read. */
typedef struct {
    float temperature_c;  /**< Temperature in degrees Celsius           */
    float humidity_rh;    /**< Relative humidity in percent (%RH)       */
} SHT3x_Data;


#endif /* SHT3X_TYPES_H_ */