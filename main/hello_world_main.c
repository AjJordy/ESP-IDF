/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ESP32 includes
#include "esp_log.h"
#include "driver/gpio.h"

// PWM
#include "driver/ledc.h"

// ADC
#include "esp_adc/adc_oneshot.h"


// My components
#include "calculadora.h"
#include "led.h"
#include "relay.h"


#define LED_PIN_1 16
#define LED_PIN_2 17
#define LED_PIN_3 18
#define BUTTON_PIN_1 19
#define BUTTON_PIN_2 20

#define RELAY_PIN_1 21
#define RELAY_PIN_2 22

Relay relay1, relay2; 

static const char *TAG = "Example";
static QueueHandle_t gpio_evt_queue = NULL;



static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


void ledTask(void *pvParameters) {
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

    while (true) {        
        gpio_set_level(LED_PIN_1, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        gpio_set_level(LED_PIN_1, 0);   
        vTaskDelay(pdMS_TO_TICKS(1000));     
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void buttonTask(void *pvParameters) {
    uint32_t gpio_num;
    TickType_t last_button_press = 0;
    bool led_state = 0;

    // // Configure Inputs
    // gpio_reset_pin(BUTTON_PIN_1);
    // gpio_set_direction(BUTTON_PIN_1, GPIO_MODE_INPUT);
    // gpio_pullup_en(BUTTON_PIN_1);
    // // gpio_set_pull_mode(BUTTON_PIN_1, GPIO_PULLUP_ONLY);
    // gpio_set_intr_type(BUTTON_PIN_1, GPIO_INTR_NEGEDGE);
    
    gpio_config_t io_config;
    io_config.pin_bit_mask = (1ULL<<BUTTON_PIN_1) | (1ULL<<BUTTON_PIN_2); 
    io_config.mode = GPIO_MODE_INPUT;
    io_config.pull_up_en = GPIO_PULLUP_ENABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_config);

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
    
    // ---------- Config relays ----------
    relay_init(&relay1, RELAY_PIN_1);
    relay_init(&relay2, RELAY_PIN_2);

    while (true) {
        relay_turn_on(&relay1);
        printf("Status do rele 1: %d\n", relay_get_status(&relay1));
        relay_turn_off(&relay2);
        printf("Status do rele 2: %d\n", relay_get_status(&relay2));
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        relay_turn_off(&relay1);
        printf("Status do rele 1: %d\n", relay_get_status(&relay1));
        relay_turn_on(&relay2);
        printf("Status do rele 2: %d\n", relay_get_status(&relay2));
        vTaskDelay(5000 / portTICK_PERIOD_MS);    
    }
}


void pwmTask(){
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


void fadeTask(){
    // Timer
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


void adcTask(){
    int adc_raw; 
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));

    while(1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
        ESP_LOGI(TAG, "ADC%d channel[%d] Raw data: %d", ADC_UNIT_1+1, ADC_CHANNEL_0, adc_raw);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



void app_main(void) {
    printf("Program init!\n");
    // esp_log_level_set(TAG, ESP_LOG_NONE);
    // esp_log_level_set(TAG, ESP_LOG_ERROR);
    // esp_log_level_set(TAG, ESP_LOG_WARN);
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    // esp_log_level_set(TAG, ESP_LOG_DEBUG);
    // esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    ESP_LOGE(TAG, "Error");
    ESP_LOGW(TAG, "Warning");
    ESP_LOGI(TAG, "Info");
    ESP_LOGD(TAG, "Debug");
    ESP_LOGV(TAG, "Verbose");


    // ---------- Creating tasks ----------
    xTaskCreate(buttonTask, "BUTTON TASK", 2048, NULL, 2, NULL);
    xTaskCreate(relayTask, "RELAY TASK", 2048, NULL, 2, NULL);
    xTaskCreate(pwmTask, "PWM TASK", 2048, NULL, 2, NULL);
    xTaskCreate(fadeTask, "FADE TASK", 2048, NULL, 2, NULL);
    xTaskCreate(ledTask, "LED TASK", 2048, NULL, 2, NULL);
    xTaskCreate(adcTask, "ADC TASK", 2048, NULL, 2, NULL);

    // int resultado = soma(10, 20);
    // printf("Resultado: %d\n", resultado);
    // liga_led();
    
    while(1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }    
}
