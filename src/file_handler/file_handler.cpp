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

bool traversePath(const char path[]) {
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

        assertDo(!next_file.open(&file, next, O_RDONLY), LL_ERROR, LM_FH, "Can't open next file", return false;);
        assertDo(!file.close(), LL_ERROR, LM_FH, "Can't close dir file", return false;);

        file = next_file;
        return traversePath(&path[i+1]);
    } else {
        log(LL_DEBUG, LM_FH, "Open file");
        log(LL_DEBUG, LM_FH, path);

        assertDo(!next_file.open(&file, path, O_RDWR), LL_ERROR, LM_FH, "Can't open next file", return false;);
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
        return traversePath(path);
    } else if(card == 2) {
        assertDo(!fs2_ready, LL_WARNING, LM_FH, "Can't open file on SD-card 2. FS not ready", return false;);
        file = root2;
        return traversePath(path);
    } else {
        assertCnt(true, LL_ERROR, LM_FH, "Invalid SD-card number");
        return false;
    }
}

void fh_fclose() {
    log(LL_DEBUG, LM_FH, "fh_fclose");
    file.close();
}

void insert(const char insert_buffer[], uint32_t insert_len) {
    log(LL_DEBUG, LM_FH, "insert");

    char buffer[512];
    
    assertRtn(!file.isFile() || !file.isOpen(), LL_ERROR, LM_FH, "File not valid (isFile && opened)");               // file must be valid

    uint32_t insertSeek = file.curPosition();

    file.seekEnd();
    uint32_t newSeek = file.curPosition();
    uint32_t max_len;

    log(LL_DEBUG, LM_FH, "InsertSeek:", insertSeek);
    log(LL_DEBUG, LM_FH, "EndSeek:", newSeek);

    // Copy full buffers
    while(newSeek > insertSeek) {
        
        if(newSeek < 512 || newSeek-512 < insertSeek)
        {
            max_len = newSeek - insertSeek;
            newSeek = insertSeek;
        } else {
            max_len = 512;
            newSeek -= 512;
        }

        log(LL_DEBUG, LM_FH, "Copy from:", newSeek);

        file.seekSet(newSeek);
        uint32_t copy_len = file.read(buffer, max_len);
        file.seekCur(-copy_len+insert_len);

        log(LL_DEBUG, LM_FH, "Copy to:", file.curPosition());
        log(LL_DEBUG, LM_FH, "N Bytes:", copy_len);
        file.write(buffer, copy_len);
        file.sync();
    }
    log(LL_DEBUG, LM_FH, "Everything moved to make place for insert");

    file.seekSet(insertSeek);
    file.write(insert_buffer, insert_len);
    file.sync();
    log(LL_DEBUG, LM_FH, "Insert done");
}

bool csv_locate_cell(uint32_t col, uint32_t row) {
    log(LL_DEBUG, LM_FH, "csv_locate_cell");

    assertDo(!file.isFile() || !file.isOpen(), LL_ERROR, LM_FH, "File not valid (isFile && opened)", return false;);
    assertDo(!file.seekSet(0), LL_ERROR, LM_FH, "File not seekable", return false;);

    char buffer[64];    
    uint32_t len = 0;

    // Goto the wanted row
    uint32_t r = 0;
    while(r < row) {                        // repeat as long as row wasn't found and 
        len = file.read(buffer, 64);        // fill the buffer
        if(len == 0)                        // if file end reached stop searching
            break;

        for(uint32_t i = 0; i < len; i++) { // check for all chars in the buffer
            if(buffer[i] == '\n') {         // whether it's a newline character
                r++;                        // then increase the row counter
                if(r == row) {              // if we found the wanted row
                    file.seekCur(i-len+1);  // correct seek pointer to be after the newline character
                    break;                  // and break from intra-buffer search
                }
            }
        }
    }            

    while(r < row) {
        file.write('\n');                  // add rows if not enough rows in the file
        r++;
    }
    file.sync();

    log(LL_DEBUG, LM_FH, "Row found");

    // Goto the wanted column, we can assume that the file seek pointer is set to the first char in the row/line
    uint32_t c = 0;
    while(c < col) {                        // repeat as long as column wasn't found or added
        len = file.read(buffer, 64);        // fill the buffer

        if(len == 0)
            break;

        for(uint32_t i = 0; i < len; i++) { // check for all chars in the buffer
            if(buffer[i] == ';') {          // whether it's a seperator character
                c++;                        // then increase the column counter
                if(c == col) {              // if we found the wanted column
                    file.seekCur(i-len+1);  // correct seek pointer after the seperator character
                    log(LL_DEBUG, LM_FH, "Column found");
                    return true;            // and return
                }
            } else if(buffer[i] == '\r' || buffer[i] == '\n') {  // we reached the end of the line/row and have to add more columns
                file.seekCur(i-len);        // correct seek pointer to be before the newline character
                goto fill_columns;
            }
        }
    }            

fill_columns:
    log(LL_DEBUG, LM_FH, "Have to add cols");
    char* moreCols = new char[col-c];
    for(uint32_t j = 0; j < (col-c); j++)
        moreCols[j] = ';';
    insert(moreCols, col-c); // Insert further seperator characters
    delete(moreCols);
    log(LL_DEBUG, LM_FH, "Missing Columns added");
    return true;    
}

