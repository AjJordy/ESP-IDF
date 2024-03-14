/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

// ---- FreeRTOS includes ----
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

// ---- ESP32 includes ----
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h" // PWM
#include "esp_adc/adc_oneshot.h"// ADC
#include "driver/dac_oneshot.h" // DAC
#include "driver/dac_cosine.h"  // DAC
#include "driver/temperature_sensor.h" // Temperature


// My components
#include "calculadora.h"
#include "led.h"
#include "relay.h"
#include "adcCalibration.h"


#define LED_PIN_1 16
#define LED_PIN_2 17
#define LED_PIN_3 18
#define BUTTON_PIN_1 9
// #define BUTTON_PIN_2 20

#define RELAY_PIN_1 21
#define RELAY_PIN_2 22

#define LED_DELAY 1000

Relay relay1, relay2; 

static const char *TAG = "Example";

QueueHandle_t gpio_evt_queue = NULL;

SemaphoreHandle_t xBinarySemaphore = NULL;
SemaphoreHandle_t xCountingSemaphore = NULL;
SemaphoreHandle_t xMutexSemaphore = NULL;

TaskHandle_t xTaskLedHandle = NULL;
TaskHandle_t xTaskButtonHandle = NULL;
TaskHandle_t xTaskPWMHandle = NULL;

TimerHandle_t xTimer1, xTimer2;


void sendSerialData(char *data) {
    printf(data);
    vTaskDelay(pdMS_TO_TICKS(100));
}


static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, xHigherPriorityTaskWoken);
    xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
}


void ledTask(void *pvParameters) {
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    // Configure Outputs
    // gpio_reset_pin(LED_PIN_1);
    // gpio_reset_pin(LED_PIN_2);
    // gpio_reset_pin(LED_PIN_3);
    // gpio_set_direction(LED_PIN_1, GPIO_MODE_OUTPUT);
    // gpio_set_direction(LED_PIN_2, GPIO_MODE_OUTPUT);
    // gpio_set_level(LED_PIN_1, 0);
    // gpio_set_level(LED_PIN_2, 0);
    // gpio_set_level(LED_PIN_3, 0);

    gpio_config_t io_config = {};
    io_config.pin_bit_mask = (1ULL<<LED_PIN_1) | (1ULL<<LED_PIN_2) | (1ULL<<LED_PIN_3); 
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_config);  

    
    bool led_state = 0;
    // int led_delay = (int) pvParameters;
    // int count = 0;
    // BaseType_t count_semph = 0;

    while (true) {  

        xSemaphoreTake(xMutexSemaphore, portMAX_DELAY);
        sendSerialData(">>>> TASK LED\n");
        xSemaphoreGive(xMutexSemaphore);      

        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
            gpio_set_level(LED_PIN_2, led_state^=1);
            ESP_LOGI(TAG, "LED: %d", led_state);
            // count_semph = uxSemaphoreGetCount(xCountingSemaphore);
            // ESP_LOGI(TAG, "Count: %d", count_semph);           
        }        

        // gpio_set_level(LED_PIN_2, 1);
        // vTaskDelay(pdMS_TO_TICKS(led_delay));
        // gpio_set_level(LED_PIN_2, 0);   
        // vTaskDelay(pdMS_TO_TICKS(led_delay));
        // count++;
        // if (count == 1024) {
        //     gpio_set_level(LED_PIN_2, 0);
        //     vTaskDelete(NULL); //(xTaskLedHandle);
        //     // vTaskSuspend(NULL);
        //     // vTaskResume(NULL);
        // }
    }
}


