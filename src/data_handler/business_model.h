#pragma once

#ifdef WIN32
#include <stdint.h>
#else
#include <Arduino.h>
#endif

#include "../util/checksum.h"

#define DB_VERSION "0.1"

/*********************************************
 * Member Properties
 ********************************************/
#define DH_MEMBERPROP_NOT_ITEM1 0x0001
#define DH_MEMBERPROP_NOT_ITEM2 0x0002
#define DH_MEMBERPROP_NOT_ITEM3 0x0004
#define DH_MEMBERPROP_NOT_ITEM4 0x0008
#define DH_MEMBERPROP_NOT_ITEM5 0x0010
#define DH_MEMBERPROP_NOT_ITEM6 0x0020

#define DH_MEMBERPROP_CLUBCARD  0x0100
#define DH_MEMBERPROP_TEAMCARD  0x0200
//********************************************

/*********************************************
 * Transaction status
 ********************************************/
#define DH_TA_CREATED   0x00
#define DH_TA_APPROVED  0x01
#define DH_TA_COMPLETED 0x02
#define DH_TA_CANCLED   0x10
#define DH_TA_TIMEOUT   0x20
#define DH_TA_CORRUPTED 0x80
//********************************************


#pragma pack(push, 1)
struct sDataBaseHeader {
    uint32_t    version;                    // Version of the Data-Base
    uint32_t    datetime_modified;          // Date and Time of the last Data-Base Change
    char        author[16];                 // Name of the last author
    uint32_t    entry_count;                // Number of entries which directly follow this header
}; // 28 Bytes
#pragma pack (pop)

#pragma pack(push, 1)
struct sMember {
    uint32_t    id;                         // unique id of that member
    char        name[16];                   // surname
    char        given_name[16];             // given name
    uint16_t    properties;                 // properties, see Member Properties above
    uint16_t    discount;                   // Discount in %
    uint32_t    card_id;                    // card id which is linked to that member
}; // 44 Bytes
#pragma pack (pop)

#pragma pack(push, 1)
struct sTransactionHeader {
    uint32_t    version;                    // Version of the SW which generated the transaction list
    uint32_t    datetime_modified;          // Date and Time of the last change
    uint32_t    entry_count;                // Number of entries which follow this header
}; // 12 Bytes
#pragma pack (pop)

#pragma pack(push, 1)
struct sTransactionHeaderCS {
    sTransactionHeader  header;             // Transaction Header
    uint32_t            checksum;           // Header checksum
}; // 16 Bytes
#pragma pack (pop)

#pragma pack(push, 1)
struct sTransaction {
    uint32_t    id;                         // unique ID of this transaction
    uint32_t    datetime_modified;          // Date and Time of the transaction
    uint32_t    member_id;                  // who was the customer
    uint8_t     status;                     // status of this transaction (see eTransactionStatus)
    uint8_t     item_id;                    // which item was selected
    uint32_t    cost;                       // cost of the item in cents
    uint16_t    discount;                   // discount which was granted and is already included in cost
}; // 20 Bytes
#pragma pack (pop)

#pragma pack(push, 1)
struct sTransactionCS {
    sTransaction    transaction;            // Transaction
    uint32_t        checksum;               // Transaction checksum
}; // 24 Bytes
#pragma pack (pop)


