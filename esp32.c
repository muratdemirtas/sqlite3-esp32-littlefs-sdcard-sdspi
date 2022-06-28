/* From: https://chromium.googlesource.com/chromium/src.git/+/4.1.249.1050/third_party/sqlite/src/os_symbian.cc
 * https://github.com/spsoft/spmemvfs/tree/master/spmemvfs
 * http://www.sqlite.org/src/doc/trunk/src/test_demovfs.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfstrace.c
 * http://www.sqlite.org/src/doc/trunk/src/test_onefile.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfs.c
 * https://github.com/nodemcu/nodemcu-firmware/blob/master/app/sqlite3/esp8266.c
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sqlite3.h>
#include <esp_spi_flash.h>
#include <esp_system.h>
#include <rom/ets_sys.h>
#include <sys/stat.h>
#include <esp_random.h>
#include "lfs.h"
#include "shox96_0_2.h"
#include "lfs_port.h"

 #define dbg_printf(...) printf(__VA_ARGS__)

#define CACHEBLOCKSZ 64
#define esp32_DEFAULT_MAXNAMESIZE 100

// From https://stackoverflow.com/questions/19758270/read-varint-from-linux-sockets#19760246
// Encode an unsigned 64-bit varint.  Returns number of encoded bytes.
// 'buffer' must have room for up to 10 bytes.
int encode_unsigned_varint(uint8_t *buffer, uint64_t value) {
    int encoded = 0;
    do {
        uint8_t next_byte = value & 0x7F;
        value >>= 7;
        if (value)
            next_byte |= 0x80;
        buffer[encoded++] = next_byte;
    } while (value);
    return encoded;
}

uint64_t decode_unsigned_varint(const uint8_t *data, int *decoded_bytes) {
    int i = 0;
    uint64_t decoded_value = 0;
    int shift_amount = 0;
    do {
        decoded_value |= (uint64_t)(data[i] & 0x7F) << shift_amount;
        shift_amount += 7;
    } while ((data[i++] & 0x80) != 0);
    *decoded_bytes = i;
    return decoded_value;
}

int esp32_Close(sqlite3_file*);
int esp32_Lock(sqlite3_file *, int);
int esp32_Unlock(sqlite3_file*, int);
int esp32_Sync(sqlite3_file*, int);
int esp32_Open(sqlite3_vfs*, const char *, sqlite3_file *, int, int*);
int esp32_Read(sqlite3_file*, void*, int, sqlite3_int64);
int esp32_Write(sqlite3_file*, const void*, int, sqlite3_int64);
int esp32_Truncate(sqlite3_file*, sqlite3_int64);
int esp32_Delete(sqlite3_vfs*, const char *, int);
int esp32_FileSize(sqlite3_file*, sqlite3_int64*);
int esp32_Access(sqlite3_vfs*, const char*, int, int*);
int esp32_FullPathname( sqlite3_vfs*, const char *, int, char*);
int esp32_CheckReservedLock(sqlite3_file*, int *);
int esp32_FileControl(sqlite3_file *, int, void*);
int esp32_SectorSize(sqlite3_file*);
int esp32_DeviceCharacteristics(sqlite3_file*);
void* esp32_DlOpen(sqlite3_vfs*, const char *);
void esp32_DlError(sqlite3_vfs*, int, char*);
void (*esp32_DlSym (sqlite3_vfs*, void*, const char*))(void);
void esp32_DlClose(sqlite3_vfs*, void*);
int esp32_Randomness(sqlite3_vfs*, int, char*);
int esp32_Sleep(sqlite3_vfs*, int);
int esp32_CurrentTime(sqlite3_vfs*, double*);

int esp32mem_Close(sqlite3_file*);
int esp32mem_Read(sqlite3_file*, void*, int, sqlite3_int64);
int esp32mem_Write(sqlite3_file*, const void*, int, sqlite3_int64);
int esp32mem_FileSize(sqlite3_file*, sqlite3_int64*);
int esp32mem_Sync(sqlite3_file*, int);

typedef struct st_linkedlist {
    uint16_t blockid;
    struct st_linkedlist *next;
    uint8_t data[CACHEBLOCKSZ];
} linkedlist_t, *pLinkedList_t;

typedef struct st_filecache {
    uint32_t size;
    linkedlist_t *list;
} filecache_t, *pFileCache_t;

typedef struct esp32_file {
    sqlite3_file base;
    lfs_file_t *fd;
    int file_descriptor;
    filecache_t *cache;
    char name[esp32_DEFAULT_MAXNAMESIZE];
} esp32_file;

sqlite3_vfs  esp32Vfs = {
        1,			// iVersion
        sizeof(esp32_file),	// szOsFile
        101,	// mxPathname
        NULL,			// pNext
        "esp32",		// name
        0,			// pAppData
        esp32_Open,		// xOpen
        esp32_Delete,		// xDelete
        esp32_Access,		// xAccess
        esp32_FullPathname,	// xFullPathname
        esp32_DlOpen,		// xDlOpen
        esp32_DlError,	// xDlError
        esp32_DlSym,		// xDlSym
        esp32_DlClose,	// xDlClose
        esp32_Randomness,	// xRandomness
        esp32_Sleep,		// xSleep
        esp32_CurrentTime,	// xCurrentTime
        0			// xGetLastError
};

const sqlite3_io_methods esp32IoMethods = {
        1,
        esp32_Close,
        esp32_Read,
        esp32_Write,
        esp32_Truncate,
        esp32_Sync,
        esp32_FileSize,
        esp32_Lock,
        esp32_Unlock,
        esp32_CheckReservedLock,
        esp32_FileControl,
        esp32_SectorSize,
        esp32_DeviceCharacteristics
};

const sqlite3_io_methods esp32MemMethods = {
        1,
        esp32mem_Close,
        esp32mem_Read,
        esp32mem_Write,
        esp32_Truncate,
        esp32mem_Sync,
        esp32mem_FileSize,
        esp32_Lock,
        esp32_Unlock,
        esp32_CheckReservedLock,
        esp32_FileControl,
        esp32_SectorSize,
        esp32_DeviceCharacteristics
};

uint32_t linkedlist_store (linkedlist_t **leaf, uint32_t offset, uint32_t len, const uint8_t *data) {
    const uint8_t blank[CACHEBLOCKSZ] = { 0 };
    uint16_t blockid = offset/CACHEBLOCKSZ;
    linkedlist_t *block;

    if (!memcmp(data, blank, CACHEBLOCKSZ))
        return len;

    block = *leaf;
    if (!block || ( block->blockid != blockid ) ) {
        block = (linkedlist_t *) sqlite3_malloc ( sizeof( linkedlist_t ) );
        if (!block)
            return SQLITE_NOMEM;

        memset (block->data, 0, CACHEBLOCKSZ);
        block->blockid = blockid;
    }

    if (!*leaf) {
        *leaf = block;
        block->next = NULL;
    } else if (block != *leaf) {
        if (block->blockid > (*leaf)->blockid) {
            block->next = (*leaf)->next;
            (*leaf)->next = block;
        } else {
            block->next = (*leaf);
            (*leaf) = block;
        }
    }

    memcpy (block->data + offset%CACHEBLOCKSZ, data, len);

    return len;
}

uint32_t filecache_pull (pFileCache_t cache, uint32_t offset, uint32_t len, uint8_t *data) {
    uint16_t i;
    float blocks;
    uint32_t r = 0;

    blocks = ( offset % CACHEBLOCKSZ + len ) / (float) CACHEBLOCKSZ;
    if (blocks == 0.0)
        return 0;
    if (!cache->list)
        return 0;

    if (( blocks - (int) blocks) > 0.0)
        blocks = blocks + 1.0;

    for (i = 0; i < (uint16_t) blocks; i++) {
        uint16_t round;
        float relablock;
        linkedlist_t *leaf;
        uint32_t relaoffset, relalen;
        uint8_t * reladata = (uint8_t*) data;

        relalen = len - r;

        reladata = reladata + r;
        relaoffset = offset + r;

        round = CACHEBLOCKSZ - relaoffset%CACHEBLOCKSZ;
        if (relalen > round) relalen = round;

        for (leaf = cache->list; leaf && leaf->next; leaf = leaf->next) {
            if ( ( leaf->next->blockid * CACHEBLOCKSZ ) > relaoffset )
                break;
        }

        relablock = relaoffset/((float)CACHEBLOCKSZ) - leaf->blockid;

        if ( ( relablock >= 0 ) && ( relablock < 1 ) )
            memcpy (data + r, leaf->data + (relaoffset % CACHEBLOCKSZ), relalen);

        r = r + relalen;
    }

    return 0;
}

uint32_t filecache_push (pFileCache_t cache, uint32_t offset, uint32_t len, const uint8_t *data) {
    uint16_t i;
    float blocks;
    uint32_t r = 0;
    uint8_t updateroot = 0x1;

    blocks = ( offset % CACHEBLOCKSZ + len ) / (float) CACHEBLOCKSZ;

    if (blocks == 0.0)
        return 0;

    if (( blocks - (int) blocks) > 0.0)
        blocks = blocks + 1.0;

    for (i = 0; i < (uint16_t) blocks; i++) {
        uint16_t round;
        uint32_t localr;
        linkedlist_t *leaf;
        uint32_t relaoffset, relalen;
        uint8_t * reladata = (uint8_t*) data;

        relalen = len - r;

        reladata = reladata + r;
        relaoffset = offset + r;

        round = CACHEBLOCKSZ - relaoffset%CACHEBLOCKSZ;
        if (relalen > round) relalen = round;

        for (leaf = cache->list; leaf && leaf->next; leaf = leaf->next) {
            if ( ( leaf->next->blockid * CACHEBLOCKSZ ) > relaoffset )
                break;
            updateroot = 0x0;
        }

        localr = linkedlist_store(&leaf, relaoffset, (relalen > CACHEBLOCKSZ) ? CACHEBLOCKSZ : relalen, reladata);
        if (localr == SQLITE_NOMEM)
            return SQLITE_NOMEM;

        r = r + localr;

        if (updateroot & 0x1)
            cache->list = leaf;
    }

    if (offset + len > cache->size)
        cache->size = offset + len;

    return r;
}

void filecache_free (pFileCache_t cache) {
    pLinkedList_t ll = cache->list, next;

    while (ll != NULL) {
        next = ll->next;
        sqlite3_free (ll);
        ll = next;
    }
}

int esp32mem_Close(sqlite3_file *id)
{
    esp32_file *file = (esp32_file*) id;

    filecache_free(file->cache);
    sqlite3_free (file->cache);

    dbg_printf("esp32mem_Close: %s OK\n", file->name);
    return SQLITE_OK;
}

int esp32mem_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
    int32_t ofst;
    esp32_file *file = (esp32_file*) id;
    ofst = (int32_t)(offset & 0x7FFFFFFF);

    filecache_pull (file->cache, ofst, amount, (uint8_t *) buffer);

    dbg_printf("esp32mem_Read: %s [%d] [%d] OK\n", file->name, ofst, amount);
    return SQLITE_OK;
}

int esp32mem_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
    int32_t ofst;
    esp32_file *file = (esp32_file*) id;

    ofst = (int32_t)(offset & 0x7FFFFFFF);

    filecache_push (file->cache, ofst, amount, (const uint8_t *) buffer);

    dbg_printf("esp32mem_Write: %s [%d] [%d] OK\n", file->name, ofst, amount);
    return SQLITE_OK;
}

int esp32mem_Sync(sqlite3_file *id, int flags)
{
    esp32_file *file = (esp32_file*) id;
    dbg_printf("esp32mem_Sync: %s OK\n", file->name);
    return  SQLITE_OK;
}

int esp32mem_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
    esp32_file *file = (esp32_file*) id;

    *size = 0LL | file->cache->size;
    dbg_printf("esp32mem_FileSize: %s [%d] OK\n", file->name, file->cache->size);
    return SQLITE_OK;
}

int esp32_Open( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags )
{
    int rc;
    char mode[5];
    esp32_file *p = (esp32_file*) file;

    int open_flag = 0;
    strcpy(mode, "r");
    if ( path == NULL ) return SQLITE_IOERR;
    dbg_printf("esp32_Open: 0o %s %s\n", path, mode);
    if( flags&SQLITE_OPEN_READONLY ){
        open_flag |= LFS_O_RDONLY;
        strcpy(mode, "r");
    }

    if( flags&SQLITE_OPEN_READWRITE || flags&SQLITE_OPEN_MAIN_JOURNAL ) {
        int result;
        if (SQLITE_OK != esp32_Access(vfs, path, flags, &result))
            return SQLITE_CANTOPEN;

        if (result == 1){
            open_flag |= LFS_O_RDWR;
            strcpy(mode, "r+");
        }
        else{
            open_flag |= LFS_O_RDWR;
            open_flag |= LFS_O_CREAT;
            strcpy(mode, "w+");
        }

    }

    dbg_printf("esp32_Open: 1o %s %s\n", path, mode);
    memset (p, 0, sizeof(esp32_file));

    strncpy (p->name, path, esp32_DEFAULT_MAXNAMESIZE);
    p->name[esp32_DEFAULT_MAXNAMESIZE-1] = '\0';

    if( flags&SQLITE_OPEN_MAIN_JOURNAL ) {
        p->fd = 0;
        p->cache = (filecache_t *) sqlite3_malloc(sizeof (filecache_t));
        if (! p->cache )
            return SQLITE_NOMEM;
        memset (p->cache, 0, sizeof(filecache_t));

        p->base.pMethods = &esp32MemMethods;
        dbg_printf("esp32_Open: 2o %s MEM OK\n", p->name);
        return SQLITE_OK;
    }

    dbg_printf("[SQLite3]Opening file %s, with flag %d\n", p->name, open_flag);
    /* try to open file over littlefs */
    p->file_descriptor = lfs_file_open(&lfs_filesystem, &lfs_file, path, open_flag);
    /* check fd val, on error print debug message */
    if ( p->file_descriptor < 0 ) {
        dbg_printf("[SQLite3]Cannot open file %s, err %d\n", p->name, p->file_descriptor);
        return SQLITE_CANTOPEN;
    }
    /* set sqlite3 io methods */
    p->base.pMethods = &esp32IoMethods;
    /* print debug message */
    dbg_printf("[SQLite3]:File %s opened successfully,fd %d\n",
                                                    p->name, p->file_descriptor);
    /* return ok */
    return SQLITE_OK;
}

