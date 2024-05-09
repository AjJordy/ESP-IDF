idf_component_register(SRCS "http_client.c"
                       INCLUDE_DIRS "."
                       REQUIRES
                       freertos
                       esp_netif
                       esp_wifi
                       esp_event
                       esp_http_client
                       log
                       )
