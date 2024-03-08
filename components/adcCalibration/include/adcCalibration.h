#ifndef ADC_CALIBRATION_H
#define ADC_CALIBRATION_H

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle); // ADC calibration init
void example_adc_calibration_deinit(adc_cali_handle_t handle);    

#endif