int esp32_Close(sqlite3_file *id)
{
    esp32_file *file = (esp32_file*) id;


    int rc = lfs_file_close(&lfs_filesystem, file->fd);
    dbg_printf("esp32_Close: %s %d\n", file->name, rc);
    return rc ? SQLITE_IOERR_CLOSE : SQLITE_OK;
}

int esp32_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
    size_t nRead;
    int32_t ofst, iofst;
    esp32_file *file = (esp32_file*) id;

    iofst = (int32_t)(offset & 0x7FFFFFFF);

    dbg_printf("esp32_Read: 1r %s %d %lld[%d] \n", file->name, amount, offset, iofst);

    ofst = lfs_file_seek(&lfs_filesystem, &lfs_file, iofst, LFS_SEEK_SET);

    if(ofst == iofst){
        dbg_printf("[SQLite3]File position set ok, pos %d\n", ofst);
    }else  {
        if (iofst != 0 ) {
            dbg_printf("esp32_Read: 2r %d != %d FAIL\n", ofst, iofst);
            return SQLITE_IOERR_SHORT_READ /* SQLITE_IOERR_SEEK */;
        }
    }

    dbg_printf("[SQLite3]Current offset %d, required amount %d\n", ofst, amount);

    lfs_size_t read_size = lfs_file_read(&lfs_filesystem, &lfs_file, buffer, amount);
    dbg_printf("[SQLite3]Readed %u bytes\n", read_size );
    nRead = read_size;

    if ( (int)read_size == amount ) {
        dbg_printf("esp32_Read: 3r %s %u %d OK\n", file->name, nRead, amount);
        return SQLITE_OK;
    } else if ( read_size >= 0 ) {
        dbg_printf("esp32_Read: 3r %s %u %d FAIL\n", file->name, nRead, amount);
        return SQLITE_IOERR_SHORT_READ;
    }

    dbg_printf("esp32_Read: 4r %s FAIL\n", file->name);
    return SQLITE_IOERR_READ;
}

