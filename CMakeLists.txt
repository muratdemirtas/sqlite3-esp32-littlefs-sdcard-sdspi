idf_component_register(SRCS
        "sd_card_example_main.cpp"
        "lfs_util.c"
        "lfs.c"
        "esp32.c"
        "sqlite3.c" "esp32.c" "shox96_0_2.c"
        "sensor_data_logger.cpp"
        PRIV_INCLUDE_DIRS "private_include"
        INCLUDE_DIRS "include"
        REQUIRES driver mbedtls
        PRIV_REQUIRES console spiffs sdmmc soc)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu99 -g3 -fno-stack-protector -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls -nostdlib -Wpointer-arith -Wno-error=unused-value -Wno-error=unused-label -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wno-error=char-subscripts -Wno-error=maybe-uninitialized -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration -MMD -c -DF_CPU=240000000L -DESP32 -DCORE_DEBUG_LEVEL=0 -DNDEBUG)
