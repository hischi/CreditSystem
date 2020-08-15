#include "data_handler.h"
#include "../util/error.h"
#include "../file_handler/file_handler.h"
#include "../util/time_format.h"
#include "../clock/clock.h"

#define MAX_MEMBER_COUNT 512

sMember  members[MAX_MEMBER_COUNT];
uint32_t member_count;

sTransaction transaction;
sTransactionHeader transaction_header;

uint8_t log_idx;
#define MAX_LOG_IDX 32

void dh_load_members() {
    log(LL_DEBUG, LM_DH, "dh_load_members");

    member_count = 0;

    assertRtn(!fh_fopen(1, "DATABASE.DB"), LL_ERROR, LM_DH, "Can't open data-base");
    
    sDataBaseHeader header;
    assertRtn(fh_fread(0, sizeof(header), (uint8_t*) &header) < sizeof(header), LL_ERROR, LM_DH, "Can't read data-base header");

    log(LL_INFO, LM_DH, "Loaded Data-Base with the following information:");
    log(LL_INFO, LM_DH, "   Version:     ", header.version);
    log(LL_INFO, LM_DH, "   Modified:    ", DateTime(header.datetime_modified));
    header.author[sizeof(header.author)-1] = 0;
    log(LL_INFO, LM_DH, "   Author:      ", header.author);
    log(LL_INFO, LM_DH, "   Entry Count: ", header.entry_count);

    member_count = header.entry_count;
    assertDo(member_count > MAX_MEMBER_COUNT, LL_WARNING, LM_DH, "Can't handle that many members. Restrict to max. member count", member_count = MAX_MEMBER_COUNT;);
    
    assertDo(fh_fread(sizeof(header), member_count*sizeof(sMember), (uint8_t*) &members) < member_count*sizeof(sMember), LL_ERROR, LM_DH, "Can't read all members", member_count = 0;);

    log(LL_INFO, LM_DH, "Members loaded successfully.");
    fh_fclose();
}

void dh_init() {
    log(LL_DEBUG, LM_DH, "dh_init");

    dh_load_members();
    if(member_count == 0) {
        assertCnt(true, LL_WARNING, LM_DH, "No members found in data-base. Try again...");
        dh_load_members();
        assertRtn(member_count == 0, LL_ERROR, LM_DH, "No members found in second try.");
    }

    transaction.status = DH_TA_CORRUPTED;

    log_idx = 0;
}

sMember* dh_get_member(uint32_t memberID) {
    log(LL_DEBUG, LM_DH, "dh_get_member");

    for(uint32_t i = 0; i < member_count; i++) {
        if(memberID == members[i].id)
            return &(members[i]);
    }

    assertCnt(true, LL_WARNING, LM_DH, "Member with the given ID not found");
    return 0;
}

bool dh_is_authorised(uint32_t memberID, uint32_t cardID) {
    sMember *member = dh_get_member(memberID);

    if(member != 0 && member->card_id == cardID) {
        log(LL_DEBUG, LM_DH, "Correct card and member id");
        return true;
    } else {
        assertCnt(true, LL_WARNING, LM_DH, "Incorrect card or member id");
        return false;
    }
}





