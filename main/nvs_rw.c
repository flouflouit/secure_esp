#include "nvs_rw.h"
#include "nvs.h"


#include <stdio.h>
#include <string.h>

#define STORAGE_NAMESPACE "storage"

esp_err_t read_string_from_nvs(char **value, char **key)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    char* tmp_value;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read
    size_t required_size = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_str(my_handle, *key, NULL, &required_size);

    //Key does not exist = return
    if (err != ESP_OK) return err;

    *value = malloc(required_size);    
    tmp_value = malloc(required_size);

    err = nvs_get_str(my_handle, *key, tmp_value, &required_size);
    if (err != ESP_OK) return err;

    strcpy(*value,tmp_value);

    free(tmp_value);
    nvs_close(my_handle);

    return ESP_OK;
}

esp_err_t write_string_to_nvs(char **value, char **key)
{
    nvs_handle_t my_handle;
    esp_err_t err;
   
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;


    // Write
    err = nvs_set_str(my_handle, *key, *value);
    if (err != ESP_OK) return err;

    // Commit and close
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t write_int8_to_nvs(int8_t value, char **key){

    nvs_handle_t my_handle;
    esp_err_t err;
   
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    err = nvs_set_i8(my_handle, *key, value);
    if (err != ESP_OK) return err;

    // Commit and close
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_int8_from_nvs(int8_t *value, char **key){

    nvs_handle_t my_handle;
    esp_err_t err;
   
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    err = nvs_get_i8(my_handle, *key, value);
    if (err != ESP_OK) return err;

    // Commit and close
    nvs_close(my_handle);
    return ESP_OK;
}


esp_err_t write_int32_to_nvs(int32_t value, char **key){

    nvs_handle_t my_handle;
    esp_err_t err;
   
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    err = nvs_set_i32(my_handle, *key, value);
    if (err != ESP_OK) return err;

    // Commit and close
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_int32_from_nvs(int32_t *value, char **key){

    nvs_handle_t my_handle;
    esp_err_t err;
   
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    err = nvs_get_i32(my_handle, *key, value);
    if (err != ESP_OK) return err;

    // Commit and close
    nvs_close(my_handle);
    return ESP_OK;
}