int esp32_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
    size_t nWrite;
    int32_t ofst, iofst;
    esp32_file *file = (esp32_file*) id;

    iofst = (int32_t)(offset & 0x7FFFFFFF);

    dbg_printf("esp32_Write: 1w %s %d %lld[%d] \n", file->name, amount, offset, iofst);

    ofst = lfs_file_seek(&lfs_filesystem,file->fd, iofst, LFS_SEEK_SET);
    if (ofst != 0) {
        return SQLITE_IOERR_SEEK;
    }

    nWrite = lfs_file_write(&lfs_filesystem, file->fd, buffer, amount);
    if ( nWrite != amount ) {
        dbg_printf("esp32_Write: 2w %s %u %d\n", file->name, nWrite, amount);
        return SQLITE_IOERR_WRITE;
    }

    dbg_printf("esp32_Write: 3w %s OK\n", file->name);
    return SQLITE_OK;
}

int esp32_Truncate(sqlite3_file *id, sqlite3_int64 bytes)
{
    esp32_file *file = (esp32_file*) id;
    //int fno = fileno(file->fd);
    //if (fno == -1)
    //	return SQLITE_IOERR_TRUNCATE;
    //if (ftruncate(fno, 0))
    //	return SQLITE_IOERR_TRUNCATE;

    dbg_printf("esp32_Truncate:\n");
    return SQLITE_OK;
}

