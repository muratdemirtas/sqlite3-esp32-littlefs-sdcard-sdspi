/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SPI peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <driver/rtc_io.h>
#include <esp_task_wdt.h>
#include <driver/spi_common.h>
#include <driver/sdspi_host.h>
#include <esp_log.h>
#include <esp_chip_info.h>


#include "sdmmc_cmd.h"
#include "lfs_port.h"


extern "C" {
void app_main(void);
}
static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

#if CONFIG_IDF_TARGET_ESP32S2
#define SPI_DMA_CHAN    host.slot
#elif CONFIG_IDF_TARGET_ESP32C3
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO
#else
#define SPI_DMA_CHAN    1
#endif

void app_main2();

void app_main(void)
{
    rtc_gpio_hold_dis(GPIO_NUM_4);
    esp_err_t ret;
    esp_err_t err = ESP_OK;
    sdmmc_card_t sdCard;
    /*@formatter:off*/
    spi_bus_config_t spiBusConfig = {
            .mosi_io_num = GPIO_NUM_13,
            .miso_io_num = GPIO_NUM_12,
            .sclk_io_num = GPIO_NUM_14,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,     ///< GPIO pin for spi data4 signal in octal mode, or -1 if not used.
            .data5_io_num = -1,     ///< GPIO pin for spi data5 signal in octal mode, or -1 if not used.
            .data6_io_num = -1,     ///< GPIO pin for spi data6 signal in octal mode, or -1 if not used.
            .data7_io_num = -1,     ///< GPIO pin for spi data7 signal in octal mode, or -1 if not used.
            .max_transfer_sz = 0, ///< Maximum transfer size, in bytes. Defaults to 4092 if 0 when DMA enabled, or to `SOC_SPI_MAXIMUM_BUFFER_SIZE` if DMA is disabled.
            .flags = 0,                	///< Abilities of bus to be checked by the driver. Or-ed value of ``SPICOMMON_BUSFLAG_*`` flags.
            .intr_flags = 0
    };/*@formatter:on*/

//Gonna experiment with a seperate SPI device module
    err= spi_bus_initialize(SPI2_HOST, &spiBusConfig, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init spi2host");
    }

    spi_device_interface_config_t deviceCfg = {
            .mode = 0,
            .clock_speed_hz = 40 * 1000 * 1000,
            .spics_io_num = 0,
           // .flags = SPI_DEVICE_HALFDUPLEX,
            .queue_size = 1,

    };
    spi_device_handle_t spiDevice;
     err = spi_bus_add_device(SPI2_HOST, &deviceCfg, &spiDevice);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init spi2host");
        spi_bus_free(SPI2_HOST);

    }
    else{
        ESP_LOGE(TAG, " ok spi2host");
    }


    sdspi_device_config_t SDSPIDeviceConfig = { .host_id = SPI2_HOST,   ///< SPI host to use, SPIx_HOST (see spi_types.h).
            .gpio_cs = static_cast<gpio_num_t>(0),     ///< GPIO number of CS signal
            .gpio_cd = GPIO_NUM_NC,     ///< GPIO number of card detect signal
            .gpio_wp = GPIO_NUM_NC,     ///< GPIO number of write protect signal
            .gpio_int = GPIO_NUM_NC,    ///< GPIO number of interrupt line (input) for SDIO card.
    };

    sdmmc_host_t SDSPIHost = SDSPI_HOST_DEFAULT();
    SDSPIHost.flags = SDMMC_HOST_FLAG_SPI;
    SDSPIHost.slot = SDSPIDeviceConfig.host_id;
    SDSPIHost.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdspi_dev_handle_t out_handle;
    sdspi_host_init();

    if( sdspi_host_init_device(&SDSPIDeviceConfig, &out_handle) == ESP_OK)
        printf("ESP host init ok\n");
    else
        printf("ESP host init fail\n");



   if( sdspi_host_set_card_clk(  out_handle, 20000)== ESP_OK)
       printf("ESP set speed ok\n");
   else
       printf("ESP set speed fail\n");

    sdmmc_card_init(&SDSPIHost, &sdCard);



    // Card has been initialized, print its properties
