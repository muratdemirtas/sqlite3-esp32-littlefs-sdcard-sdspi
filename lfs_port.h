//
// Created by murat on 26.06.2022.
//

#ifndef SD_CARD_LFS_PORT_H
#define SD_CARD_LFS_PORT_H

#include <driver/sdspi_host.h>
#include "lfs.h"
extern void LittleFS_Mount(sdmmc_card_t *sdCard);
extern void app_test(sdmmc_card_t *sdCard);
extern lfs_t lfs_filesystem;
extern lfs_file_t lfs_file;
/**
 * LittleFS disk io write function
 * @param c littlefs config structure
 * @param block block address
 * @param off offset address
 * @param buffer read buffer
 * @param size read buffer size
 * @return return ok on success
 */
extern int lfs_deskio_prog(const struct lfs_config *c, lfs_block_t block,
                    lfs_off_t off, const void *buffer, lfs_size_t size);

/**
 * LittleFS disk io read function
 * @param c littlefs config structure
 * @param block block address
 * @param off offset address
 * @param buffer read buffer
 * @param size read buffer size
 * @return return ok on success
 */
extern int lfs_deskio_read(const struct lfs_config *c,
                    lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
/**
 * LittleFS disk io erase function
 * @param c littlefs config structure
 * @param block block address
 * @return returns ok on success
 */
extern int lfs_deskio_erase(const struct lfs_config *c, lfs_block_t block);
#endif //SD_CARD_LFS_PORT_H
