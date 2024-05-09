#include "timer_handler.h"

// #include "freertos/timers.h"
// #include "esp_event.h"
// #include "driver/gpio.h"
// #include "freertos/event_groups.h"

// #define TASK_1_BIT (1 << 0) // 1
// #define TASK_2_BIT (1 << 1) // 10

// #define LED_PIN_3 18

// EventGroupHandle_t xEvents;

// void timer_init_event() {
//     xEvents = xEventGroupCreate();
// }

// void timer1_callback(TimerHandle_t xTimer){
//     static uint8_t count = 0;
//     count++;
//     static uint8_t led_state = 0;
//     gpio_set_level(LED_PIN_3, led_state^=1);
//     ESP_LOGI("xTimer1", "Callback timer 1");

//     if (count == 5) {
//         xEventGroupSetBits(xEvents, TASK_1_BIT);
//     }
//     else if (count == 10) {
//         xEventGroupSetBits(xEvents, TASK_2_BIT);
//     }
//     else if (count == 15) {
//         count = 0;
//         xEventGroupSetBits(xEvents, TASK_1_BIT | TASK_2_BIT);        
//     }    
// }


// void timer2_callback(TimerHandle_t xTimer) {
//     gpio_set_level(LED_PIN_3, 0);
//     ESP_LOGI("xTimer2", "Callback timer 2");
// }
