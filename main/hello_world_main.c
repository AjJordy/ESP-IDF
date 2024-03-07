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

// My components
#include "calculadora.h"
#include "led.h"
#include "relay.h"

// ESP32 includes
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_PIN_1 16
#define LED_PIN_2 17
#define LED_PIN_3 18
#define BUTTON_PIN_1 19
#define BUTTON_PIN_2 20

#define RELAY_PIN_1 21
#define RELAY_PIN_2 22

Relay relay1, relay2; 

static const char *TAG = "Button test";
static QueueHandle_t gpio_evt_queue = NULL;


void ledTask(void *pvParameters) {
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

    while (true) {
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


static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
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


    // // Configure Inputs
    // gpio_reset_pin(BUTTON_PIN_1);
    // gpio_set_direction(BUTTON_PIN_1, GPIO_MODE_INPUT);
    // gpio_pullup_en(BUTTON_PIN_1);
    // // gpio_set_pull_mode(BUTTON_PIN_1, GPIO_PULLUP_ONLY);
    // gpio_set_intr_type(BUTTON_PIN_1, GPIO_INTR_NEGEDGE);
    
    io_config.pin_bit_mask = (1ULL<<BUTTON_PIN_1) | (1ULL<<BUTTON_PIN_2); 
    io_config.mode = GPIO_MODE_INPUT;
    io_config.pull_up_en = GPIO_PULLUP_ENABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_config);

    // Configuring interruptions
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(BUTTON_PIN_1, gpio_isr_handler, (void*) BUTTON_PIN_1);


    // Creating tasks
    xTaskCreate(buttonTask, "BUTTON TASK", 2048, NULL, 2, NULL);
    xTaskCreate(ledTask, "LED TASK", 1024, NULL, 2, NULL);    


    int resultado = soma(10, 20);
    printf("Resultado: %d\n", resultado);

    liga_led();

    relay_init(&relay1, RELAY_PIN_1);
    relay_init(&relay2, RELAY_PIN_2);

    // int button_state = 1;
    while(1) {
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
