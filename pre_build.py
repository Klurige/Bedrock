""" Create version header and tracker file if missing """
import subprocess
import datetime
import os
import base64

# Import if platformio environment is needed.
#Import("env")

# Apart from public key, the parameter length limit is enforced.
maxParameterLength = 24

proc = subprocess.run(['git', 'describe', '--tags',
                      '--always', '--dirty'], capture_output=True)
GIT_VERSION = proc.stdout.decode("UTF-8").strip()

CONFIG_HEADER_FILE = 'bedrock_config.h'
CONFIG_SRC_FILE = 'bedrock_config.cpp'
CONFIG_HEADER_PATH = ''
CONFIG_SRC_PATH = ''

PUBLIC_KEY_FILE = 'public.key'
public_key = open(PUBLIC_KEY_FILE, "r").read().replace('\n', '\\n')

CONFIG_FILE = 'config'

COMMON_HEADER = """

// AUTO GENERATED FILE, DO NOT EDIT
// DO NOT STORE IN VERSION CONTROL

"""

CONFIG_TOP_HEADER = """
#ifndef __BEDROCK_CONFIG_H__
#define __BEDROCK_CONFIG_H__

"""

CONFIG_BOTTOM_HEADER = """
extern const char* const html_files[][2];
extern const char* const config_key_values[][2];

#endif // __BEDROCK_CONFIG_H__
"""

CONFIG_TOP_SRC = """
#ifndef ARDUINO
#define PROGMEM
#else
#include <Arduino.h>
#endif

#include <{}>
""".format(CONFIG_HEADER_FILE)

CONFIG_SRC_AUTO = """
const char config_key_0[] PROGMEM = "Version";
const char config_value_0[] PROGMEM = "{}";
const char config_key_1[] PROGMEM = "BuildTimestamp";
const char config_value_1[] PROGMEM = "{}";
const char config_key_2[] PROGMEM = "PublicKey";
const char config_value_2[] PROGMEM = "{}";
""".format(GIT_VERSION, datetime.datetime.now(), public_key)

if os.environ.get('PLATFORMIO_INCLUDE_DIR') is not None:
    CONFIG_HEADER_PATH = os.environ.get(
        'PLATFORMIO_INCLUDE_DIR') + os.sep + CONFIG_HEADER_FILE
elif os.path.exists("include"):
    CONFIG_HEADER_PATH = "include" + os.sep + CONFIG_HEADER_FILE
else:
    PROJECT_DIR = env.subst("$PROJECT_DIR")
    os.mkdir(PROJECT_DIR + os.sep + "include")
    CONFIG_HEADER_PATH = "include" + os.sep + CONFIG_HEADER_FILE

if os.environ.get('PLATFORMIO_SRC_DIR') is not None:
    CONFIG_SRC_PATH = os.environ.get(
        'PLATFORMIO_SRC_DIR') + os.sep + CONFIG_SRC_FILE
elif os.path.exists("src"):
    CONFIG_SRC_PATH = "src" + os.sep + CONFIG_SRC_FILE
else:
    PROJECT_DIR = env.subst("$PROJECT_DIR")
    os.mkdir(PROJECT_DIR + os.sep + "src")
    CONFIG_SRC_PATH = "src" + os.sep + CONFIG_SRC_FILE

numConfigParams = 0

with open(CONFIG_SRC_PATH, 'w+') as FILE:
    FILE.write(COMMON_HEADER)
    FILE.write("#ifndef ARDUINO\n")
    FILE.write("#define PROGMEM\n")
    FILE.write("#else\n")
    FILE.write("#include <Arduino.h>\n")
    FILE.write("#endif\n\n")
    FILE.write("#include <{}>\n\n".format(CONFIG_HEADER_FILE))
    contents = os.listdir("html")
    num_files = 0
    for f in contents:
        if f.startswith('.') == False:
            FILE.write("const char html_filename_" +
                       str(num_files) + "[] PROGMEM = \"/" + f + "\";\n")
            FILE.write("const char html_contents_" +
                       str(num_files) + "[] PROGMEM = \"")
            with open("html/" + f, 'rb') as binary_file:
                binary_file_data = binary_file.read()
                base64_encoded_data = base64.b64encode(binary_file_data)
                base64_message = base64_encoded_data.decode('utf-8')
                FILE.write(base64_message)
            FILE.write("\";\n")
            num_files += 1
    FILE.write("\nconst char *const html_files[][2] PROGMEM = {\n")
    for file_index in range(num_files):
        FILE.write("\t{html_filename_" + str(file_index) +
                   ", html_contents_" + str(file_index) + "},\n")

    FILE.write("\t\"\", \"\"};\n")

    FILE.write(CONFIG_SRC_AUTO)
    configFile = open(CONFIG_FILE, 'r')
    numConfigParams = 3 # Version, Timestamp and public key.
    while True:
        line = configFile.readline()
        if not line:
            break
        line = line.strip()
        try:
            comment = line.index("#")
            line = line[:comment]
        except:
            pass
        if not line:
            continue
        param = line.strip().split("=")
        if(len(param) != 2):
            FILE.write(
                "#error config file contains error near line {}.\n".format(numConfigParams))
        else:
            FILE.write("const char config_key_" + str(numConfigParams) + "[] PROGMEM = \"" + param[0].strip() + "\";\n")
            FILE.write("const char config_value_" + str(numConfigParams) + "[] PROGMEM = \"" + param[1].strip() + "\";\n")
            numConfigParams = numConfigParams + 1
            keylen = len(param[0].strip())
            if(keylen > maxParameterLength):
                FILE.write("#error \"Max key length is " + str(maxParameterLength) + ", but the above is longer.\"\n")
            if((param[0].strip() != "PublicKey") & (param[0].strip() != "WifiPassword")):
                valuelen = len(param[1].strip())
                if(valuelen > maxParameterLength):
                    FILE.write("#error \"Max value length is " + str(maxParameterLength) + ", but the above is longer.\"\n")

    FILE.write("\nconst char *const config_key_values[][2] PROGMEM = {\n")
    for config_index in range(numConfigParams):
        FILE.write("\t{config_key_" + str(config_index) +
                   ", config_value_" + str(config_index) + "},\n")
    FILE.write("\t{\"\", \"\"}\n};\n\n")

with open(CONFIG_HEADER_PATH, 'w+') as FILE:
    FILE.write(COMMON_HEADER)
    FILE.write(CONFIG_TOP_HEADER)
    FILE.write("#define BEDROCK_CONFIG_NUMBER_OF_PARAMS " + str(numConfigParams) + "\n")
    FILE.write("#define BEDROCK_CONFIG_KEY_VALUE_MAX_LENGTH " + str(maxParameterLength) + "\n")
    FILE.write(CONFIG_BOTTOM_HEADER)

