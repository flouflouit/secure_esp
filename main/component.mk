#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_EMBED_TXTFILES :=  ${PROJECT_PATH}/main/ota_certs/update_cert.pem ${PROJECT_PATH}/main/mqtt_certs/client.crt ${PROJECT_PATH}/main/mqtt_certs/client.key ${PROJECT_PATH}/main/mqtt_certs/ca.crt