int32_t csv_read_string(uint32_t col, uint32_t row, uint32_t maxLength, char str[]) {
    log(LL_DEBUG, LM_FH, "csv_read_string");

    assertDo(!csv_locate_cell(col, row), LL_ERROR, LM_FH, "Can't locate cell.", return -1;);

    int32_t length = 0;
    
    assertDo((length = file.read(str, maxLength-1)) <= 0, LL_WARNING, LM_FH, "File seems empty", return -1;);

    str[length-1] = 0;
    for(int32_t i = 0; i < length; i++) {
        if(str[i] == ';' || str[i] == '\r' || str[i] == '\n') {
            str[i] = 0;
            length = i;
            break;
        }
    }

    log(LL_DEBUG, LM_FH, "Read string at col:", col);
    log(LL_DEBUG, LM_FH, "Read string at row:", row);
    log(LL_DEBUG, LM_FH, "Read string:", str);
    log(LL_DEBUG, LM_FH, "String len:", (uint32_t) length);
    return length;
}

void csv_write_string(uint32_t col, uint32_t row, uint32_t length, const char str[]) {
    log(LL_DEBUG, LM_FH, "csv_write_string");

    char buffer[32];
    uint32_t len = 0;
    uint32_t previousLen = 0;
    bool endFound = false;

    assertRtn(!csv_locate_cell(col, row), LL_ERROR, LM_FH, "Can't locate cell.");
    uint32_t startSeek = file.curPosition();

    do {
        len = file.read(buffer, 32);
        for(uint32_t i = 0; i < len; i++, previousLen++) {
            if(buffer[i] == ';' || buffer[i] == '\r' || buffer[i] == '\n') {
                endFound = true;
                break;
            }
        }
    } while(!endFound && len > 0);

    file.seekSet(startSeek);
    if(previousLen >= length)
    {
        file.write(str, length);
        for(;length < previousLen; length++)
            file.write(" ");
    } else {
        file.write(str, previousLen);
        insert(str+previousLen, length-previousLen);
    }
    file.sync();
    log(LL_DEBUG, LM_FH, "String was written to file");
}

uint32_t csv_read_uint32(uint32_t col, uint32_t row) {
    log(LL_DEBUG, LM_FH, "csv_read_uint32");
    uint32_t value = 0;
    char buf[16];

    if(csv_read_string(col, row, 16, buf) > 0) {
        value = atoi(buf);
        log(LL_DEBUG, LM_FH, "Read numeric value:", value);
    }
    else
        assertCnt(true, LL_WARNING, LM_FH, "Cell did not contain a numeric value");

    return value;
}

void csv_write_uint32(uint32_t col, uint32_t row, uint32_t value) {
    log(LL_DEBUG, LM_FH, "csv_write_uint32");

    char str[16];
    int32_t length = sprintf(str, "%lu", value);
    
    csv_write_string(col, row, length, str);
}

