#include "file_handler.h"
#include <SD.h>
#include <SPI.h>
#include "../util/error.h"

#define SDCARD1_CS BUILTIN_SDCARD
#define SDCARD2_CS 15

Sd2Card card1, card2;
SdVolume volume1, volume2;
SdFile root1, root2;
bool fs1_ready = false;
bool fs2_ready = false;

SdFile file;

void fh_init() {
    log(LL_DEBUG, LM_FH, "fh_init");

    fs1_ready = false;
    fs2_ready = false;

    assertCnt(!card1.init(SPI_HALF_SPEED, SDCARD1_CS), LL_ERROR, LM_FH, "Initialisation of SD-card 1 failed") 
    else {
        assertCnt(!volume1.init(card1), LL_ERROR, LM_FH, "Could not find FAT16/FAT32 partition on SD-card 1")
        else {
            root1.openRoot(volume1);
            fs1_ready = true;
            if(checkLogLevel(LM_FH, LL_DEBUG)) {
                log(LL_DEBUG, LM_FH, "Root-Directory of SD-card 1:");
                root1.ls(LS_R | LS_DATE | LS_SIZE);
            }
        }
    }

    assertCnt(!card2.init(SPI_HALF_SPEED, SDCARD2_CS), LL_ERROR, LM_FH, "Initialisation of SD-card 2 failed") 
    else {
        assertCnt(!volume2.init(card2), LL_ERROR, LM_FH, "Could not find FAT16/FAT32 partition on SD-card 2") 
        else {
            root2.openRoot(volume2);
            fs2_ready = true;
            if(checkLogLevel(LM_FH, LL_DEBUG)) {
                log(LL_DEBUG, LM_FH, "Root-Directory of SD-card 2:");
                root2.ls(LS_R | LS_DATE | LS_SIZE);
            }
        }
    }
}

bool traversePath(const char path[], bool dirExpected) {
    log(LL_DEBUG, LM_FH, "traversePath");

    char next[16];
    uint8_t i;
    SdFile next_file;

    assertDo(!file.isDir(), LL_WARNING, LM_FH, "This is not a directory. Can't traverse path", return false;);

    for(i = 0; i < 16; i++) {
        if(path[i] == '/' || path[i] == 0)
            break;
    }
    assertDo(i >= 16, LL_ERROR, LM_FH, "Invalid Path. Can't find / char", return false;);

    if(path[i] == '/') {
        memcpy(next, path, i);
        next[i] = 0;

        log(LL_DEBUG, LM_FH, "Traverse to next dir");
        log(LL_DEBUG, LM_FH, next);

        assertDo(!next_file.open(&file, next, O_RDONLY), LL_ERROR, LM_FH, "Can't open next dir file", return false;);
        assertDo(!file.close(), LL_ERROR, LM_FH, "Can't close dir file", return false;);

        file = next_file;
        return traversePath(&path[i+1], dirExpected);
    } else {
        log(LL_DEBUG, LM_FH, "Open ", path);

        if(dirExpected) {
            log(LL_DEBUG, LM_FH, "As Directoy");
            assertDo(!next_file.open(&file, path, O_RDONLY), LL_ERROR, LM_FH, "Can't open directory", return false;);
        } else {
            log(LL_DEBUG, LM_FH, "As File");
            assertDo(!next_file.open(&file, path, O_RDWR | O_CREAT), LL_ERROR, LM_FH, "Can't open file", return false;);
        }
        assertDo(!file.close(), LL_ERROR, LM_FH, "Can't close dir file", return false;);

        file = next_file;
        return true;
    }
}

bool fh_fopen(uint8_t card, const char path[]) {
    log(LL_DEBUG, LM_FH, "fh_fopen");
    file.close();

    if(card == 1) {
        assertDo(!fs1_ready, LL_WARNING, LM_FH, "Can't open file on SD-card 1. FS not ready", return false;);
        file = root1;
        return traversePath(path, false);
    } else if(card == 2) {
        assertDo(!fs2_ready, LL_WARNING, LM_FH, "Can't open file on SD-card 2. FS not ready", return false;);
        file = root2;
        return traversePath(path, false);
    } else {
        assertCnt(true, LL_ERROR, LM_FH, "Invalid SD-card number");
        return false;
    }
}

