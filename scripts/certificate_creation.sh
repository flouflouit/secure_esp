#!/bin/bash

IP="192.168.178.123"
SUBJECT_CA="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=CA/CN=$IP"
SUBJECT_SERVER="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=MQTTServer/CN=$IP"
SUBJECT_CLIENT="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=MQTTClient/CN=$IP"
SUBJECT_UPDATE="/C=SE/ST=Stockholm/L=Stockholm/O=himinds/OU=UPDATEServer/CN=$IP"

function create_folders (){
   mkdir ~/esp/secure_esp/ca
   mkdir ~/esp/secure_esp/ca/keys
   mkdir ~/esp/secure_esp/ca/certs
   mkdir ~/esp/secure_esp/ca/req
   mkdir ~/esp/secure_esp/main/mqtt_certs
   mkdir ~/esp/secure_esp/main/ota_certs
}

function generate_CA () {
   echo "$SUBJECT_CA"
   openssl req -x509 -nodes -sha256 -newkey rsa:2048 -subj "$SUBJECT_CA"  -days 365 -keyout ~/esp/secure_esp/ca/keys/ca.key -out ~/esp/secure_esp/ca/certs/ca.crt
}

function generate_server () {
   echo "$SUBJECT_SERVER"
   openssl req -nodes -sha256 -new -subj "$SUBJECT_SERVER" -keyout ~/esp/secure_esp/ca/keys/server.key -out ~/esp/secure_esp/ca/req/server.csr
   openssl x509 -req -sha256 -in ~/esp/secure_esp/ca/req/server.csr -CA ~/esp/secure_esp/ca/certs/ca.crt -CAkey ~/esp/secure_esp/ca/keys/ca.key -CAcreateserial -out ~/esp/secure_esp/ca/certs/server.crt -days 365
}

function generate_client () {
   echo "$SUBJECT_CLIENT"
   openssl req -new -nodes -sha256 -subj "$SUBJECT_CLIENT" -out ~/esp/secure_esp/ca/req/client.csr -keyout ~/esp/secure_esp/ca/keys/client.key 
   openssl x509 -req -sha256 -in ~/esp/secure_esp/ca/req/client.csr -CA ~/esp/secure_esp/ca/certs/ca.crt -CAkey ~/esp/secure_esp/ca/keys/ca.key -CAcreateserial -out ~/esp/secure_esp/ca/certs/client.crt -days 365
}

function generate_update () {
   echo "$SUBJECT_UPDATE"
   openssl req -newkey rsa:2048 -subj "$SUBJECT_CLIENT" -days 60 -nodes -keyout ~/esp/secure_esp/ca/keys/update_key.pem -out ~/esp/secure_esp/ca/req/update_req.pem
   openssl x509 -req -in ~/esp/secure_esp/ca/req/update_req.pem -CA ~/esp/secure_esp/ca/certs/ca.crt -CAkey ~/esp/secure_esp/ca/keys/ca.key -CAcreateserial -out ~/esp/secure_esp/ca/certs/update_cert.pem
}

function copy_keys_to_esp_folders () {
   cp ~/esp/secure_esp/ca/certs/update_cert.pem ~/esp/secure_esp/main/ota_certs/update_cert.pem
   cp ~/esp/secure_esp/ca/certs/ca.crt ~/esp/secure_esp/main/mqtt_certs/ca.crt
   cp ~/esp/secure_esp/ca/certs/client.crt ~/esp/secure_esp/main/mqtt_certs/client.crt
   cp ~/esp/secure_esp/ca/keys/client.key ~/esp/secure_esp/main/mqtt_certs/client.key
}

function copy_keys_to_broker () {
   sudo cp ~/esp/secure_esp/ca/certs/ca.crt /etc/mosquitto/certs/
   sudo cp ~/esp/secure_esp/ca/certs/server.crt /etc/mosquitto/certs/
   sudo cp ~/esp/secure_esp/ca/keys/server.key /etc/mosquitto/certs/
}
create_folders
generate_CA
generate_server
generate_client
generate_update
copy_keys_to_esp_folders
#copy_keys_to_broker
