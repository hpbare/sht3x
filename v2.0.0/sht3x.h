#ifndef SHT3X_H_
#define SHT3X_H_

#include "sht3x_types.h"

typedef enum {
    SHT3X_OK                 =  0,  /**< Operation completed successfully         */
    SHT3X_ERROR_I2C          = -1,  /**< I2C bus communication error              */
    SHT3X_ERROR_CRC          = -2,  /**< CRC-8 validation failed on received data */
    SHT3X_ERROR_INVALID_ARGS = -3,  /**< Invalid or out-of-range parameter        */
    SHT3X_ERROR_INVALID_MODE = -4   /**< Operation not valid for the configured mode */
} SHT3x_Status;

typedef struct {
    SHT3x_Config config;
    SHT3x_Hal    hal;
} SHT3x_Sensor;

/**
 * @brief   Initialise a sensor handle with default configuration.
 * @details Sets mode to @ref SHT3X_MODE_PERIODIC, I2C address to
 *          @ref SHT3X_I2C_ADDR_VSS, repeatability to @ref SHT3X_REPEAT_HIGH,
 *          rate to @ref SHT3X_MPS_1, ART disabled. HAL function pointers are
 *          cleared to NULL and must be set via @ref SHT3x_HalSetI2cWrite,
 *          @ref SHT3x_HalSetI2cRead and @ref SHT3x_HalSetDelayMs before use.
 *
 * @param[in,out] s  Pointer to sensor handle; must have @c config and @c hal
 *                    already pointing to valid storage.
 * @return            @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s, @p s->config or @p s->hal is NULL.
 */
SHT3x_Status SHT3x_CreateDefaultSensor(SHT3x_Sensor *s);

/**
 * @brief Set measurement mode for SHT3x.
 * @param s Pointer to sensor handle.
 * @param m SHT3x mode, @ref SHT3X_MODE_PERIODIC by default.
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
 * @param s   Pointer to sensor handle.
 * @param mps Measurement rate.
 * @return    @ref SHT3x_Status code.
 * @retval  SHT3X_ERROR_INVALID_MODE  if measurement mode is not @ref SHT3X_MODE_PERIODIC.
 */
SHT3x_Status SHT3x_ConfigSetMps(SHT3x_Sensor *s, SHT3x_MPS mps);

/**
 * @brief Enable ART (Accelerated Response Time).
 * @param s      Pointer to sensor handle.
 * @param on_off @c true to enable, @c false to disable.
 * @return       @ref SHT3x_Status code.
 * @retval  SHT3X_ERROR_INVALID_MODE  if measurement mode is not @ref SHT3X_MODE_PERIODIC.
 */
SHT3x_Status SHT3x_ConfigSetArtEnable(SHT3x_Sensor *s, bool on_off);

/**
 * @brief Enable clock stretching for single-shot reads.
 * @param s      Pointer to sensor handle.
 * @param on_off @c true to enable, @c false to disable.
 * @return       @ref SHT3x_Status code.
 * @retval  SHT3X_ERROR_INVALID_MODE  if measurement mode is not @ref SHT3X_MODE_SINGLE_SHOT.
 */
SHT3x_Status SHT3x_ConfigSetClockStretch(SHT3x_Sensor *s, bool on_off);

/**
 * @brief Set the platform I2C write callback.
 * @param s         Pointer to sensor handle.
 * @param i2c_write Platform I2C write function. Must not be NULL before use.
 */
void SHT3x_HalSetI2cWrite(SHT3x_Sensor *s, SHT3x_I2cWrite i2c_write);

/**
 * @brief Set the platform I2C read callback.
 * @param s        Pointer to sensor handle.
 * @param i2c_read Platform I2C read function. Must not be NULL before use.
 */
void SHT3x_HalSetI2cRead(SHT3x_Sensor *s, SHT3x_I2cRead i2c_read);

/**
 * @brief Set the platform millisecond delay callback.
 * @param s        Pointer to sensor handle.
 * @param delay_ms Platform blocking delay function. Must not be NULL before use.
 */
void SHT3x_HalSetDelayMs(SHT3x_Sensor *s, SHT3x_DelayMs delay_ms);

/**
 * @brief   Reset, then start periodic measurement at the configured rate.
 * @param   s  Pointer to a fully configured sensor handle.
 * @return     @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s is NULL or not fully configured.
 * @retval  SHT3X_ERROR_INVALID_MODE  if measurement mode is not @ref SHT3X_MODE_PERIODIC.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 */
SHT3x_Status SHT3x_StartPeriodicMeasurement(SHT3x_Sensor *s);

/**
 * @brief   Stop periodic measurement and return the sensor to idle.
 * @param   s  Pointer to sensor handle.
 * @return     @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s is NULL or not fully configured.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 */
SHT3x_Status SHT3x_StopPeriodicMeasurement(SHT3x_Sensor *s);

/**
 * @brief   Read a temperature/humidity measurement.
 * @details In @ref SHT3X_MODE_PERIODIC, fetches the latest pending result.
 *          In @ref SHT3X_MODE_SINGLE_SHOT, triggers a new measurement and
 *          waits for it to complete before reading.
 *
 * @param[in]  s  Pointer to sensor handle.
 * @param[out] d  Pointer to receive the decoded temperature/humidity.
 * @return        @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s or @p d is NULL, or @p s is not fully configured.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 * @retval  SHT3X_ERROR_CRC           if the response fails CRC verification.
 */
SHT3x_Status SHT3x_ReadMeasurement(SHT3x_Sensor *s, SHT3x_MeasurementData *d);

/**
 * @brief   Enable or disable the on-chip heater.
 * @param   s      Pointer to sensor handle.
 * @param   on_off @c true to enable, @c false to disable.
 * @return         @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s is NULL or not fully configured.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 */
SHT3x_Status SHT3x_HeaterEnable(SHT3x_Sensor *s, bool on_off);

/**
 * @brief   Perform a soft reset via the I2C interface.
 * @param   s  Pointer to sensor handle.
 * @return     @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s is NULL or not fully configured.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 */
SHT3x_Status SHT3x_SoftReset(SHT3x_Sensor *s);

/**
 * @brief   Read the 16-bit device status register.
 * @details Sends @ref SHT3X_CMD_READ_STATUS, then reads one word with CRC
 *          verification. The raw register value is returned in @p status;
 *          use the @c SHT3X_SREG_* bitmasks from @ref sht3x_defs.h to decode
 *          individual flag bits.
 *
 * @param[in]  s               Pointer to an initialised @ref SHT3x_Sensor handle.
 * @param[out] status_register Pointer to receive the 16-bit status register value.
 * @return                     @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s or @p status_register is NULL.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 * @retval  SHT3X_ERROR_CRC           if the response fails CRC verification.
 */
SHT3x_Status SHT3x_ReadStatusRegister(SHT3x_Sensor *s, SHT3x_StatusRegister *status_register);

/**
 * @brief   Clear all flags in the status register.
 * @param   s  Pointer to sensor handle.
 * @return     @ref SHT3X_OK on success.
 * @retval  SHT3X_ERROR_INVALID_ARGS  if @p s is NULL or not fully configured.
 * @retval  SHT3X_ERROR_I2C           if any I2C transaction fails.
 */
SHT3x_Status SHT3x_ClearStatusRegister(SHT3x_Sensor *s);

#endif /* SHT3X_H_ */