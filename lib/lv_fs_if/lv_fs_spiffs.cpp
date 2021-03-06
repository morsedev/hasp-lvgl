/**
 * @file lv_fs_spiffs.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
//#include <Arduino.h>
#include "lv_fs_if.h"
#include "lv_fs_spiffs.h"

#if LV_USE_FS_IF
#if LV_FS_IF_SPIFFS != '\0'

#if defined(ARDUINO_ARCH_ESP32)
#include "SPIFFS.h"
#endif
#include "FS.h" // Include the SPIFFS library

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/* Create a type to store the required data about your file.*/
typedef File lv_spiffs_file_t;

/*Similarly to `file_t` create a type for directory reading too */
typedef File lv_spiffs_dir_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static lv_fs_res_t fs_open(lv_fs_drv_t * drv, void * file_p, const char * path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos);
static lv_fs_res_t fs_size(lv_fs_drv_t * drv, void * file_p, uint32_t * size_p);
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);
static lv_fs_res_t fs_remove(lv_fs_drv_t * drv, const char * path);
static lv_fs_res_t fs_trunc(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_rename(lv_fs_drv_t * drv, const char * oldname, const char * newname);
static lv_fs_res_t fs_free(lv_fs_drv_t * drv, uint32_t * total_p, uint32_t * free_p);
static lv_fs_res_t fs_dir_open(lv_fs_drv_t * drv, void * dir_p, const char * path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * dir_p, char * fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * dir_p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_fs_if_spiffs_init(void)
{
    /*----------------------------------------------------
     * Initialize your storage device and File System
     * -------------------------------------------------*/
    fs_init();

    /*---------------------------------------------------
     * Register the file system interface  in LittlevGL
     *--------------------------------------------------*/

    /* Add a simple drive to open images */
    lv_fs_drv_t fs_drv; /*A driver descriptor*/
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.file_size     = sizeof(lv_spiffs_file_t);
    fs_drv.letter        = LV_FS_IF_SPIFFS;
    fs_drv.open_cb       = fs_open;
    fs_drv.close_cb      = fs_close;
    fs_drv.read_cb       = fs_read;
    fs_drv.write_cb      = fs_write;
    fs_drv.seek_cb       = fs_seek;
    fs_drv.tell_cb       = fs_tell;
    fs_drv.free_space_cb = fs_free;
    fs_drv.size_cb       = fs_size;
    fs_drv.remove_cb     = fs_remove;
    fs_drv.rename_cb     = fs_rename;
    fs_drv.trunc_cb      = fs_trunc;

    fs_drv.rddir_size   = sizeof(lv_spiffs_dir_t);
    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb  = fs_dir_open;
    fs_drv.dir_read_cb  = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Initialize your Storage device and File system. */
static void fs_init(void)
{
    SPIFFS.begin();
}

/**
 * Open a file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable
 * @param path path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_open(lv_fs_drv_t * drv, void * file_p, const char * path, lv_fs_mode_t mode)
{
    Serial.print("Opening: ");
    Serial.println(path);

    String fullpath = "/" + String(path);
    Serial.println(fullpath);

    const char * fullfilename = fullpath.c_str();
    Serial.println(fullfilename);
    Serial.flush();

    if(!SPIFFS.exists(fullfilename)) {
        Serial.println("File does not exist");
        return LV_FS_RES_NOT_EX;
    }
    // return LV_FS_RES_UNKNOWN;

    static lv_spiffs_file_t file;
    if(mode == LV_FS_MODE_WR)
        file = SPIFFS.open(fullfilename, "a");
    else if(mode == LV_FS_MODE_RD)
        file = SPIFFS.open(fullfilename, "r");
    else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
        file = SPIFFS.open(fullfilename, "r+");

    if(file) {

        lv_spiffs_file_t * fp = (lv_spiffs_file_t *)file_p; /*Just avoid the confusing casings*/
        *fp                   = file;

        Serial.println("Opened OK");
        Serial.flush();
        // return LV_FS_RES_UNKNOWN;

        return LV_FS_RES_OK;
    } else {
        Serial.println("Open failed");
        return LV_FS_RES_UNKNOWN;
    }
}

/**
 * Close an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable. (opened with lv_ufs_open)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p)
{
    // File file = *(File *)(file_p);
    lv_spiffs_file_t * file = (lv_spiffs_file_t *)file_p; /*Convert type*/

    char * msg = nullptr;
    sprintf(msg, "Closing: %s", file->name());
    Serial.println(msg);
    Serial.flush();
    file->close();

    return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable.
 * @param buf pointer to a memory block where to store the read data
 * @param btr number of Bytes To Read
 * @param br the real number of read bytes (Byte Read)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    lv_spiffs_file_t * file = (lv_spiffs_file_t *)file_p; /*Convert type*/
                                                          // lv_spiffs_file_t file = *(lv_spiffs_file_t *)(file_p);

    char * msg = nullptr;
    sprintf(msg, "Reading: %s", file->name());
    Serial.println(msg);
    Serial.flush();

    if(!*file || file->isDirectory()) {
        return LV_FS_RES_UNKNOWN;
    }

    return LV_FS_RES_UNKNOWN;
    *br = file->readBytes((char *)buf, btr);
    return LV_FS_RES_OK;
}