float csv_read_float32(uint32_t col, uint32_t row) {
    log(LL_DEBUG, LM_FH, "csv_read_float32");

    float value = 0.0;
    char *komma = 0;
    char buf[32];

    if(csv_read_string(col, row, 32, buf) > 0) {
        if((komma = strchr(buf, ',')) > 0)
            *komma = '.';
        value = (float) atof(buf);
    } else {
        assertDo(true, LL_WARNING, LM_FH, "Can't read cell", return NAN;);
    }        

    log(LL_DEBUG, LM_FH, "Read float value:", value);
    return value;
}

void csv_write_float32(uint32_t col, uint32_t row, float value) {
    log(LL_DEBUG, LM_FH, "csv_write_float32");

    char str[32];
    int32_t length = sprintf(str, "%f", (double) value);
    
    csv_write_string(col, row, length, str);
}

cPrice csv_read_price(uint32_t col, uint32_t row) {
    log(LL_DEBUG, LM_FH, "csv_read_price");

    cPrice price;
    int32_t euros = 0;
    uint8_t cents = 0;
    char *komma = 0;
    char buf[32];

    if(csv_read_string(col, row, 32, buf) > 0) {
        if((komma = strchr(buf, ',')) > 0) {
            cents = atoi(komma+1);
        }
        euros = atoi(buf);
    } else {
        assertDo(true, LL_WARNING, LM_FH, "Can't read cell", price.SetValid(false); return price;);
    }        

    assertDo(cents > 99, LL_ERROR, LM_FH, "Invalid cent format. More than 99 cents", price.SetValid(false); return price;);
    price.Set(euros < 0, euros, cents);

    log(LL_DEBUG, LM_FH, "Read price:", price);
    return price;
}

void csv_write_price(uint32_t col, uint32_t row, const cPrice &price) {
    log(LL_DEBUG, LM_FH, "csv_write_price");

    char str[32];
    int32_t length = sprintf(str, "%d,%02hhu", price.GetEuros(), price.GetCents());
    
    csv_write_string(col, row, length, str);
}

int32_t csv_findInColumn(uint32_t col, const char str[]) {
    log(LL_DEBUG, LM_FH, "csv_findInColumn");

    assertDo(!file.isFile() || !file.isOpen(), LL_ERROR, LM_FH, "File not valid (isFile && opened)", return -1;);
    assertDo(!file.seekSet(0), LL_ERROR, LM_FH, "File not seekable", return -1;);
    assertDo(strlen(str)>31, LL_ERROR, LM_FH, "Input string too long", return -1;);

    char buffer[64];    
    uint32_t len = 0;

    // Go row by row
    uint32_t r = 0;
    uint32_t c = 0;
    bool afterSeperator = true;
    while(true) {                           // repeat as long as row wasn't found and 
        len = file.read(buffer, 64);        // fill the buffer
        log_hexdump(LL_DEBUG, LM_FH, "Read-Data:", len, (const uint8_t*) buffer);

        if(len == 0)                        // if file end reached stop searching
        {
            log(LL_DEBUG, LM_FH, "EoF reached. Nothing found");
            return -1;                      // we have reached the end of file and have not found anything
        }
            
        for(uint32_t i = 0; i < len; i++) { // check for all chars in the buffer
            if(buffer[i] == ';') {          // whether it's a seperator character
                c++;                        // then increase the column counter
                afterSeperator = true;
                i++;
            } else if(buffer[i] == '\n') {  // we reached the end of the line/row and have to add more columns
                log(LL_DEBUG, LM_FH, "New line. Increase row");
                r++;
                c = 0;
                afterSeperator = true;
                i++;
            }

            if(afterSeperator && c == col) {              // if we found the wanted column
                log(LL_DEBUG, LM_FH, "In wanted column");
                file.seekCur(i-len);  // correct seek pointer after the seperator character
                len = file.read(buffer, 64);
                i = 0;

                log_hexdump(LL_DEBUG, LM_FH, "Compare:", len, (const uint8_t*) buffer);
                log_hexdump(LL_DEBUG, LM_FH, "With:   ", strlen(str), (const uint8_t*) str);
                bool equal = true;
                for(uint32_t k = 0; k < len; k++) {
                    if(str[k] == 0) {
                        if(buffer[k] != ';')
                            equal = false;
                        break;
                    } else if(buffer[k] == ';' || buffer[k] != str[k]) {
                        equal = false;
                        break;
                    }
                }
                if(equal) 
                    return r;          
            }
            afterSeperator = false;
        }
    }       
}