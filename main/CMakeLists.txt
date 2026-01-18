## This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

idf_component_register(SRCS main.c fm_tx.c wav_parser.c mp3_parser.c
                       INCLUDE_DIRS . ../include
                       REQUIRES esp_wifi esp_event esp_netif esp_http_server esp_vfs_spiffs)