int esp32_Delete( sqlite3_vfs * vfs, const char * path, int syncDir )
{
    int32_t rc = lfs_remove(&lfs_filesystem, path);
    dbg_printf("esp32_Delete: %s OK\n", path);
    return SQLITE_OK;
}

int esp32_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
    esp32_file *file = (esp32_file*) id;
    lfs_soff_t filesize = lfs_file_size(&lfs_filesystem, &lfs_file);

    dbg_printf("[SQLite3]Get size of File: %s, size %d: ", file->name, filesize);
    if(filesize < 0)
        return SQLITE_IOERR_FSTAT;

    *size = filesize;
    dbg_printf("size: %d]\n", *size);
    return SQLITE_OK;
}

int esp32_Sync(sqlite3_file *id, int flags)
{
    esp32_file *file = (esp32_file*) id;
    int rc = lfs_file_sync(&lfs_filesystem, file->fd);
    dbg_printf("esp32_Sync( %s: ): %d \n",file->name, rc);
    return rc ? SQLITE_IOERR_FSYNC : SQLITE_OK;
}

/**
 *
 * @param vfs
 * @param path
 * @param flags
 * @param result
 * @return
 */
int esp32_Access( sqlite3_vfs * vfs, const char * path, int flags, int * result )
{
    int rc = 0;
    struct lfs_info st;
    memset(&st, 0, sizeof(struct lfs_info));
    rc = lfs_stat(&lfs_filesystem, path, &st);
    *result = ( rc != -1 );
    return SQLITE_OK;
}