bool dh_read_transaction_header() {
    log(LL_DEBUG, LM_DH, "dh_read_transaction_header");

    assertDo(!fh_fopen(1, "TRANSACT.DB"), LL_ERROR, LM_DH, "Can't open transaction list", return false;);

    log(LL_DEBUG, LM_DH, "Transaction file length", (uint32_t) fh_flen());

    sTransactionHeaderCS header_cs;
    if(fh_flen() <= 0) {
        log(LL_INFO, LM_DH, "Transaction list file is empty. Generate header.");
        header_cs.header.datetime_modified = clock_now().unixtime();
        header_cs.header.version = 0x01;
        header_cs.header.entry_count = 0;
        header_cs.checksum = calculate_checksum((uint16_t*) &header_cs.header, sizeof(header_cs.header));
        fh_fwrite(0, sizeof(header_cs), (uint8_t*) &header_cs);

    } else {
        fh_flog(0);
        assertDo(fh_fread(0, sizeof(header_cs), (uint8_t*) &header_cs) < sizeof(header_cs), LL_ERROR, LM_DH, "Can't read transaction header", return false;);
        uint32_t checksum = calculate_checksum((uint16_t*) &header_cs.header, sizeof(header_cs.header));
        log(LL_DEBUG, LM_DH, "Checksum from file:  ", header_cs.checksum);
        log(LL_DEBUG, LM_DH, "Checksum calculated: ", checksum);
        assertDo(checksum != header_cs.checksum, LL_FATAL, LM_DH, "Checksum of transaction header wrong", return false;);
    }   

    transaction_header = header_cs.header;
    fh_fclose();
    return true;
}

bool dh_write_transaction_header() {
    log(LL_DEBUG, LM_DH, "dh_write_transaction_header");

    assertDo(!fh_fopen(1, "TRANSACT.DB"), LL_ERROR, LM_DH, "Can't open transaction list", return false;);

    sTransactionHeaderCS header_cs;
    header_cs.header = transaction_header;
    header_cs.checksum = calculate_checksum((uint16_t*) &header_cs.header, sizeof(header_cs.header));

    fh_fwrite(0, sizeof(header_cs), (uint8_t*) &header_cs);
    fh_flog(0);

    fh_fclose();
    return true;
}

bool dh_read_last_transaction() {
    log(LL_DEBUG, LM_DH, "dh_read_last_transaction");

    assertDo(!fh_fopen(1, "TRANSACT.DB"), LL_ERROR, LM_DH, "Can't open transaction list", return false;);

    log(LL_DEBUG, LM_DH, "Transaction file length", (uint32_t) fh_flen());

    sTransactionCS transaction_cs;
    fh_flog(sizeof(sTransactionHeaderCS) + (transaction_header.entry_count-1)*sizeof(sTransactionCS));
    fh_fread(sizeof(sTransactionHeaderCS) + (transaction_header.entry_count-1)*sizeof(sTransactionCS), sizeof(sTransactionCS), (uint8_t*) &transaction_cs);
    log_hexdump(LL_DEBUG, LM_DH, "Transaction:", sizeof(sTransactionCS), (uint8_t*) &transaction_cs);
    uint32_t checksum = calculate_checksum((uint16_t*) &transaction_cs.transaction, sizeof(transaction_cs.transaction));
    log(LL_DEBUG, LM_DH, "Checksum from file:  ", transaction_cs.checksum);
    log(LL_DEBUG, LM_DH, "Checksum calculated: ", checksum);
    assertDo(checksum != transaction_cs.checksum, LL_FATAL, LM_DH, "Checksum of transaction header wrong", return false;);
    
    transaction = transaction_cs.transaction;

    fh_fclose();
    return true;
}

bool dh_append_transaction() {
    log(LL_DEBUG, LM_DH, "dh_append_transaction");

    log(LL_INFO, LM_DH, "Try to append a new transaction...");
    assertDo(!fh_fopen(1, "TRANSACT.DB"), LL_ERROR, LM_DH, "Can't open transaction list", return false;);

    sTransactionCS transaction_cs;
    transaction_cs.transaction = transaction;
    transaction_cs.checksum = calculate_checksum((uint16_t*) &transaction_cs.transaction, sizeof(transaction_cs.transaction));
    fh_fappend(sizeof(transaction_cs), (uint8_t*) & transaction_cs);
    fh_flog(0);
    fh_fclose();

    transaction_header.entry_count++;
    transaction_header.datetime_modified = clock_now().unixtime();

    log(LL_INFO, LM_DH, "New transaction appended to SD-card.");
    return dh_write_transaction_header();
}

