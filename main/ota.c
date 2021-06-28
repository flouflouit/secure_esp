#include "ota.h"
#include "nvs.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include <string.h>

char *TAG;
SemaphoreHandle_t Semaphore_Update_Started;
//OTA Cert
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_crt_start");


#define OTA_URL_SIZE 256

esp_err_t read_int8_from_nvs(int8_t*,char **);
esp_err_t read_string_from_nvs(char **, char **);

char* get_firmware_update_url(){

    char *key = "use_url";
    char *firmware_update_url;

    int8_t value;
    esp_err_t err = read_int8_from_nvs(&value,&key);

    if (err != ESP_OK){
        return CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL;
    };
    //Check if use_url is set to 0; 0 = take default url; 1 = take nvs url
    if(value==0){
        return CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL;
    //Read url from nvs
    }else if(value==1){
        key = "update_url";
        err = read_string_from_nvs(&firmware_update_url,&key);
        if (err != ESP_OK){
            return CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL;
        };
        return firmware_update_url;
    }

    return CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL;
}

esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(strcmp(new_app_info->project_name, TAG)!=0){
        ESP_LOGW(TAG, "ESP project_name does not fit with TAG. Update will not continue.");
        return ESP_FAIL;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "Current version is the same as a new version. Update will not continue.");
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}


void ota_update(void *parameter)
{
    
    char * firmware_url = (char *) parameter;
    ESP_LOGI(TAG, "Start update with firmware upgrade url: %s",firmware_url);
   
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = firmware_url,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
    };
    //should be disabled - security feature
    #ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
        config.skip_cert_common_name_check = true;
    #endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed"); 
    if( Semaphore_Update_Started != NULL )
    {
        xSemaphoreGive( Semaphore_Update_Started );
    }
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "image header verification failed");
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
       
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        ESP_LOGE(TAG, "Complete data was not received.");
    }
     if( Semaphore_Update_Started != NULL )
    {
    xSemaphoreGive( Semaphore_Update_Started );
    }
ota_end:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
     if( Semaphore_Update_Started != NULL )
    {
    xSemaphoreGive( Semaphore_Update_Started );
    }
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
        vTaskDelete(NULL);
    }
}