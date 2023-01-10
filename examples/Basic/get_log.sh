#!/bin/bash -e

WEBUSER=$(grep "^WebUser" config | cut -d "=" -f 2 | tr -d " " | cut -d "#" -f 1)
WEBPWD=$(grep "^WebPassword" config | cut -d "=" -f 2 | tr -d " " | cut -d "#" -f 1)
IPADDR=$(grep "^upload_port" platformio.ini | cut -d "=" -f 2 | tr -d ' ')

if [[ $# == 1 ]]; then
    SKIP="?skip=${1}"
else
    SKIP=""
fi
curl --digest --user ${WEBUSER}:${WEBPWD} http://${IPADDR}/logHistory${SKIP}
