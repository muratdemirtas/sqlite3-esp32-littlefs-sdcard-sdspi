/*
 * sensor_data_logger.c
 *
 *  Created on: Mar 3, 2022
 *      Author: DONE
 */
#include <sdmmc_cmd.h>
#include <driver/sdmmc_defs.h>
#include <freertos/task.h>
#include "lfs_port.h"
sdmmc_card_t *sdCardInstance;

static uint8_t read_buffer[512];
static uint8_t prog_buffer[512];
static uint8_t lookahead_buffer[512];
lfs_t lfs_filesystem;
lfs_file_t lfs_file;
uint8_t rx_buffer[512];
uint8_t tx_buffer[512];
#include "lfs.h"
// #define CONFIG_LITTLE_FS_IO_DEBUG 1

/**
 * LittleFS disk io erase function
 * @param c littlefs config structure
 * @param block block address
 * @return returns ok on success
 */
 int lfs_deskio_erase(const struct lfs_config *c, lfs_block_t block)
{
    return LFS_ERR_OK;
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
    printf("[ERASE]block %lu\n",(uint32_t)block);
#endif
    esp_err_t err = sdmmc_erase_sectors(sdCardInstance, block,
                                        1, SDMMC_ERASE_ARG);
    if(err != ESP_OK){
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
        printf("[ERASE]Failed at block %lu, err %d", (uint32_t)block, err);
#endif
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
/**
 * LittleFS disk io read function
 * @param c littlefs config structure
 * @param block block address
 * @param off offset address
 * @param buffer read buffer
 * @param size read buffer size
 * @return return ok on success
 */
 int lfs_deskio_read(const struct lfs_config *c,
                           lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    void* tmp_buf = heap_caps_malloc(512, MALLOC_CAP_DMA);
    uint32_t current_tick = xTaskGetTickCount();
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
    printf("[READ]block addr %d, offset %d, size %d\n",block, off,size);
#endif
    esp_err_t ret = sdmmc_read_sectors(sdCardInstance, tmp_buf, block, 1);
    if(ret != ESP_OK){
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
        printf("[READ]Error on read, code: %d\n", ret);
#endif
        return LFS_ERR_IO;
    }
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
    uint32_t elapsed = xTaskGetTickCount() - current_tick;
    elapsed = elapsed * portTICK_PERIOD_MS;
    printf("[READ]512 byte reading in %d ms\n", elapsed);
#endif

    memcpy(buffer,tmp_buf,512);
    free(tmp_buf);
    return LFS_ERR_OK;
}
/**
 * LittleFS disk io write function
 * @param c littlefs config structure
 * @param block block address
 * @param off offset address
 * @param buffer read buffer
 * @param size read buffer size
 * @return return ok on success
 */
 int lfs_deskio_prog(const struct lfs_config *c, lfs_block_t block,
                           lfs_off_t off, const void *buffer, lfs_size_t size)
{
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
    printf("[WRITE]block %d, offset %d, size %d\n",block,
            off,size);
#endif
    esp_err_t ret = sdmmc_write_sectors(sdCardInstance, buffer, block, 1);
    if(ret != ESP_OK){
#ifdef CONFIG_LITTLE_FS_IO_DEBUG
        printf("[WRITE]Error on write, code: %d\n", ret);
#endif
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
















void app_test(sdmmc_card_t *sdCard) {
    sdCardInstance = sdCard;
    for(int i = 0 ; i < 20000; i++)
    {
        memset(rx_buffer,0,512);
        memset(tx_buffer,0,512);
        for(int i = 0; i < 512; i++)
            rx_buffer[i] = i%255;

        sdmmc_erase_sectors(sdCardInstance, i,1,SDMMC_ERASE_ARG);

        sdmmc_write_sectors(sdCardInstance, rx_buffer,i, 1 );


        sdmmc_read_sectors(sdCardInstance, tx_buffer, i, 1);

        if(memcmp(tx_buffer, rx_buffer, 512) == 0){
            printf("TEST OK, index %d\n", i);
        }else
            printf("TEST FAIL\n");



    }
}














static int lfs_deskio_sync(const struct lfs_config *c)
{

	return LFS_ERR_OK;
}


const struct lfs_config cfg =
{
	.read  = lfs_deskio_read,
	.prog  = lfs_deskio_prog,
	.erase = lfs_deskio_erase,
	.sync  = lfs_deskio_sync,
	.read_size = 512,
	.prog_size = 512,
	.block_size = 512,
	.block_count = 30560256,
    .block_cycles = 500,
	.cache_size = 512,
	.lookahead_size = 512,
	.read_buffer = read_buffer,
	.prog_buffer = prog_buffer,
	.lookahead_buffer = lookahead_buffer

};
int32_t Application_Read_File_Size(char file_name[])
{
    int32_t current_size = 0;


    lfs_file_open(&lfs_filesystem, &lfs_file, file_name, LFS_O_RDONLY);
    current_size = lfs_file_size(&lfs_filesystem, &lfs_file);
    lfs_file_close(&lfs_filesystem, &lfs_file);

    return current_size;
}bool Application_File_Remove(char file_name[] )
{
    int error = 0;


    error = lfs_remove(&lfs_filesystem, file_name);

    if(error > 1)
        return false;
    else
        return true;
}int lfs_ls(lfs_t *lfs, const char *path) {
    lfs_dir_t dir;
    int err = lfs_dir_open(lfs, &dir, path);
    if (err) {
        return err;
    }

    struct lfs_info info;
    while (true) {
        int res = lfs_dir_read(lfs, &dir, &info);
        if (res < 0) {
            return res;
        }

        if (res == 0) {
            break;
        }

        switch (info.type) {
            case LFS_TYPE_REG: printf("reg "); break;
            case LFS_TYPE_DIR: printf("dir "); break;
            default:           printf("?   "); break;
        }

        static const char *prefixes[] = {"", "K", "M", "G"};
        for (int i = sizeof(prefixes)/sizeof(prefixes[0])-1; i >= 0; i--) {
            if (info.size >= (1 << 10*i)-1) {
                printf("%*u%sB ", 4-(i != 0), info.size >> 10*i, prefixes[i]);
                break;
            }
        }

        printf("%s\n", info.name);
    }

    err = lfs_dir_close(lfs, &dir);
    if (err) {
        return err;
    }

    return 0;
}

char str[512];
void Application_Append_File_Text(char file_name[], char buffer[], int size );
void LittleFS_Mount(sdmmc_card_t *sdCard){

    sdCardInstance = sdCard;

    int err = lfs_mount(&lfs_filesystem, &cfg);


    for(int i = 0; i < 510;i++)
        str[i] = 'a';

    str[511] = '\n';
	if (err)
	{

        {
            printf("mount failed %d\n", err);
        }
	}else{
        printf("lfs ok\n");
       // for(int i = 0 ; i < 300; i++){
         //   Application_Append_File_Text("/deneme.txt",str,200);


        //}

        lfs_ls(&lfs_filesystem, "/");

	}
}


void Application_Append_File_Text(char file_name[], char buffer[], int size)
{
    int file_open = 0;
    file_open =  lfs_file_open(&lfs_filesystem, &lfs_file,
                               file_name, LFS_O_APPEND | LFS_O_RDWR |LFS_O_CREAT);

    if(file_open < 0){
        printf("Cannot open file\n");
    }else{
        lfs_file_write(&lfs_filesystem, &lfs_file,
                       buffer, 	size);

        printf("File size %d\n", lfs_file_size(&lfs_filesystem, &lfs_file));

        lfs_file_close(&lfs_filesystem, &lfs_file);
    }

}