#if LOG_LOCAL_LEVEL > ESP_LOG_WARN
    sdmmc_card_print_info(stdout, &sdCard);
#endif
    printf("SD Card block size %d, read len %d, capacity %d\n",
           sdCard.csd.sector_size,
           sdCard.csd.read_block_len,
           sdCard.csd.capacity);

    ESP_LOGI(TAG, "SD card info:");
    ESP_LOGI(TAG, "\tBus width (log2): %d", sdCard.log_bus_width);
    ESP_LOGI(TAG, "\tFreq (kHz): %d", sdCard.max_freq_khz);
    ESP_LOGI(TAG, "\tDDR: %d", sdCard.is_ddr);
    ESP_LOGI(TAG, "\tCID: Date %d, MFG_ID %d, Name %s, OEM ID %d, Rev %d, Serial %d", sdCard.cid.date, sdCard.cid.mfg_id, sdCard.cid.name, sdCard.cid.oem_id, sdCard.cid.revision, sdCard.cid.serial);
    ESP_LOGI(TAG, "\tCSD: Capacity %d, Card Common Class %d, CSD version %d, MMC version %d, read block len %d, sector size %d, tr speed %d", sdCard.csd.capacity, sdCard.csd.card_command_class, sdCard.csd.csd_ver, sdCard.csd.mmc_ver, sdCard.csd.read_block_len, sdCard.csd.sector_size, sdCard.csd.tr_speed);
    ESP_LOGI(TAG, "\tCSD: Ease mem state %d, Power class %d, Revision %d, Sec feature %d", sdCard.ext_csd.erase_mem_state, sdCard.ext_csd.power_class, sdCard.ext_csd.rev, sdCard.ext_csd.sec_feature);
    ESP_LOGI(TAG, "\tSCR: bus width %d, erase mem state %d, reserved %d, rsvd_mnf %d, sd_spec %d", sdCard.scr.bus_width, sdCard.scr.erase_mem_state, sdCard.scr.reserved, sdCard.scr.rsvd_mnf, sdCard.scr.sd_spec);
    ESP_LOGI(TAG, "\tSSR: cur_bus_width %d, discard_support %d, fule_support %d, reserved %d", sdCard.ssr.cur_bus_width, sdCard.ssr.discard_support, sdCard.ssr.fule_support, sdCard.ssr.reserved);


     LittleFS_Mount(&sdCard);

     app_main2();

    for(;;){
        vTaskDelay(100);
    }
}

/*
    This example opens Sqlite3 databases from SD Card and
    retrieves data from them.
    Before running please copy following files to SD Card:
    data/mdr512.db
    data/census2000names.db
*/
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "esp_err.h"
#include "esp_log.h"

#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sqlite3.h"
#include "sdmmc_cmd.h"



// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName){
    int i;
    printf("%s: ", (const char*)data);
    for (i = 0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int openDb(const char *filename, sqlite3 **db) {
    int rc = sqlite3_open(filename, db);
    if (rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    } else {
        printf("Opened database successfully\n");
    }
    return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
    printf("%s\n", sql);

    int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        printf("Operation done successfully\n");
    }

    return rc;
}

void app_main2()
{
    sqlite3 *db1;

    int rc;


    sqlite3_initialize();

    // Open database 1
    if (openDb("JanStore.db", &db1))
        return;


    rc = db_exec(db1, "SELECT main.Identity.rawData FROM main.Identity");
    if (rc != SQLITE_OK) {
        ESP_LOGI(TAG, "Card unmounted");
        sqlite3_close(db1);
        return;
    }
    ESP_LOGI(TAG, "Card unmounted2");

    sqlite3_close(db1);


    // All done, unmount partition and disable SDMMC or SPI peripheral



    //while(1);
}