void buttonTask(void *pvParameters) {
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    uint32_t gpio_num;
    TickType_t last_button_press = 0;
    bool led_state = 0;

    // Configure Inputs
    gpio_reset_pin(BUTTON_PIN_1);
    gpio_set_direction(BUTTON_PIN_1, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_1);
    // gpio_set_pull_mode(BUTTON_PIN_1, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_PIN_1, GPIO_INTR_NEGEDGE);
    
    // gpio_config_t io_config;
    // io_config.pin_bit_mask = (1ULL<<BUTTON_PIN_1) | (1ULL<<BUTTON_PIN_2); 
    // io_config.mode = GPIO_MODE_INPUT;
    // io_config.pull_up_en = GPIO_PULLUP_ENABLE;
    // io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // io_config.intr_type = GPIO_INTR_NEGEDGE;
    // gpio_config(&io_config);

    // ---------- Configuring interruptions ----------
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(BUTTON_PIN_1, gpio_isr_handler, (void*) BUTTON_PIN_1);

    // int button_state = 1;
    while (true) {

        // ---------- Reading button ----------
        // int new_state = gpio_get_level(BUTTON_PIN);
        // if (new_state != button_state) {
        //     button_state = new_state;
        //     if (!button_state) {
        //         gpio_set_level(LED_PIN_2, led_state^=1);
        //         ESP_LOGI(TAG, "BUTTON PRESSED");
        //     }
        //     else {
        //         ESP_LOGI(TAG, "BUTTON RELEASED");
        //     }
        // }

        xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY);
        ESP_LOGI(TAG, "GPIO[%li] intr\n", gpio_num);
        TickType_t current_time = xTaskGetTickCount();
        if (current_time - last_button_press >= pdMS_TO_TICKS(250)) {
            last_button_press = current_time;
            if (gpio_num == BUTTON_PIN_1) {
                gpio_set_level(LED_PIN_1, led_state^=1);
            }
        }
    }
}


void relayTask(void *pvParameters){
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    
    // ---------- Config relays ----------
    relay_init(&relay1, RELAY_PIN_1);
    relay_init(&relay2, RELAY_PIN_2);

    while (true) {
        relay_turn_on(&relay1);
        printf("[%s] Status do rele 1: %d\n", pcTaskGetName(NULL), relay_get_status(&relay1));
        relay_turn_off(&relay2);
        printf("[%s] Status do rele 2: %d\n", pcTaskGetName(NULL), relay_get_status(&relay2));
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        relay_turn_off(&relay1);
        printf("[%s] Status do rele 1: %d\n", pcTaskGetName(NULL), relay_get_status(&relay1));
        relay_turn_on(&relay2);
        printf("[%s] Status do rele 2: %d\n", pcTaskGetName(NULL), relay_get_status(&relay2));
        vTaskDelay(5000 / portTICK_PERIOD_MS);    
    }
}


void pwmTask(void *pvParameters){
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    // ---------- PWM ----------
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_10_BIT, // 0 - 1023
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    // PWM 1
    ledc_channel_config_t channel_config = {
        .channel = LEDC_CHANNEL_0,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_NUM_14,
        .duty = 0
    };
    ledc_channel_config(&channel_config);

    // PWM 2
    ledc_channel_config_t channel_config2 = {
        .channel = LEDC_CHANNEL_1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_NUM_21,
        .duty = 511 // 50%
    };
    ledc_channel_config(&channel_config2);


    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 511); // 50% 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);


    int duty = 0;
    while(1) {
        if (duty < 1024){
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            duty++;
        }
        else {
            duty = 0;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void fadeTask(void *pvParameters){
    // Timer
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_12_BIT, // 0 - 4095
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    // PWM 
    ledc_channel_config_t channel_config = {
        .channel = LEDC_CHANNEL_0,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_NUM_14,
        .duty = 0
    };
    ledc_channel_config(&channel_config);
    ledc_fade_func_install(0);

    while(1) {
       ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095, 2000, LEDC_FADE_WAIT_DONE);
       vTaskDelay(200/portTICK_PERIOD_MS);
       ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 2000, LEDC_FADE_WAIT_DONE);
       vTaskDelay(200/portTICK_PERIOD_MS);
    }  
}


