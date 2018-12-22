#include "rfid.h"
#include "Desfire.h"
#include "Secrets.h"
#include "Buffer.h"
#include "../util/error.h"

#define PN532_RESET_PIN 6 // Not connected
#define PN532_MISO_PIN 5
#define PN532_MOSI_PIN 21
#define PN532_CLK_PIN 20
#define PN532_CS_PIN 22

#define USE_DESFIRE   true              
#define USE_AES   false
#define COMPILE_SELFTEST  0
#define ALLOW_ALSO_CLASSIC   false

#define DESFIRE_KEY_TYPE   DES
#define DEFAULT_APP_KEY    pn532.DES3_DEFAULT_KEY

struct sCard
{
    byte        uidLength;   // UID = 4 or 7 bytes
    byte        keyVersion;  // for Desfire random ID cards
    bool        PN532_Error; // true -> the error comes from the PN532, false -> crypto error
    eCardType   cardType;    
};

bool pn532_ready = false;
Desfire pn532;
DESFIRE_KEY_TYPE piccMasterKey;
DESFIRE_KEY_TYPE appKey;

void reset_reader() {
    log(LL_INFO, LM_RFID, "Reader will be reset now...");

    do // pseudo loop (just used for aborting with break;)
    {
        pn532_ready = false;
      
        // Reset the PN532
        pn532.begin(); // delay > 400 ms
    
        byte IC, VersionHi, VersionLo, Flags;
        if (!pn532.GetFirmwareVersion(&IC, &VersionHi, &VersionLo, &Flags))
            break;
    
        char Buf[80];
        sprintf(Buf, "Chip: PN5%02X, Firmware version: %d.%d", IC, VersionHi, VersionLo);
        log(LL_DEBUG, LM_RFID, Buf);
        sprintf(Buf, "Supports ISO 14443A:%s, ISO 14443B:%s, ISO 18092:%s", (Flags & 1) ? "Yes" : "No",
                                                                                (Flags & 2) ? "Yes" : "No",
                                                                                (Flags & 4) ? "Yes" : "No");
        log(LL_DEBUG, LM_RFID, Buf);
         
        // Set the max number of retry attempts to read from a card.
        // This prevents us from waiting forever for a card, which is the default behaviour of the PN532.
        if (!pn532.SetPassiveActivationRetries())
            break;
        
        // configure the PN532 to read RFID tags
        if (!pn532.SamConfig())
            break;
    
        pn532_ready = true;
    }
    while (false);

    assertRtn(!pn532_ready, LL_ERROR, LM_RFID, "Reader is not ready");
    log(LL_INFO, LM_RFID, "Reader ready.");
}

bool authenticatePICC(uint8_t* keyVersion) {
    log(LL_DEBUG, LM_RFID, "authenticatePICC");

    if (!pn532.SelectApplication(0x000000)) // PICC level
        return false;

    if (!pn532.GetKeyVersion(0, keyVersion)) // Get version of PICC master key
        return false;

    // The factory default key has version 0, while a personalized card has key version CARD_KEY_VERSION
    if (*keyVersion == CARD_KEY_VERSION)
    {
        if (!pn532.Authenticate(0, &piccMasterKey))
            return false;
    }
    else // The card is still in factory default state
    {
        if (!pn532.Authenticate(0, &pn532.DES2_DEFAULT_KEY))
            return false;
    }
    return true;
}

void rfid_init() {
    log(LL_DEBUG, LM_RFID, "rfid_init");

    pn532.InitHardwareSPI(PN532_CLK_PIN, PN532_MISO_PIN, PN532_MOSI_PIN, PN532_CS_PIN, PN532_RESET_PIN);
    reset_reader();
    piccMasterKey.SetKeyData(SECRET_PICC_MASTER_KEY, sizeof(SECRET_PICC_MASTER_KEY), CARD_KEY_VERSION);
    appKey.SetKeyData(SECRET_APPLICATION_KEY, sizeof(SECRET_APPLICATION_KEY), CARD_KEY_VERSION);
}