bool dh_write_last_transaction() {
    log(LL_DEBUG, LM_DH, "dh_write_last_transaction");

    log(LL_INFO, LM_DH, "Try to write / update last transaction...");
    assertDo(!fh_fopen(1, "TRANSACT.DB"), LL_ERROR, LM_DH, "Can't open transaction list", return false;);

    sTransactionCS transaction_cs;
    transaction_cs.transaction = transaction;
    transaction_cs.checksum = calculate_checksum((uint16_t*) &transaction_cs.transaction, sizeof(transaction_cs.transaction));
    fh_fwrite(sizeof(sTransactionHeaderCS) + (transaction_header.entry_count-1)*sizeof(sTransactionCS), sizeof(sTransactionCS), (uint8_t*) &transaction_cs);
    fh_flog(0);
    
    transaction = transaction_cs.transaction;
    
    fh_fclose();

    log(LL_INFO, LM_DH, "Last transaction was written / updated.");
    return true;
}

bool dh_is_available(uint32_t memberID, uint16_t itemID){
    log(LL_DEBUG, LM_DH, "dh_is_available");
    sMember *member = dh_get_member(memberID);

    if(member != 0) {
        uint32_t mask = 1 << (itemID-1);
        log(LL_DEBUG, LM_DH, "Mask:      ", mask);
        log(LL_DEBUG, LM_DH, "Properties:", (uint32_t) member->properties);
        return !(mask&member->properties);
        return true;
    } else {
        assertCnt(true, LL_WARNING, LM_DH, "Incorrect member id");
        return false;
    }
}

uint32_t dh_calculate_discount(uint32_t memberID, uint32_t cost){
    log(LL_DEBUG, LM_DH, "dh_calculate_discount");
    sMember *member = dh_get_member(memberID);

    if(member != 0) {
        uint32_t discount = (member->discount * cost) / 100;
        return discount;
    } else {
        assertCnt(true, LL_WARNING, LM_DH, "Incorrect member id");
        return false;
    }
}

bool dh_create_transaction(uint32_t memberID, uint8_t itemID, uint32_t cost, uint16_t discount) {
    log(LL_DEBUG, LM_DH, "dh_create_transaction");
    log(LL_INFO, LM_DH, "A new transaction was requested.");

    // Create the transaction and append it to the list
    if(transaction.status <= DH_TA_APPROVED) {
        assertCnt(true, LL_ERROR, LM_DH, "There was already a transaction in progress. Can't start a new one.");
        
        if(clock_now().unixtime() - transaction.datetime_modified > 20 ) {
            log(LL_WARNING, LM_DH, "Timeout of unapproved transaction. Mark as such and continue with the new transaction.");
            dh_timeout_transaction();
        } else
            return false;
    }
    
    assertDo(!dh_read_transaction_header(), LL_ERROR, LM_DH, "Can't start new transaction without valid header", return false;);

    if(transaction_header.entry_count == 0) {
        transaction.id = 0;
    } else {
        assertDo(!dh_read_last_transaction(), LL_ERROR, LM_DH, "Can't start new transaction without knowing last transaction", return false;);
        transaction.id++; 
    }

    
    transaction.status = DH_TA_CREATED;
    transaction.member_id = memberID;
    transaction.item_id = itemID;
    transaction.cost = cost;
    transaction.discount = discount;
    transaction.datetime_modified = clock_now().unixtime();
    log(LL_INFO, LM_DH, "New transaction was created but is yet not stored");

    return dh_append_transaction();
}

bool dh_approve_transaction() {
    log(LL_DEBUG, LM_DH, "dh_approve_transaction");
    log(LL_INFO, LM_DH, "Approve the transaction");

    assertDo(!dh_read_last_transaction(), LL_ERROR, LM_DH, "Can't read last transaction", return false;);

    assertDo(transaction.status != DH_TA_CREATED, LL_ERROR, LM_DH, "There is no just created transaction. Out of order", return false;);

    transaction.status |= DH_TA_APPROVED;
    //transaction.datetime_modified = clock_now().unixtime();

    assertDo(!dh_write_last_transaction(), LL_ERROR, LM_DH, "Can't write last transaction", return false;);

    return true;    
}