// void adcTask(){
//     int adc_raw; 
//     int voltage;
//     adc_oneshot_unit_handle_t adc1_handle;
//     adc_oneshot_unit_init_cfg_t init_config = {
//         .unit_id = ADC_UNIT_1,
//     };
//     ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
//     adc_oneshot_chan_cfg_t config = {
//         .bitwidth = ADC_BITWIDTH_DEFAULT,
//         .atten = ADC_ATTEN_DB_12,
//     };
//     ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
//     while(1) {
//         ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
//         ESP_LOGI(TAG, "ADC%d channel[%d] Raw data: %d", ADC_UNIT_1+1, ADC_CHANNEL_0, adc_raw);
//         voltage = (adc_raw * 2500) / 8192;
//         ESP_LOGI(TAG, "ADC%d channel[%d] Voltage: %dmV", ADC_UNIT_1+1, ADC_CHANNEL_0, voltage);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }


void adcCallibrateTask(void *pvParameters){
    int adc_raw; 
    int voltage;
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));

    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);

    while(1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
        // ESP_LOGI(TAG, "ADC%d channel[%d] Raw data: %d", ADC_UNIT_1+1, ADC_CHANNEL_0, adc_raw);
        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &voltage));
            ESP_LOGI(TAG, "ADC%d channel[%d] Voltage: %dmV", ADC_UNIT_1+1, ADC_CHANNEL_0, voltage);
        }        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0) {
        example_adc_calibration_deinit(adc1_cali_chan0_handle);
    }

}


