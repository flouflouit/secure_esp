#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include "esp_err.h"

esp_err_t write_string_to_nvs(char **, char **);
esp_err_t read_string_from_nvs(char **, char **);

esp_err_t write_int8_to_nvs(int8_t,char **);
esp_err_t read_int8_from_nvs(int8_t*,char **);


esp_err_t write_int32_to_nvs(int32_t , char **);
esp_err_t read_int32_from_nvs(int32_t *, char **);

#endif