void fh_fclose() {
    log(LL_DEBUG, LM_FH, "fh_fclose");
    file.close();
}

bool fh_fs_ready(uint8_t card) {
    log(LL_DEBUG, LM_FH, "fh_fs_ready");

    if(card == 1) 
        return fs1_ready;
    if(card == 2)
        return fs2_ready;
    return false;
}

int32_t fh_fread(uint32_t pos, uint16_t len, uint8_t *buf) {
    log(LL_DEBUG, LM_FH, "fh_fread");

    assertDo(!file.isFile() || !file.isOpen(), LL_ERROR, LM_FH, "File not valid (isFile && opened)", return -1;);               // file must be valid

    assertDo(file.seekSet(pos) == 0, LL_ERROR, LM_FH, "Can't set file seek", return -1;);

    return file.read(buf, len);
}

int32_t fh_fwrite(uint32_t pos, uint16_t len, uint8_t *buf) {
    log(LL_DEBUG, LM_FH, "fh_fwrite");

    assertDo(!file.isFile() || !file.isOpen(), LL_ERROR, LM_FH, "File not valid (isFile && opened)", return -1;);               // file must be valid

    assertDo(file.seekSet(pos) == 0, LL_ERROR, LM_FH, "Can't set file seek", return -1;);

    int write_len = file.write(buf, len);
    assertDo(write_len < 0, LL_ERROR, LM_FH, "Can't write to file", return -1;);

    assertDo(file.sync() == 0, LL_ERROR, LM_FH, "Can't sync data to sd card", return -1;);

    return write_len;
}

int32_t fh_fappend(uint16_t len, uint8_t *buf) {
    log(LL_DEBUG, LM_FH, "fh_fappend");
    return fh_fwrite(file.fileSize(), len, buf);    
}

int32_t fh_flen() {
    log(LL_DEBUG, LM_FH, "fh_flen");
    
    return file.fileSize();
}

void fh_flog(uint32_t pos) {
    uint8_t buf[256];
    assertRtn(!fh_fread(pos, 256, buf), LL_ERROR, LM_FH, "Can't read from file");
    log(LL_VERBOSE, LM_FH, "File-Seek-At:", pos);
    log(LL_VERBOSE, LM_FH, "File-Len    :", file.fileSize());
    log_hexdump(LL_VERBOSE, LM_FH, "File-Content:", 256, buf);
}

void fh_clear() {
    log(LL_DEBUG, LM_FH, "fh_clear");

    file.truncate(0);
}

bool fh_mkdir(uint8_t card, const char path[], const char name[]) {
    log(LL_DEBUG, LM_FH, "fh_mkdir(..): ", path);
    SdFile newDir;

    file.close();

    if(card == 1) {
        assertDo(!fs1_ready, LL_WARNING, LM_FH, "Can't open file on SD-card 1. FS not ready", return false;);
        file = root1;
        assertDo(!traversePath(path, true), LL_WARNING, LM_FH, "Can't find path on SD-card 1", return false;);
    } else if(card == 2) {
        assertDo(!fs2_ready, LL_WARNING, LM_FH, "Can't open file on SD-card 2. FS not ready", return false;);
        file = root2;
        assertDo(!traversePath(path, true), LL_WARNING, LM_FH, "Can't find path on SD-card 2", return false;);
    } else {
        assertCnt(true, LL_ERROR, LM_FH, "Invalid SD-card number");
        return false;
    }
    
    assertDo(file.isFile(), LL_WARNING, LM_FH, "The path does not specify a directory but a file", return false;);
    
    if(newDir.makeDir(&file, name) > 0) {
        log(LL_INFO, LM_FH, "New subdir created at: ", path);
        log(LL_INFO, LM_FH, "Sub-Dir Name: ", name);
    }
    else
        log(LL_WARNING, LM_FH, "Subdir already exists. Data could be overwritten. Dir-Name: ", name);

    return true;
}