int esp32_FullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath )
{
    //structure stat does not have name.
    //struct stat st;
    //int32_t rc = stat( path, &st );
    //if ( rc == 0 ){
    //	strncpy( fullpath, st.name, len );
    //} else {
    //	strncpy( fullpath, path, len );
    //}

    // As now just copy the path
    strncpy( fullpath, path, len );
    fullpath[ len - 1 ] = '\0';

    dbg_printf("esp32_FullPathname: %s\n", fullpath);
    return SQLITE_OK;
}

int esp32_Lock(sqlite3_file *id, int lock_type)
{
    esp32_file *file = (esp32_file*) id;

    dbg_printf("esp32_Lock:Not locked\n");
    return SQLITE_OK;
}

int esp32_Unlock(sqlite3_file *id, int lock_type)
{
    esp32_file *file = (esp32_file*) id;

    dbg_printf("esp32_Unlock:\n");
    return SQLITE_OK;
}

int esp32_CheckReservedLock(sqlite3_file *id, int *result)
{
    esp32_file *file = (esp32_file*) id;

    *result = 0;

    dbg_printf("esp32_CheckReservedLock:\n");
    return SQLITE_OK;
}

int esp32_FileControl(sqlite3_file *id, int op, void *arg)
{
    esp32_file *file = (esp32_file*) id;

    dbg_printf("esp32_FileControl:\n");
    return SQLITE_OK;
}

int esp32_SectorSize(sqlite3_file *id)
{
    esp32_file *file = (esp32_file*) id;

    dbg_printf("esp32_SectorSize:\n");
    return 512;
}

int esp32_DeviceCharacteristics(sqlite3_file *id)
{
    esp32_file *file = (esp32_file*) id;

    dbg_printf("esp32_DeviceCharacteristics:\n");
    return 0;
}

void * esp32_DlOpen( sqlite3_vfs * vfs, const char * path )
{
    dbg_printf("esp32_DlOpen:\n");
    return NULL;
}

