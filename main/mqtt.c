#include "mqtt.h"
#include "nvs_rw.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "ota.h"

#include "esp_ota_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


//MQTT Certs
extern const uint8_t ca_cert_pem_start[] asm("_binary_ca_crt_start");
extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");

char *TAG; 

//MQTT Topics
#define UPDATE_URL "/esp/update/url"
#define UPDATE_RUN "/esp/update/run"
#define UPDATE_USE_URL "/esp/update/use_url"
#define UPDATE_INTERVAL "/esp/update/interval"
#define RESTART "/esp/restart"
#define ESP_LOG "/esp/log"


//NVS Methods
esp_err_t write_string_to_nvs(char **, char **);
esp_err_t read_string_from_nvs(char **, char **);
esp_err_t write_int8_to_nvs(int8_t,char **);
esp_err_t read_int8_from_nvs(int8_t*,char **);
esp_err_t write_int32_to_nvs(int32_t , char **);

void ota_update();
char *get_firmware_update_url();

SemaphoreHandle_t Semaphore_Update_Started;

esp_err_t mqtt_log(esp_mqtt_event_handle_t,char *,char *);


void update_url(esp_mqtt_event_handle_t event){
    
    char *key ="update_url";
    char *update_url = malloc(event->data_len);

    //save url into nvs
    strncpy(update_url,event->data,event->data_len);
    update_url[event->data_len] = '\0';
    esp_err_t err = write_string_to_nvs(&update_url,&key);
    if (err != ESP_OK) printf("Error (%s) writing data to NVS!\n", esp_err_to_name(err));
    free(update_url);
    mqtt_log(event,"[SUCCESS] Custom URL is written to storage","");
    //read use_url and check use_url=1 else prompt for user
    int8_t value;
    key="use_url";
    err = read_int8_from_nvs(&value,&key);
    if (err != ESP_OK) printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));

    if(value==0){
        mqtt_log(event,"[WARNING] Custom url is not activated, because use_url is set to 0. Set use_url to 1 to activate the custom url.","");
    }

}

void update_use_url(esp_mqtt_event_handle_t event){

    char *key = "use_url";

    printf("Output of str_value: %d\n",event->data_len);
    if(event->data_len == 0){
        mqtt_log(event,"[ERROR] No message send","");
        return;
    }
    char *str_value = malloc(event->data_len);
    //save url into nvs
    strncpy(str_value,event->data,event->data_len);
    str_value[event->data_len] = '\0';
    
    if(strcmp(str_value,"0")==0){
        esp_err_t err = write_int8_to_nvs(0,&key);
        if (err != ESP_OK) printf("Error (%s) writing data to NVS!\n", esp_err_to_name(err));
        mqtt_log(event,"[SUCCESS] use_url is now set to 0","");

    }else if(strcmp(str_value,"1")==0){
        esp_err_t err = write_int8_to_nvs(1,&key);
        if (err != ESP_OK) printf("Error (%s) writing data to NVS!\n", esp_err_to_name(err));
        mqtt_log(event,"[SUCCESS] use_url is now set to 1","");
    }else{
        mqtt_log(event,"[ERROR] Message is not 0 or 1","");
    }


}

void update_run(esp_mqtt_event_handle_t event){

    char *firmware_update_url = get_firmware_update_url();
    if( Semaphore_Update_Started != NULL )
    {
        if( xSemaphoreTake( Semaphore_Update_Started, ( TickType_t ) 10 ) == pdTRUE )
        {
            mqtt_log(event,"[SUCCESS] OTA-Update started","");
            xTaskCreate(&ota_update, "ota_update", 1024 * 8, (void *) firmware_update_url, 1, NULL);
        }
        else
        {
            mqtt_log(event,"[ERROR] Update Process is running (maybe triggerd by Interval Timer)- Cancel Interval","");
        }
    }
}

void restart(esp_mqtt_event_handle_t event){
    mqtt_log(event,"[SUCCESS] Restart ESP32...","");
    ESP_LOGE(TAG, " Restart ESP32...");
    esp_restart();
}

void update_interval(esp_mqtt_event_handle_t event){
    char *key = "interval";
    if(event->data_len == 0){
        mqtt_log(event,"[ERROR] no interval provided","");
        ESP_LOGE(TAG, "no interval provided");
        return;
    }else if(event->data_len > 7){
        mqtt_log(event,"[ERROR] interval must be lower that 9.999.999","");
        ESP_LOGE(TAG, "interval must be lower that 9.999.999");
        return;
    }

    char *str_value = malloc(event->data_len);
    //save url into nvs
    strncpy(str_value,event->data,event->data_len);
    str_value[event->data_len] = '\0';
    
    int32_t int_value = atoi(str_value);
    if(int_value < 300){
        mqtt_log(event,"[ERROR] Update Interval is lower than 300","");
        ESP_LOGE(TAG, "Update Interval is lower than 300");
        return;
    }

    esp_err_t err = write_int32_to_nvs(int_value, &key);
    if (err != ESP_OK){
        mqtt_log(event,"[ERROR] NVS Cloud not write:",str_value);
    }else{
        mqtt_log(event,"[SUCCESS] Update Interval set: ",str_value);
        ESP_LOGI(TAG, "Update Interval set: %s", str_value);
    }
    
}

esp_err_t mqtt_log(esp_mqtt_event_handle_t event,char *message,char *var){
    char buf[512];

    strcpy(buf,TAG);strcat(buf,": ");strcat(buf,message);strcat(buf,var);
    esp_mqtt_client_publish(event->client, ESP_LOG, buf, 0, 0, 0);
    return ESP_OK;
}

esp_err_t mqtt_subscibe_action_handler(esp_mqtt_event_handle_t event){

    if(strncmp(UPDATE_URL,event->topic, event->topic_len) == 0){
        update_url(event);
    }else if (strncmp(UPDATE_RUN,event->topic, event->topic_len) == 0){
        update_run(event);
    }else if (strncmp(UPDATE_USE_URL,event->topic, event->topic_len) == 0){
       update_use_url(event);
    }else if (strncmp(UPDATE_INTERVAL,event->topic, event->topic_len) == 0){
        update_interval(event);
    }else if (strncmp(RESTART,event->topic, event->topic_len) == 0){
        restart(event);
    }
    return ESP_OK;
}

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:

            esp_mqtt_client_subscribe(client, UPDATE_URL, 0);
            esp_mqtt_client_subscribe(client, UPDATE_RUN, 0);
            esp_mqtt_client_subscribe(client, UPDATE_USE_URL, 0);
            esp_mqtt_client_subscribe(client, UPDATE_INTERVAL, 0);
            esp_mqtt_client_subscribe(client, RESTART, 0);

            mqtt_log(event,"ESP32 successfully booted and connected (^_^)","");

            //Display Firmware Version to mqqt and console
            const esp_partition_t *running = esp_ota_get_running_partition();
            esp_app_desc_t running_app_info;
            if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                mqtt_log(event,"Running firmware version: ",running_app_info.version);
            }
            
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            mqtt_subscibe_action_handler(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}
void mqtt_app_start()
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtts://192.168.178.123:8883",
        .event_handle = mqtt_event_handler,
        .client_cert_pem = (const char *)client_cert_pem_start,
        .client_key_pem = (const char *)client_key_pem_start,
        .cert_pem = (const char *)ca_cert_pem_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}