bool dh_complete_transaction() {
    log(LL_DEBUG, LM_DH, "dh_complete_transaction");
    log(LL_INFO, LM_DH, "Complete the last transaction...");

    assertDo(!dh_read_last_transaction(), LL_ERROR, LM_DH, "Can't read last transaction", return false;);

    assertDo(transaction.status != DH_TA_APPROVED, LL_ERROR, LM_DH, "There is no just approved transaction. Out of order", return false;);

    transaction.status |= DH_TA_COMPLETED;
    //transaction.datetime_modified = clock_now().unixtime();

    assertDo(!dh_write_last_transaction(), LL_ERROR, LM_DH, "Can't write last transaction", return false;);

    return true;    
}

bool dh_cancle_transaction() {
    log(LL_DEBUG, LM_DH, "dh_cancle_transaction");
    log(LL_INFO, LM_DH, "Cancle the last transaction");

    assertDo(!dh_read_last_transaction(), LL_ERROR, LM_DH, "Can't read last transaction", return false;);

    assertDo(transaction.status > DH_TA_APPROVED, LL_ERROR, LM_DH, "There is no new or just approved transaction. Out of order", return false;);

    transaction.status |= DH_TA_CANCLED;
    transaction.datetime_modified = clock_now().unixtime();

    assertDo(!dh_write_last_transaction(), LL_ERROR, LM_DH, "Can't write last transaction", return false;);

    return true;    
}

bool dh_timeout_transaction() {
    log(LL_DEBUG, LM_DH, "dh_timeout_transaction");
    log(LL_INFO, LM_DH, "Cancle the last transaction because of timeout");

    assertDo(!dh_read_last_transaction(), LL_ERROR, LM_DH, "Can't read last transaction", return false;);

    assertDo(transaction.status > DH_TA_APPROVED, LL_ERROR, LM_DH, "There is no new or just approved transaction. Out of order", return false;);

    transaction.status |= DH_TA_TIMEOUT;
    transaction.datetime_modified = clock_now().unixtime();

    assertDo(!dh_write_last_transaction(), LL_ERROR, LM_DH, "Can't write last transaction", return false;);

    return true;    
}

sMember* dh_get_member_from_idx(uint32_t idx) {
    log(LL_DEBUG, LM_DH, "dh_get_member_from_idx");

    if(idx >= member_count)
        return 0;

    return &members[idx];    
}

char log_dir[16];
char log_parent[] = "logs";


bool dh_prepare_log() {
    log(LL_DEBUG, LM_DH, "dh_prepare_log");

    char log_path[64];
    DateTime time = clock_now();
    sprintf(log_dir, "%02hhu%02hhu%02hhu%02hhu", time.month(), time.day(), time.hour(), time.minute());
    sprintf(log_path, "%s/%s", log_parent, log_dir);
    log(LL_INFO, LM_DH, "Logs will be stored here: ", log_path);

    assertDo(!fh_mkdir(1, log_parent, log_dir), LL_ERROR, LM_DH, "Can't make or open directoy", return false;);
    
    return true;
}

bool dh_store_log(char *log_buffer, uint32_t size) {
    log(LL_DEBUG, LM_DH, "dh_store_log");

    char log_name[16];
    char log_path[64];
    DateTime time;

    sprintf(log_name, "LOG%03d.txt", log_idx);
    sprintf(log_path, "%s/%s/%s", log_parent, log_dir, log_name);
    log(LL_INFO, LM_DH, "Generate new log File: ", log_path);

    assertDo(!fh_fopen(1, log_path), LL_ERROR, LM_DH, "Can't open new or existing log", return false;);

    // Always start with an empty file
    fh_clear();

    fh_fwrite(0, size, (uint8_t*) log_buffer);
    fh_fclose();

    // Calculate next log idx:
    log_idx = (log_idx + 1) % MAX_LOG_IDX;
}