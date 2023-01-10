#!/bin/bash -e

WEBUSER=$(grep "^WebUser" config | cut -d "=" -f 2 | tr -d " " | cut -d "#" -f 1)
WEBPWD=$(grep "^WebPassword" config | cut -d "=" -f 2 | tr -d " " | cut -d "#" -f 1)
IPADDR=$(grep "^upload_port" platformio.ini | cut -d "=" -f 2 | tr -d ' ')

curl --digest --user ${WEBUSER}:${WEBPWD} http://${IPADDR}/count_direction
