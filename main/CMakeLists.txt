# SET(SOURCES screens/ui_Screen1.c
#     ui.c
#     ui_helpers.c)

# add_library(ui ${SOURCES})

file(GLOB_RECURSE SOURCES_C *.c)
file(GLOB_RECURSE SOURCES_CPP *.cpp)
# force re-glob when new source files are added

list(REMOVE_ITEM SOURCES_C "${CMAKE_CURRENT_SOURCE_DIR}/APP/lvgl_demo.c")

idf_component_register(
    SRCS "main.c" "board.c" ${SOURCES_C} ${SOURCES_CPP}
    INCLUDE_DIRS "." "APP" "APP/ui" "APP/ble" "APP/comm"
    PRIV_REQUIRES nvs_flash esp_lcd driver lvgl bt
        espressif__esp_lvgl_port
        espressif__esp_lcd_touch_cst816s
        espressif__esp_lcd_touch_ft5x06
        espressif__esp_lcd_touch_gt911
        espressif__esp_lcd_touch
        espressif__button
   )

    
# spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)