/**
 * Write into a file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable
 * @param buf pointer to a buffer with the bytes to write
 * @param btr Bytes To Write
 * @param br the number of real written bytes (Bytes Written). NULL if unused.
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    return LV_FS_RES_NOT_IMP;
    /*
        File file = SPIFFS.open((char *)file_p, "w");
        if(!file) {
            return LV_FS_RES_UNKNOWN;
        }
        char * message;
        strncpy(buf, message, btw);
        if(file.print(message)) {
            *bw = (uint32_t)sizeof(message);
            return LV_FS_RES_OK;
        } else {
            bw = 0;
            return LV_FS_RES_UNKNOWN;
        }*/
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable. (opened with lv_ufs_open )
 * @param pos the new position of read write pointer
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos)
{
    lv_spiffs_file_t * file = (lv_spiffs_file_t *)file_p; /*Convert type*/
                                                          // lv_spiffs_file_t file = *(lv_spiffs_file_t *)(file_p);
    char * msg = nullptr;
    sprintf(msg, "Seeking: %s", file->name());
    Serial.println(msg);
    Serial.flush();

    if(file->seek(pos, SeekSet)) {
        return LV_FS_RES_OK;
    } else {
        return LV_FS_RES_UNKNOWN;
    }
}

/**
 * Give the size of a file bytes
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable
 * @param size pointer to a variable to store the size
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_size(lv_fs_drv_t * drv, void * file_p, uint32_t * size_p)
{
    lv_spiffs_file_t * file = (lv_spiffs_file_t *)file_p; /*Convert type*/
                                                          // lv_spiffs_file_t file = *(lv_spiffs_file_t *)(file_p);
    char * msg = nullptr;
    sprintf(msg, "Filesize: %s", file->name());
    Serial.println(msg);
    Serial.flush();

    *size_p = file->size();
    return LV_FS_RES_OK;
}

/**
 * Give the position of the read write pointer
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a file_t variable.
 * @param pos_p pointer to to store the result
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
    lv_spiffs_file_t * file = (lv_spiffs_file_t *)file_p; /*Convert type*/
                                                          // lv_spiffs_file_t file = *(lv_spiffs_file_t *)(file_p);
    char * msg = nullptr;
    sprintf(msg, "Telling: %s", file->name());
    Serial.println(msg);
    Serial.flush();

    uint32_t pos = file->position();
    pos_p        = &pos;
    return LV_FS_RES_OK;
}

/**
 * Delete a file
 * @param drv pointer to a driver where this function belongs
 * @param path path of the file to delete
 * @return  LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_remove(lv_fs_drv_t * drv, const char * path)
{
    Serial.println("Deleteing: " + (String)path);
    Serial.flush();

    if(SPIFFS.remove(path)) {
        return LV_FS_RES_OK;
    } else {
        return LV_FS_RES_UNKNOWN;
    }
}

/**
 * Truncate the file size to the current position of the read write pointer
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to an 'ufs_file_t' variable. (opened with lv_fs_open )
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_trunc(lv_fs_drv_t * drv, void * file_p)
{
    return LV_FS_RES_NOT_IMP;
}

/**
 * Rename a file
 * @param drv pointer to a driver where this function belongs
 * @param oldname path to the file
 * @param newname path with the new name
 * @return LV_FS_RES_OK or any error from 'fs_res_t'
 */
static lv_fs_res_t fs_rename(lv_fs_drv_t * drv, const char * oldname, const char * newname)
{
    if(SPIFFS.rename(oldname, newname)) {
        return LV_FS_RES_OK;
    } else {
        return LV_FS_RES_UNKNOWN;
    }
}

/**
 * Get the free and total size of a driver in kB
 * @param drv pointer to a driver where this function belongs
 * @param letter the driver letter
 * @param total_p pointer to store the total size [kB]
 * @param free_p pointer to store the free size [kB]
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_free(lv_fs_drv_t * drv, uint32_t * total_p, uint32_t * free_p)
{
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;

    /* Add your code here*/

    return res;
}

/**
 * Initialize a 'fs_read_dir_t' variable for directory reading
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to a 'fs_read_dir_t' variable
 * @param path path to a directory
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_open(lv_fs_drv_t * drv, void * dir_p, const char * path)
{
    lv_spiffs_dir_t * dir = (lv_spiffs_file_t *)dir_p; /*Convert type*/

    Serial.print("Open directory: ");
    Serial.println(path);
    Serial.flush();

    *dir = SPIFFS.open(path, "r");
    if(*dir) {
        dir_p = &dir;
        return LV_FS_RES_OK;
    } else {
        return LV_FS_RES_UNKNOWN;
    }
}

/**
 * Read the next filename form a directory.
 * The name of the directories will begin with '/'
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to an initialized 'fs_read_dir_t' variable
 * @param fn pointer to a buffer to store the filename
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * dir_p, char * fn)
{
    lv_spiffs_dir_t * dir = (lv_spiffs_file_t *)dir_p; /*Convert type*/
    lv_spiffs_file_t file = dir->openNextFile();

    if(file) {
        strcpy(fn, file.name());
        return LV_FS_RES_OK;
    } else {
        return LV_FS_RES_UNKNOWN;
    }
}

/**
 * Close the directory reading
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to an initialized 'fs_read_dir_t' variable
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * dir_p)
{
    return LV_FS_RES_OK;
}

#endif /*LV_USE_FS_IF*/
#endif /*LV_FS_IF_SPIFFS*/