void esp32_DlError( sqlite3_vfs * vfs, int len, char * errmsg )
{
    dbg_printf("esp32_DlError:\n");
    return;
}

void ( * esp32_DlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void )
{
    dbg_printf("esp32_DlSym:\n");
    return NULL;
}

void esp32_DlClose( sqlite3_vfs * vfs, void * handle )
{
    dbg_printf("esp32_DlClose:\n");
    return;
}

int esp32_Randomness( sqlite3_vfs * vfs, int len, char * buffer )
{
    long rdm;
    int sz = 1 + (len / sizeof(long));
    char a_rdm[sz * sizeof(long)];
    while (sz--) {
        rdm = esp_random();
        memcpy(a_rdm + sz * sizeof(long), &rdm, sizeof(long));
    }
    memcpy(buffer, a_rdm, len);
    dbg_printf("esp32_Randomness\n");
    return SQLITE_OK;
}

int esp32_Sleep( sqlite3_vfs * vfs, int microseconds )
{
    ets_delay_us(microseconds);
    dbg_printf("esp32_Sleep:\n");
    return SQLITE_OK;
}

int esp32_CurrentTime( sqlite3_vfs * vfs, double * result )
{
    time_t t = time(NULL);
    *result = t / 86400.0 + 2440587.5;
    // This is stubbed out until we have a working RTCTIME solution;
    // as it stood, this would always have returned the UNIX epoch.
    //*result = 2440587.5;
    dbg_printf("esp32_CurrentTime: %g\n", *result);
    return SQLITE_OK;
}

static void shox96_0_2c(sqlite3_context *context, int argc, sqlite3_value **argv) {
    int nIn, nOut;
    long int nOut2;
    const unsigned char *inBuf;
    unsigned char *outBuf;
    unsigned char vInt[9];
    int vIntLen;

    assert( argc==1 );
    nIn = sqlite3_value_bytes(argv[0]);
    inBuf = (unsigned char *) sqlite3_value_blob(argv[0]);
    nOut = 13 + nIn + (nIn+999)/1000;
    vIntLen = encode_unsigned_varint(vInt, (uint64_t) nIn);

    outBuf = (unsigned char *) malloc( nOut+vIntLen );
    memcpy(outBuf, vInt, vIntLen);
    nOut2 = shox96_0_2_compress((const char *) inBuf, nIn, (char *) &outBuf[vIntLen], NULL);
    sqlite3_result_blob(context, outBuf, nOut2+vIntLen, free);
}

static void shox96_0_2d(sqlite3_context *context, int argc, sqlite3_value **argv) {
    unsigned int nIn, nOut, rc;
    const unsigned char *inBuf;
    unsigned char *outBuf;
    long int nOut2;
    uint64_t inBufLen64;
    int vIntLen;

    assert( argc==1 );

    if (sqlite3_value_type(argv[0]) != SQLITE_BLOB)
        return;

    nIn = sqlite3_value_bytes(argv[0]);
    if (nIn < 2){
        return;
    }
    inBuf = (unsigned char *) sqlite3_value_blob(argv[0]);
    inBufLen64 = decode_unsigned_varint(inBuf, &vIntLen);
    nOut = (unsigned int) inBufLen64;
    outBuf = (unsigned char *) malloc( nOut );
    //nOut2 = (long int)nOut;
    nOut2 = shox96_0_2_decompress((const char *) (inBuf + vIntLen), nIn - vIntLen, (char *) outBuf, NULL);
    //if( rc!=Z_OK ){
    //  free(outBuf);
    //}else{
    sqlite3_result_blob(context, outBuf, nOut2, free);
    //}
}

int registerShox96_0_2(sqlite3 *db, const char **pzErrMsg, const struct sqlite3_api_routines *pThunk) {
    sqlite3_create_function(db, "shox96_0_2c", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0, shox96_0_2c, 0, 0);
    sqlite3_create_function(db, "shox96_0_2d", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0, shox96_0_2d, 0, 0);
    return SQLITE_OK;
}

int sqlite3_os_init(void){
    sqlite3_vfs_register(&esp32Vfs, 1);
    sqlite3_auto_extension((void (*)())registerShox96_0_2);
    return SQLITE_OK;
}

int sqlite3_os_end(void){
    return SQLITE_OK;
}