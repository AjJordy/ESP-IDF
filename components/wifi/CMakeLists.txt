idf_component_register(SRCS "wifi.c"
                       INCLUDE_DIRS "."
                       REQUIRES
                       esp_netif
                       esp_wifi
                       freertos
                       log
                       )