bool rfid_card_present() {
    log(LL_DEBUG, LM_RFID, "rfid_card_present");

    assertDo(!pn532_ready, LL_WARNING, LM_RFID, "Reader not ready. Can't detect card. Try to reset...", reset_reader(); return false;);

    uint8_t uid[8];
    sCard card;
    memset(&card, 0, sizeof(card));

    assertDo (!pn532.ReadPassiveTargetID(uid, &card.uidLength, &card.cardType), LL_ERROR, LM_RFID, "PN532 error during ReadPassiveTargetID", card.PN532_Error = true; return false;);

    if (card.cardType == CARD_Desfire && card.uidLength > 0) // The card is a Desfire card in random ID mode
    {
        log(LL_DEBUG, LM_RFID, "Desfire card is present");
        return true;
    } else if(card.uidLength == 0)
        log(LL_DEBUG, LM_RFID, "No valid Desfire found");
    else
        assertCnt(true, LL_WARNING, LM_RFID, "Card found but invalid");
    return false;
}

void rfid_low_power_mode() {
    assertCnt(!pn532.SwitchOffRfField(), LL_WARNING, LM_RFID, "Can't turn off RF-Field");
}

bool rfid_read_tennis_app(uint8_t tennisCardID[], uint8_t tennisCustomerID[]) {
    log(LL_DEBUG, LM_RFID, "rfid_read_tennis_app");
    
    assertDo (!pn532.SelectApplication(0x000000), LL_ERROR, LM_RFID, "Can't move to PICC level", return false;); // PICC level

    byte u8_Version; 
    assertDo (!pn532.GetKeyVersion(0, &u8_Version), LL_ERROR, LM_RFID, "Can't get PICC key version", return false;);
    log(LL_DEBUG, LM_RFID, "PICC key version on card:", (uint32_t) u8_Version);

    // The factory default key has version 0, while a personalized card has key version CARD_KEY_VERSION
    assertDo (u8_Version != CARD_KEY_VERSION, LL_ERROR, LM_RFID, "PICC Key versions don't match", return false;);

    assertDo (!pn532.SelectApplication(CARD_APPLICATION_ID), LL_ERROR, LM_RFID, "Can't select APP", return false;);

    assertDo (!pn532.Authenticate(0, &appKey), LL_ERROR, LM_RFID, "Can't authenticate APP", return false;);

    // Read the 16 byte secret from the card
    byte u8_FileData[16];
    assertDo (!pn532.ReadFileData(CARD_FILE_ID, 0, 16, u8_FileData), LL_ERROR, LM_RFID, "Can't read data", return false;);

    memcpy(tennisCardID, u8_FileData, 8);
    memcpy(tennisCustomerID, u8_FileData+8, 8);

    return true;
}

