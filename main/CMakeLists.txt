idf_component_register(SRCS "interval.c" "ota.c" "mqtt.c" "nvs_rw.c" "secure_esp.c" 
                    INCLUDE_DIRS "."
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/main/ota_certs/update_cert.pem ${project_dir}/main/mqtt_certs/client.crt ${project_dir}/main/mqtt_certs/client.key ${project_dir}/main/mqtt_certs/ca.crt)