#if SOC_DAC_SUPPORTED
void dacTask(void *pvParameters){
    uint8_t val = 0;
    dac_oneshot_handle_t chan0_handle;
    dac_oneshot_config_t chan0_cfg = {
        .chan_id = DAC_CHAN_0,
    };
    ESP_ERROR_CHECK(dac_oneshot_new_channel(&chan0_cfg, &chan0_handle));
    ESP_LOGI(TAG, "DAC oneshot example start.");

    while(1){
        // Ramp
        for (val=0; val<255; val++){
            ESP_ERROR_CHECK(dac_oneshot_output_voltage(chan0_handle, val)); // 0 - 255 ~ 0 - 3.3v
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}


void dacCosineTask(void *pvParameters){
    dac_cosine_handle_t chan0_handle;
    dac_cosine_config_t cos0_cfg = {
        .chan_id = DAC_CHAN_0,
        .freq_hz = 1000, // 1kHz
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT, // default clock source
        .offset = 0, // 0V
        .phase = DAC_COSINE_PHASE_0, // 0 degrees
        .atten = DAC_COSINE_ATTEN_DEFAULT, // default attenuation 
        .flags.force_set_freq = false, // do not force frequency
    };

    ESP_LOGI(TAG, "Initializing DAC cosine wave generator...");
    ESP_ERROR_CHECK(dac_cosine_new_channel(&cos0_cfg, &chan0_handle));
    ESP_ERROR_CHECK(dac_cosine_start(chan0_handle));
    ESP_LOGI(TAG, "DAC cosine initialized");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
}
#endif


void temperatureTask(void *pvParameters){
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    ESP_LOGI(TAG, "Install temperature sensor, expected temp ranger range: 10-50 °C");
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
    ESP_LOGI(TAG, "Enable temperature sensor");
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
    ESP_LOGI(TAG, "Read temperature sensor");
    ESP_LOGI(TAG, "Read temperature");    
    float tsense_value;
    while (1){
        ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor, &tsense_value));
        ESP_LOGI(TAG, "Temperature value %.02f °C", tsense_value);        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


void timer1_callback(TimerHandle_t xTimer){
    static uint8_t led_state = 0;
    gpio_set_level(LED_PIN_3, led_state^=1);
}


void timer2_callback(TimerHandle_t xTimer){
    gpio_set_level(LED_PIN_3, 0);
     ESP_LOGI("xTimer2", "Callback");
}


void timerTask(void *pvParameters){
    #ifdef configUSE_TIMERS    
    xTimer1 = xTimerCreate("Timer1", pdMS_TO_TICKS(5000),  pdTRUE, (void*)0, timer1_callback); // Auto reload
    xTimer2 = xTimerCreate("Timer2", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, timer2_callback); // One shot

    xTimerStart(xTimer1, 0); // Init immediately
    #endif

    TickType_t last_button_press = 0;
    

    while(true){
        if (gpio_get_level(BUTTON_PIN_1) == 0) {
            TickType_t current_time = xTaskGetTickCount();
            if (current_time - last_button_press >= pdMS_TO_TICKS(250)) {
                last_button_press = current_time;           
                gpio_set_level(LED_PIN_3, 1);
                xTimerStart(xTimer2, 0);
                ESP_LOGI("xTimer2", "Start");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}


void app_main(void) {
    printf("Program init!\n");
    // esp_log_level_set(TAG, ESP_LOG_NONE);
    // esp_log_level_set(TAG, ESP_LOG_ERROR);
    // esp_log_level_set(TAG, ESP_LOG_WARN);
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    // esp_log_level_set(TAG, ESP_LOG_DEBUG);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    ESP_LOGE(TAG, "Error");
    ESP_LOGW(TAG, "Warning");
    ESP_LOGI(TAG, "Info");
    ESP_LOGD(TAG, "Debug");
    ESP_LOGV(TAG, "Verbose");

    xBinarySemaphore = xSemaphoreCreateBinary();
    // xCountingSemaphore = xSemaphoreCreateCounting(255, 0);
    xMutexSemaphore = xSemaphoreCreateMutex();


    // ---------- Creating tasks ----------
    xTaskCreate(
        ledTask,            // Task function
        "LED TASK",         // Task name
        2048,               // Stack size
        (void*) LED_DELAY,  // Parameters 
        2,                  // less 1 - 5 more 
        &xTaskLedHandle     // task reference
    );

    // xTaskCreatePinnedToCore(buttonTask, "BUTTON TASK", 2048, NULL, 2, &xTaskButtonHandle, 0);
    xTaskCreate(buttonTask, "BUTTON TASK", 2048, NULL, 3, &xTaskButtonHandle);
    xTaskCreate(relayTask, "RELAY TASK", 2048, NULL, 2, NULL);
    xTaskCreate(pwmTask, "PWM TASK", 2048, NULL, 2, &xTaskPWMHandle);
    xTaskCreate(fadeTask, "FADE TASK", 2048, NULL, 2, NULL);
    // xTaskCreate(adcTask, "ADC TASK", 2048, NULL, 2, NULL);
    xTaskCreate(adcCallibrateTask, "ADC CALIBRATE TASK", 2048, NULL, 2, NULL);    
    xTaskCreate(temperatureTask, "TEMPERATURE TASK", 2048, NULL, 2, NULL);
    xTaskCreate(timerTask, "TEMPERATURE TASK", 2048, NULL, 2, NULL);

    #if SOC_DAC_SUPPORTED
    xTaskCreate(dacTask, "DAC TASK", 2048, NULL, 2, NULL);
    xTaskCreate(dacCosineTask, "DAC COSINE TASK", 2048, NULL, 2, NULL);    
    #endif

    // Components 
    // int resultado = soma(10, 20);
    // printf("Resultado: %d\n", resultado);
    // liga_led();
    
    ESP_LOGI(TAG, "Iniciando a [%s]. CORE[%d]", pcTaskGetName(NULL), xPortGetCoreID());
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        xSemaphoreTake(xMutexSemaphore, portMAX_DELAY);
        sendSerialData(">>>> TASK MAIN\n");
        xSemaphoreGive(xMutexSemaphore);

        ESP_LOGI(TAG, "Led High water mark: %d", uxTaskGetStackHighWaterMark(xTaskLedHandle));
        ESP_LOGI(TAG, "Button High water mark: %d", uxTaskGetStackHighWaterMark(xTaskButtonHandle));
        ESP_LOGI(TAG, "PWM High water mark: %d", uxTaskGetStackHighWaterMark(xTaskPWMHandle));
    }    
}
