# Bedrock
## A foundation for writing C/C++ applications for ESP8266, providing a web ui, sse and mqtt interfaces.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Build configuration (Note that memory usage is affected by these parameters):
;    -DBEDROCK_LOG_ON=true               # Set to true to turn on logging. False or undefined to turn off all logging.
;    -DBEDROCK_LOG_SIZE=100              # Number of messages stored. The oldest messages will be discarded if full.
;    -DBEDROCK_LOG_MESSAGE_LENGTH=120    # Maximum length of each log message. Default is 100 chars.
;    -DBEDROCK_DEBUG_SERIAL=true         # Log to serial, as well as web log.
;    -DBEDROCK_DEBUG_FILES=true          # Log file system accesses.
;    -DBEDROCK_DEBUG_MQTT=true           # Log mqtt events.
;    -DBEDROCK_DEBUG_WEB=true            # Log web server (ui and sse) events.
;    -DBEDROCK_DEBUG_MISC=true           # Log messages that don't fit in other categories. Only if LOG_ON is true.
;    -DBEDROCK_MQTT_MAX_SUBSCRIPTIONS=8  # Maximum number of MQTT subscriptions. Mandatory.
;    -DBEDROCK_SSE_MAX_EVENTS=8          # Maximum number of sse events. Mandatory.
;    -DBEDROCK_WIPE_FILESYSTEM=false     # Wipe filesystem on every boot. Use with caution. Typically only if a corrupt file was uploaded by mistake.
;
; The upload is done by requesting over ssh the server to send ota to the target.
; Pre-authorized ssh access to the server is required.
;
; Generate keys and place them in the same dir as this file:
;    openssl genrsa -out private.key 2048
;    openssl rsa -in private.key -outform PEM -pubout -out public.key
;
; Bootstrap:
;    Comment out all lines starting with "upload_" to flash using serial port.
;    Do not forget to uncomment after initial flash.
;    Note: Do not have anything connected to TX, RX while bootstrapping. These pins are used
;    for programming over serial.
;
; upload_port should be set to ip address.
; upload_flags should be set to: -P[PORT] -a[PASSWD] where
;    PORT is the server port to use.
;    PASSWD matches ota password in config.
;
; Make sure the server port is open in the firewall.
;
; Specify server name in the upload command. (Perhaps localhost, or your home assistant)
;
; Updating the html files can be done by running "./upload_html"
;
; Set build_flags = -DBEDROCK_DEBUG_SERIAL and connect a serial port to RX and TX to
; see debug info.
; If using FTDI USB: FTDI 1 (black) -> ESP GND
;                    FTDI 4 (orange) -> ESP RX
;                    FTDI 5 (yellow) -> ESP TX
; See monitor_speed, 8N1
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
