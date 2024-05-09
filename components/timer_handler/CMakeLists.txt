idf_component_register(SRCS "timer_handler.c"
                       INCLUDE_DIRS "."
                       REQUIRES  
                       esp_event
                       driver
                       log
                       )
