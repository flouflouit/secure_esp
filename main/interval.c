#include <stdio.h>

#include "nvs_rw.h"
#include "interval.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

char* TAG;
SemaphoreHandle_t Semaphore_Update_Started;
char *get_firmware_update_url();
void ota_update();

// 1. Read NVS value with key=interval
// 2. Checks if NVS calue > default minutes
// 3. Starts Update Routine with delay of value or default_seconds 
void interval(){

    int default_seconds = 300; // = 5 Minutes
    while (1)
    {
        int32_t value;
        char *key = "interval";
        esp_err_t err = read_int32_from_nvs(&value,&key);
        if (err == ESP_OK) {
            if(value>default_seconds){ // Minimum value about 5 minutes
                ESP_LOGI(TAG, "Update Interval is set to %d minutes", value / 60);
                vTaskDelay(1000*value / portTICK_PERIOD_MS);
            }else{
                ESP_LOGI(TAG, "Update Interval is set to %d minutes", default_seconds / 60);
                vTaskDelay(1000*default_seconds/ portTICK_PERIOD_MS);
            }
       
        }else{
            ESP_LOGI(TAG, "Update Interval is set to %d minutes", default_seconds);
            vTaskDelay(1000*default_seconds / portTICK_PERIOD_MS);
        }

    char *firmware_update_url = get_firmware_update_url();


    if( Semaphore_Update_Started != NULL )
    {
        if( xSemaphoreTake( Semaphore_Update_Started, ( TickType_t ) 10 ) == pdTRUE )
        {
            xTaskCreate(&ota_update, "ota_update", 1024 * 8, (void *) firmware_update_url, 1, NULL);
        }
        else
        {
            ESP_LOGE(TAG, "Update Process is running (maybe triggerd by MQTT)- Cancel Interval");
        }
    }
    }
}