bool rfid_store_tennis_app(uint8_t tennisCardID[], uint8_t tennisCustomerID[]) {
    log(LL_DEBUG, LM_RFID, "rfid_store_tennis_app");

    assertDo (CARD_APPLICATION_ID == 0x000000 || CARD_KEY_VERSION == 0, LL_ERROR, LM_RFID, "severe errors in Secrets.h -> abort", return false;);
  
    byte u8_StoreValue[16];

    memcpy(u8_StoreValue, tennisCardID, 8);
    memcpy(u8_StoreValue+8, tennisCustomerID, 8);

    // First delete the application (The current application master key may have changed after changing the user name for that card)
    assertDo (!pn532.DeleteApplicationIfExists(CARD_APPLICATION_ID), LL_ERROR, LM_RFID, "Can't delete APP", return false;);

    // Create the new application with default settings (we must still have permission to change the application master key later)
    assertDo (!pn532.CreateApplication(CARD_APPLICATION_ID, KS_FACTORY_DEFAULT, 1, appKey.GetKeyType()), LL_ERROR, LM_RFID, "Can't create new APP", return false;);

    // After this command all the following commands will apply to the application (rather than the PICC)
    assertDo (!pn532.SelectApplication(CARD_APPLICATION_ID), LL_ERROR, LM_RFID, "Can't select newly created APP", return false;);

    // Authentication with the application's master key is required
    assertDo (!pn532.Authenticate(0, &DEFAULT_APP_KEY), LL_ERROR, LM_RFID, "Can't authenticate with default APP key", return false;);

    // Change the master key of the application
    assertDo (!pn532.ChangeKey(0, &appKey, NULL), LL_ERROR, LM_RFID, "Can't change to new APP key", return false;);

    // A key change always requires a new authentication with the new key
    assertDo (!pn532.Authenticate(0, &appKey), LL_ERROR, LM_RFID, "Can't authorise with new APP key", return false;);

    // After this command the application's master key and it's settings will be frozen. They cannot be changed anymore.
    // To read or enumerate any content (files) in the application the application master key will be required.
    // Even if someone knows the PICC master key, he will neither be able to read the data in this application nor to change the app master key.
    assertDo (!pn532.ChangeKeySettings(KS_CHANGE_KEY_FROZEN), LL_ERROR, LM_RFID, "APP key can't be frozen", return false;);

    // --------------------------------------------

    // Create Standard Data File with 16 bytes length
    DESFireFilePermissions k_Permis;
    k_Permis.e_ReadAccess         = AR_KEY0;
    k_Permis.e_WriteAccess        = AR_KEY0;
    k_Permis.e_ReadAndWriteAccess = AR_KEY0;
    k_Permis.e_ChangeAccess       = AR_KEY0;
    assertDo (!pn532.CreateStdDataFile(CARD_FILE_ID, &k_Permis, 16), LL_ERROR, LM_RFID, "Data File can't be created", return false;);

    // Write the StoreValue into that file
    assertDo (!pn532.WriteFileData(CARD_FILE_ID, 0, 16, u8_StoreValue), LL_ERROR, LM_RFID, "Write of tennis data was not successful", return false;);
  
    return true;
}

bool rfid_restore_card() {
    log(LL_DEBUG, LM_RFID, "rfid_restore_card");

    byte u8_KeyVersion;
    assertDo (!authenticatePICC(&u8_KeyVersion), LL_ERROR, LM_RFID, "Can't authenticate card", return false;);

    // If the key version is zero AuthenticatePICC() has already successfully authenticated with the factory default DES key
    if (u8_KeyVersion == 0)
        return true;

    // An error in DeleteApplication must not abort. 
    // The key change below is more important and must always be executed.
    bool b_Success = pn532.DeleteApplicationIfExists(CARD_APPLICATION_ID);
    if (!b_Success)
    {
        // After any error the card demands a new authentication
        assertDo (!pn532.Authenticate(0, &piccMasterKey), LL_ERROR, LM_RFID, "Can't authenticate after delete app", return false;);
    }
    
    assertDo (!pn532.ChangeKey(0, &pn532.DES2_DEFAULT_KEY, NULL), LL_ERROR, LM_RFID, "Key change was not succesful", return false;);

    // Check if the key change was successfull
    assertDo (!pn532.Authenticate(0, &pn532.DES2_DEFAULT_KEY), LL_ERROR, LM_RFID, "Key change can't be verified", return false;);

    return b_Success;
}

bool rfid_set_PICC()
{
    log(LL_DEBUG, LM_RFID, "rfid_set_PICC");

    byte u8_KeyVersion;
    assertDo (!authenticatePICC(&u8_KeyVersion), LL_ERROR, LM_RFID, "Can't authenticate card", return false;);

    if (u8_KeyVersion != CARD_KEY_VERSION) // empty card
    {
        // Store the secret PICC master key on the card.
        assertDo (!pn532.ChangeKey(0, &piccMasterKey, NULL), LL_ERROR, LM_RFID, "Can't change key", return false;);

        // A key change always requires a new authentication
        assertDo (!pn532.Authenticate(0, &piccMasterKey), LL_ERROR, LM_RFID, "Key change can't be verified", return false;);
    }
    return true;
}