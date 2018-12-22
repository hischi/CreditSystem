/**************************************************************************
    @file     PN532.cpp
    @author   Adafruit Industries, Elmü
    @license  BSD (see license.txt)

      Driver for NXP's PN532 NFC/13.56MHz RFID Transceiver

      This is a library for the Adafruit PN532 NFC/RFID breakout board.
      This library works with the Adafruit NFC breakout
      ----> https://www.adafruit.com/products/364

      Check out the links above for our tutorials and wiring diagrams
      These chips use SPI or I2C to communicate.

      Adafruit invests time and resources providing this open source code,
      please support Adafruit and open-source hardware by purchasing products from Adafruit!

    ----------------------------------------------------------
    April 2016, modifications by Elmü:
    The code from Adafruit was a VERY SLOPPY code just for testing and playing around but not usable for production.
    It has been rewritten completely by Elmü.
    IRQ is not required anymore in I2C mode. Now the software handshake is used instead.
    Removed all compiler warnings that appeared when compiling Adafruit code.
    Bugfix: Adafruit used strncmp() to compare binary data which is completey wrong -> replaced with memcmp()
    Bugfix: (Severe bug) Adafruit code does not check for valid return packets. The checksum is completely irgnored. Bytes received before the start code are not skipped!
    Bugfix: (Severe bug) Adafruit code used a timeout = 0 (wait forever). This is completely wrong. If the chip does not respond, the code hangs forever!
    Bugfix: Adafruit code does not allow to distinguish why readPassiveTargetID() returns false. (because there is no card or because of communication problem?)
    Added support for Value blocks (in Mifare.cpp)
    Added memory Dump (in Mifare.cpp)
    AuthenticateDataBlock(), ReadDataBlock() and WriteDataBlock() rewritten (in Mifare.cpp)
    Implemented the correct wake up procedure (sending PN532_WAKEUP) instead of sending getFirmwareVersion.
    Debug output was buggy: The checksum bytes were displayed as 0xFFFFFFFC instead of 0xFC and removed useless "0x" before each byte.
    Detailed debug output was missing. Added display of valid data bytes inside the packet.
    SPI slow speed added (using Software SPI to get 10kHz clock)
    getFirmwareVersion() was a very clumsy and cryptic command -> completely rewritten
    writeGPIO() rewritten -> no warning about wrong usage anymore.
    setPassiveActivationRetries() did not have any error checking at all.
    Ugly code in writecommand() completely rewritten
    Crappy code like this removed:   int offset = mb_UsingSPI ? 5 : 6;
    ----------------------------------------------------------

    Check for a new version on http://www.codeproject.com/Articles/1096861/DIY-electronic-RFID-Door-Lock-with-Battery-Backup
    
**************************************************************************/

#include "PN532.h"
#include "../util/error.h"

/**************************************************************************
    Constructor
**************************************************************************/
PN532::PN532()
{
    mu8_ClkPin     = 0;
    mu8_MisoPin    = 0;  
    mu8_MosiPin    = 0;  
    mu8_SselPin    = 0;  
    mu8_ResetPin   = 0;
}

/**************************************************************************
    Initializes for hardware I2C usage.
    param  reset     The RSTPD_N pin
**************************************************************************/
#if USE_HARDWARE_I2C
    void PN532::InitI2C(byte u8_Reset)
    {
        mu8_ResetPin = u8_Reset;
        Utils::SetPinMode(mu8_ResetPin, OUTPUT);
    }
#endif

/**************************************************************************
    Initializes for software SPI usage.
    param  clk       SPI clock pin (SCK)
    param  miso      SPI MISO pin
    param  mosi      SPI MOSI pin
    param  sel       SPI chip select pin (CS/SSEL)
    param  reset     Location of the RSTPD_N pin
**************************************************************************/
#if USE_SOFTWARE_SPI
    void PN532::InitSoftwareSPI(byte u8_Clk, byte u8_Miso, byte u8_Mosi, byte u8_Sel, byte u8_Reset)
    {
        mu8_ClkPin     = u8_Clk;
        mu8_MisoPin    = u8_Miso;
        mu8_MosiPin    = u8_Mosi;
        mu8_SselPin    = u8_Sel;
        mu8_ResetPin   = u8_Reset;
    
        Utils::SetPinMode(mu8_ResetPin, OUTPUT);  
        Utils::SetPinMode(mu8_SselPin,  OUTPUT);
        Utils::SetPinMode(mu8_ClkPin,   OUTPUT);   
        Utils::SetPinMode(mu8_MosiPin,  OUTPUT);
        Utils::SetPinMode(mu8_MisoPin,  INPUT);
    }
#endif

/**************************************************************************
    Initializes for hardware SPI uage.
    param  sel       SPI chip select pin (CS/SSEL)
    param  reset     Location of the RSTPD_N pin
**************************************************************************/
#if USE_HARDWARE_SPI
    void PN532::InitHardwareSPI(byte u8_Clk, byte u8_Miso, byte u8_Mosi, byte u8_Sel, byte u8_Reset)
    {
        mu8_ClkPin     = u8_Clk;
        mu8_MisoPin    = u8_Miso;
        mu8_MosiPin    = u8_Mosi;
        mu8_SselPin    = u8_Sel;
        mu8_ResetPin   = u8_Reset;
    
        Utils::SetPinMode(mu8_ResetPin, OUTPUT);
        Utils::SetPinMode(mu8_SselPin,  OUTPUT);
    }
#endif

/**************************************************************************
    Reset the PN532, wake up and start communication
**************************************************************************/
void PN532::begin() 
{
    log(LL_DEBUG, LM_PN532, "begin");

    Utils::WritePin(mu8_ResetPin, HIGH);
    Utils::DelayMilli(10);
    Utils::WritePin(mu8_ResetPin, LOW);
    Utils::DelayMilli(400);
    Utils::WritePin(mu8_ResetPin, HIGH);
    Utils::DelayMilli(10);  // Small delay required before taking other actions after reset. See datasheet section 12.23, page 209.
  
    #if (USE_HARDWARE_SPI || USE_SOFTWARE_SPI) 
    {
        #if USE_HARDWARE_SPI
            SpiClass::Begin(mu8_ClkPin, mu8_MisoPin, mu8_MosiPin, PN532_HARD_SPI_CLOCK);
        #endif

        // Wake up the PN532 (chapter 7.2.11) -> send a sequence of 0x55 (dummy bytes)
        byte u8_Buffer[20];
        memset(u8_Buffer, PN532_WAKEUP, sizeof(u8_Buffer));
        SendPacket(u8_Buffer, sizeof(u8_Buffer));

        log_hexdump(LL_DEBUG, LM_PN532, "Send WakeUp packet:", sizeof(u8_Buffer), u8_Buffer);
    }
    #elif USE_HARDWARE_I2C
    {
        I2cClass::Begin();
    }
    #endif
}

/**************************************************************************
    Gets the firmware version of the PN5xx chip
    returns:
    pIcType = Version of the IC. For PN532, this byte is 0x32
    pVersionHi, pVersionLo = Firmware version
    pFlags, bit 0 = Support of ISO 14443A
    pFlags, bit 1 = Support of ISO 14443B
    pFlags, bit 2 = Support of ISO 18092
**************************************************************************/
bool PN532::GetFirmwareVersion(byte* pIcType, byte* pVersionHi, byte* pVersionLo, byte* pFlags) 
{
    log(LL_DEBUG, LM_PN532, "GetFirmwareVersion");
    
    mu8_PacketBuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
    if (!SendCommandCheckAck(mu8_PacketBuffer, 1))
        return 0;

    byte len = ReadData(mu8_PacketBuffer, 13);
    assertDo(len != 6 || mu8_PacketBuffer[1] != PN532_COMMAND_GETFIRMWAREVERSION + 1, LL_ERROR, LM_PN532, "GetFirmwareVersion failed", return false;);

    *pIcType    = mu8_PacketBuffer[2];
    *pVersionHi = mu8_PacketBuffer[3];
    *pVersionLo = mu8_PacketBuffer[4];
    *pFlags     = mu8_PacketBuffer[5];    
    return true;
}

/**************************************************************************
    Configures the SAM (Secure Access Module)
**************************************************************************/
bool PN532::SamConfig(void)
{
    log(LL_DEBUG, LM_PN532, "SamConfig");
  
    mu8_PacketBuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
    mu8_PacketBuffer[1] = 0x01; // normal mode;
    mu8_PacketBuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    mu8_PacketBuffer[3] = 0x01; // use IRQ pin!
  
    if (!SendCommandCheckAck(mu8_PacketBuffer, 4))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 9);
    assertDo(len != 2 || mu8_PacketBuffer[1] != PN532_COMMAND_SAMCONFIGURATION + 1, LL_ERROR, LM_PN532, "SamConfig failed", return false;);

    return true;
}

/**************************************************************************
    Sets the amount of reties that the PN532 tries to activate a target
**************************************************************************/
bool PN532::SetPassiveActivationRetries() 
{
    log(LL_DEBUG, LM_PN532, "SetPassiveActivationRetries");
  
    mu8_PacketBuffer[0] = PN532_COMMAND_RFCONFIGURATION;
    mu8_PacketBuffer[1] = 5;    // Config item 5 (MaxRetries)
    mu8_PacketBuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
    mu8_PacketBuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
    mu8_PacketBuffer[4] = 3;    // one retry is enough for Mifare Classic but Desfire is slower (if you modify this, you must also modify PN532_TIMEOUT!)
    
    if (!SendCommandCheckAck(mu8_PacketBuffer, 5))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 9);
    assertDo(len != 2 || mu8_PacketBuffer[1] != PN532_COMMAND_RFCONFIGURATION + 1, LL_ERROR, LM_PN532, "SetPassiveActivationRetries failed", return false;);

    return true;
}

/**************************************************************************
    Turns the RF field off.
    When the field is on, the PN532 consumes approx 110 mA
    When the field is off, the PN532 consumes approx 18 mA
    The RF field is turned on again by ReadPassiveTargetID().
**************************************************************************/
bool PN532::SwitchOffRfField() 
{
    log(LL_DEBUG, LM_PN532, "SwitchOffRfField");
  
    mu8_PacketBuffer[0] = PN532_COMMAND_RFCONFIGURATION;
    mu8_PacketBuffer[1] = 1; // Config item 1 (RF Field)
    mu8_PacketBuffer[2] = 0; // Field Off
    
    if (!SendCommandCheckAck(mu8_PacketBuffer, 3))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 9);
    assertDo(len != 2 || mu8_PacketBuffer[1] != PN532_COMMAND_RFCONFIGURATION + 1, LL_ERROR, LM_PN532, "SwitchOffRfField failed", return false;);

    return true;
}

/**************************************************************************/
/*!
    Writes an 8-bit value that sets the state of the PN532's GPIO pins

    All pins that can not be used as GPIO should ALWAYS be left high
    (value = 1) or the system will become unstable and a HW reset
    will be required to recover the PN532.

    pinState[0] (01) = P30   Can be used as GPIO
    pinState[1] (02) = P31   Can be used as GPIO
    pinState[2] (04) = P32   *** RESERVED (Must be set) ***
    pinState[3] (08) = P33   Can be used as GPIO
    pinState[4] (10) = P34   *** RESERVED (Must be set) ***
    pinState[5] (20) = P35   Can be used as GPIO

    This function is not used. The original intention was to drive a LED that 
    is connected to the PN532 board. But the pins deliver so few current 
    that a connected LED is very dark. (even if connected without resistor!)
    Additionally the red LED cannot be connected to the PN532 because it should
    flash if there is a communication error with the PN532. But if there is a
    communication problem the command WRITEGPIO will never arrive at the PN532
    and the red LED would never flash.
*/
/**************************************************************************/
bool PN532::WriteGPIO(bool P30, bool P31, bool P33, bool P35)
{
    log(LL_DEBUG, LM_PN532, "WriteGPIO");
  
    byte pinState = (P30 ? PN532_GPIO_P30 : 0) |
                    (P31 ? PN532_GPIO_P31 : 0) |
                           PN532_GPIO_P32      |
                    (P33 ? PN532_GPIO_P33 : 0) |
                           PN532_GPIO_P34      |
                    (P35 ? PN532_GPIO_P35 : 0);

    mu8_PacketBuffer[0] = PN532_COMMAND_WRITEGPIO;
    mu8_PacketBuffer[1] = PN532_GPIO_VALIDATIONBIT | pinState;  // P3 Pins
    mu8_PacketBuffer[2] = 0x00;                                 // P7 GPIO Pins (not used ... taken by SPI)
                    
    if (!SendCommandCheckAck(mu8_PacketBuffer, 3))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 9);
    assertDo(len != 2 || mu8_PacketBuffer[1] != PN532_COMMAND_WRITEGPIO + 1, LL_ERROR, LM_PN532, "WriteGPIO failed", return false;);

    return true;
}


/**************************************************************************
    Waits for an ISO14443A target to enter the field.
    If the RF field has been turned off before, this command switches it on.

    param u8_UidBuffer  Pointer to an 8 byte buffer that will be populated with the card's UID (4 or 7 bytes)
    param pu8_UidLength Pointer to the variable that will hold the length of the card's UID.
    param pe_CardType   Pointer to the variable that will hold if the card is a Desfire card
    
    returns false only on error!
    returns true and *UidLength = 0 if no card was found
    returns true and *UidLength > 0 if a card has been read successfully
**************************************************************************/
bool PN532::ReadPassiveTargetID(byte* u8_UidBuffer, byte* pu8_UidLength, eCardType* pe_CardType) 
{
    log(LL_DEBUG, LM_PN532, "ReadPassiveTargetID");
      
    *pu8_UidLength = 0;
    *pe_CardType   = CARD_Unknown;
    memset(u8_UidBuffer, 0, 8);
      
    mu8_PacketBuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    mu8_PacketBuffer[1] = 1;  // read data of 1 card (The PN532 can read max 2 targets at the same time)
    mu8_PacketBuffer[2] = CARD_TYPE_106KB_ISO14443A; // This function currently does not support other card types.
  
    if (!SendCommandCheckAck(mu8_PacketBuffer, 3))
        return false; // Error (no valid ACK received or timeout)
  
    /* 
    ISO14443A card response:
    mu8_PacketBuffer Description
    -------------------------------------------------------
    b0               D5 (always) (PN532_PN532TOHOST)
    b1               4B (always) (PN532_COMMAND_INLISTPASSIVETARGET + 1)
    b2               Amount of cards found
    b3               Tag number (always 1)
    b4,5             SENS_RES (ATQA = Answer to Request Type A)
    b6               SEL_RES  (SAK  = Select Acknowledge)
    b7               UID Length
    b8..Length       UID (4 or 7 bytes)
    nn               ATS Length     (Desfire only)
    nn..Length-1     ATS data bytes (Desfire only)
    */ 
    byte len = ReadData(mu8_PacketBuffer, 28);
    assertDo (len < 3 || mu8_PacketBuffer[1] != PN532_COMMAND_INLISTPASSIVETARGET + 1, LL_ERROR, LM_PN532, "ReadPassiveTargetID failed", return false;);

    byte cardsFound = mu8_PacketBuffer[2]; 
    log(LL_DEBUG, LM_PN532, "Cards found:", (uint32_t) cardsFound);

    if (cardsFound != 1)
        return true; // no card found -> this is not an error!

    byte u8_IdLength = mu8_PacketBuffer[7];
    if (u8_IdLength != 4 && u8_IdLength != 7)
    {
        log(LL_WARNING, LM_PN532, "Cards found:", (uint32_t) u8_IdLength);
        return true; // unsupported card found -> this is not an error!
    }   

    memcpy(u8_UidBuffer, mu8_PacketBuffer + 8, u8_IdLength);    
    *pu8_UidLength = u8_IdLength;

    // See "Mifare Identification & Card Types.pdf" in the ZIP file
    uint16_t u16_ATQA = ((uint16_t)mu8_PacketBuffer[4] << 8) | mu8_PacketBuffer[5];
    byte     u8_SAK   = mu8_PacketBuffer[6];

    if (u8_IdLength == 7 && u8_UidBuffer[0] != 0x80 && u16_ATQA == 0x0344 && u8_SAK == 0x20) *pe_CardType = CARD_Desfire;
    if (u8_IdLength == 4 && u8_UidBuffer[0] == 0x80 && u16_ATQA == 0x0304 && u8_SAK == 0x20) *pe_CardType = CARD_DesRandom;
    
    if(checkLogLevel(LM_PN532, LL_DEBUG)) {
        log_hexdump(LL_DEBUG, LM_PN532, "Card UID:", u8_IdLength, u8_UidBuffer);

        // Examples:              ATQA    SAK  UID length
        // MIFARE Mini            00 04   09   4 bytes
        // MIFARE Mini            00 44   09   7 bytes
        // MIFARE Classic 1k      00 04   08   4 bytes
        // MIFARE Classic 4k      00 02   18   4 bytes
        // MIFARE Ultralight      00 44   00   7 bytes
        // MIFARE DESFire Default 03 44   20   7 bytes
        // MIFARE DESFire Random  03 04   20   4 bytes
        // See "Mifare Identification & Card Types.pdf"
        char s8_Buf[80];
        sprintf(s8_Buf, "Card Type:   ATQA= 0x%04X, SAK= 0x%02X", u16_ATQA, u8_SAK);

        if (*pe_CardType == CARD_Desfire)   strcat(s8_Buf, " (Desfire Default)");
        if (*pe_CardType == CARD_DesRandom) strcat(s8_Buf, " (Desfire RandomID)");
            
        log(LL_DEBUG, LM_PN532, s8_Buf);
    }
    return true;
}

/**************************************************************************
    The goal of this command is to select the target. (Initialization, anti-collision loop and Selection)
    If the target is already selected, no action is performed and Status OK is returned. 
**************************************************************************/
bool PN532::SelectCard()
{
    log(LL_DEBUG, LM_PN532, "SelectCard");
  
    mu8_PacketBuffer[0] = PN532_COMMAND_INSELECT;
    mu8_PacketBuffer[1] = 1; // Target 1

    if (!SendCommandCheckAck(mu8_PacketBuffer, 2))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 10);
    assertDo (len < 3 || mu8_PacketBuffer[1] != PN532_COMMAND_INSELECT + 1, LL_ERROR, LM_PN532, "SelectCard failed", return false;);

    return CheckPN532Status(mu8_PacketBuffer[2]);
}

/**************************************************************************
    The goal of this command is to deselect the target. 
    The PN532 keeps all the information relative to this target (also certain error status).  
    This function is required due to a stupid behaviour with Mifare Classic:
    When AuthenticateDataBlock() has failed for a sector, you also get an
    authentication error for the next sector although you have passed the correct key.
    So, after an authentication error you must first deselect the card before
    authenticating a new sector!
**************************************************************************/
bool PN532::DeselectCard()
{
    log(LL_DEBUG, LM_PN532, "DeselectCard");
  
    mu8_PacketBuffer[0] = PN532_COMMAND_INDESELECT;
    mu8_PacketBuffer[1] = 0; // Deselect all cards

    if (!SendCommandCheckAck(mu8_PacketBuffer, 2))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 10);
    assertDo (len < 3 || mu8_PacketBuffer[1] != PN532_COMMAND_INDESELECT + 1, LL_ERROR, LM_PN532, "DeselectCard failed", return false;);

    return CheckPN532Status(mu8_PacketBuffer[2]);
}

/**************************************************************************
    The goal of this command is to release the target.
    Releasing a target means that the host controller has finished the communication with 
    the target, so the PN532 erases all the information relative to it. 
**************************************************************************/
bool PN532::ReleaseCard()
{
    log(LL_DEBUG, LM_PN532, "ReleaseCard");
  
    mu8_PacketBuffer[0] = PN532_COMMAND_INRELEASE;
    mu8_PacketBuffer[1] = 0; // Deselect all cards

    if (!SendCommandCheckAck(mu8_PacketBuffer, 2))
        return false;
  
    byte len = ReadData(mu8_PacketBuffer, 10);
    assertDo (len < 3 || mu8_PacketBuffer[1] != PN532_COMMAND_INRELEASE + 1, LL_ERROR, LM_PN532, "ReleaseCard failed", return false;);

    return CheckPN532Status(mu8_PacketBuffer[2]);
}

/**************************************************************************
    This function is private
    It checks the status byte that is returned by some commands.
    See chapter 7.1 in the manual.
    u8_Status = the status byte
**************************************************************************/
bool PN532::CheckPN532Status(byte u8_Status)
{
    log(LL_DEBUG, LM_PN532, "CheckPN532Status");

    // Bits 0...5 contain the error code.
    u8_Status &= 0x3F;

    if (u8_Status == 0)
        return true;

    char s8_Buf[128];
    sprintf(s8_Buf, "PN532 Error 0x%02X: ", u8_Status);

    switch (u8_Status)
    {
        case 0x01: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Timeout"));
            return false;
        case 0x02: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "CRC error"));
            return false;
        case 0x03: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Parity error"));
            return false;
        case 0x04: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Wrong bit count during anti-collision"));
            return false;
        case 0x05: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Framing error"));
            return false;
        case 0x06: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Abnormal bit collision"));
            return false;
        case 0x07: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Insufficient communication buffer"));
            return false;
        case 0x09: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "RF buffer overflow"));
            return false;
        case 0x0A: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "RF field has not been switched on"));
            return false;
        case 0x0B: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "RF protocol error"));
            return false;
        case 0x0D: 
            assertCnt(true, LL_WARNING, LM_PN532, strcat(s8_Buf, "Overheating"));
            return false;
        case 0x0E: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Internal buffer overflow"));
            return false;
        case 0x10: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Invalid parameter"));
            return false;
        case 0x12: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Command not supported"));
            return false;
        case 0x13: 
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Wrong data format"));
            return false;
        case 0x14:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Authentication error"));
            return false;
        case 0x23:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Wrong UID check byte"));
            return false;
        case 0x25:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Invalid device state"));
            return false;
        case 0x26:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Operation not allowed"));
            return false;
        case 0x27:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "Command not acceptable"));
            return false;
        case 0x29:
            assertCnt(true, LL_INFO, LM_PN532, strcat(s8_Buf, "Target has been released"));
            return false;
        case 0x2A:
            assertCnt(true, LL_INFO, LM_PN532, strcat(s8_Buf, "Card has been exchanged"));
            return false;
        case 0x2B:
            assertCnt(true, LL_INFO, LM_PN532, strcat(s8_Buf, "Card has disappeared"));
            return false;
        case 0x2C:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "NFCID3 initiator/target mismatch"));
            return false;
        case 0x2D:
            assertCnt(true, LL_WARNING, LM_PN532, strcat(s8_Buf, "Over-current"));
            return false;
        case 0x2E:
            assertCnt(true, LL_ERROR, LM_PN532, strcat(s8_Buf, "NAD msssing"));
            return false;
        default:
            assertCnt(true, LL_FATAL, LM_PN532, strcat(s8_Buf, "Undocumented error"));
            return false;
    }
}

// ########################################################################
// ####                      LOW LEVEL FUNCTIONS                      #####
// ########################################################################


/**************************************************************************
    Return true if the PN532 is ready with a response.
**************************************************************************/
bool PN532::IsReady() 
{
    #if (USE_HARDWARE_SPI || USE_SOFTWARE_SPI) 
    {
        Utils::WritePin(mu8_SselPin, LOW);
        Utils::DelayMilli(2); // INDISPENSABLE!! Otherwise reads bullshit

        log(LL_DEBUG, LM_PN532, "IsReady(): write STATUSREAD");

        SpiWrite(PN532_SPI_STATUSREAD);
        byte u8_Ready = SpiRead();

        log_hexdump(LL_DEBUG, LM_PN532, "IsReady(): read", 1, &u8_Ready);
    
        Utils::WritePin(mu8_SselPin, HIGH);
        Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
        
        return u8_Ready == PN532_SPI_READY; // 0x01
    }
    #elif USE_HARDWARE_I2C
    { 
        // After reading this byte, the bus must be released with a Stop condition
        I2cClass::RequestFrom((byte)PN532_I2C_ADDRESS, (byte)1);

        // PN532 Manual chapter 6.2.4: Before the data bytes the chip sends a Ready byte.
        byte u8_Ready = I2cClass::Read();
        log_hexdump(LL_DEBUG, LM_PN532, "IsReady(): read", 1, &u8_Ready);        
        
        return u8_Ready == PN532_I2C_READY; // 0x01
    }
    #endif
}

/**************************************************************************
    Waits until the PN532 is ready.
**************************************************************************/
bool PN532::WaitReady() 
{
    uint16_t timer = 0;
    while (!IsReady()) 
    {
        assertDo (timer >= PN532_TIMEOUT, LL_ERROR, LM_PN532, "WaitReady() -> TIMEOUT", return false;);
        Utils::DelayMilli(10);
        timer += 10;        
    }
    return true;
}

/**************************************************************************
    Sends a command and waits a specified period for the ACK
    param cmd       Pointer to the command buffer
    param cmdlen    The size of the command in bytes

    returns  true  if everything is OK, 
             false if timeout occured before an ACK was recieved
**************************************************************************/
bool PN532::SendCommandCheckAck(byte *cmd, byte cmdlen) 
{
    WriteCommand(cmd, cmdlen);
    return ReadAck();
}

/**************************************************************************
    Writes a command to the PN532, inserting the
    preamble and required frame details (checksum, len, etc.)

    param  cmd       Command buffer
    param  cmdlen    Command length in bytes
**************************************************************************/
void PN532::WriteCommand(byte* cmd, byte cmdlen)
{
    byte TxBuffer[PN532_PACKBUFFSIZE + 10];
    int P=0;
    TxBuffer[P++] = PN532_PREAMBLE;    // 00
    TxBuffer[P++] = PN532_STARTCODE1;  // 00
    TxBuffer[P++] = PN532_STARTCODE2;  // FF
    TxBuffer[P++] = cmdlen + 1;
    TxBuffer[P++] = 0xFF - cmdlen;
    TxBuffer[P++] = PN532_HOSTTOPN532; // D4
    
    for (byte i=0; i<cmdlen; i++) 
    {
        TxBuffer[P++] = cmd[i];
    }

    byte checksum = 0;
    for (byte i=0; i<P; i++) 
    {
       checksum += TxBuffer[i];
    }

    TxBuffer[P++] = ~checksum;
    TxBuffer[P++] = PN532_POSTAMBLE; // 00

    SendPacket(TxBuffer, P);
   
    log_hexdump(LL_DEBUG, LM_PN532, "Sending:", P, TxBuffer);
}

/**************************************************************************
    Send a data packet
**************************************************************************/
void PN532::SendPacket(byte* buff, byte len)
{
    #if (USE_HARDWARE_SPI || USE_SOFTWARE_SPI) 
    {
        Utils::WritePin(mu8_SselPin, LOW);
        Utils::DelayMilli(2);  // INDISPENSABLE!!

        log(LL_DEBUG, LM_PN532, "WriteCommand(): write DATAWRITE");
        SpiWrite(PN532_SPI_DATAWRITE);

        for (byte i=0; i<len; i++) 
        {
            SpiWrite(buff[i]);
        }

        Utils::WritePin(mu8_SselPin, HIGH);
        Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
    }
    #elif USE_HARDWARE_I2C
    {
        Utils::DelayMilli(2); // delay is for waking up the board
    
        I2cClass::BeginTransmission(PN532_I2C_ADDRESS);
        for (byte i=0; i<len; i++) 
        {
            I2cClass::Write(buff[i]);
        }   
        I2cClass::EndTransmission();
    }
    #endif
}

/**************************************************************************
    Read the ACK packet (acknowledge)
**************************************************************************/
bool PN532::ReadAck() 
{
    const byte Ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    byte ackbuff[sizeof(Ack)];
    
    // ATTENTION: Never read more than 6 bytes here!
    // The PN532 has a bug in SPI mode which results in the first byte of the response missing if more than 6 bytes are read here!
    if (!ReadPacket(ackbuff, sizeof(ackbuff)))
        return false; // Timeout

    log_hexdump(LL_DEBUG, LM_PN532, "Read ACK", sizeof(ackbuff), ackbuff);
    
    assertDo(memcmp(ackbuff, Ack, sizeof(Ack)) != 0, LL_ERROR, LM_PN532, "No ACK frame received", return false;);

    return true;
}

/**************************************************************************
    Reads n bytes of data from the PN532 via SPI or I2C and checks for valid data.
    param  buff      Pointer to the buffer where data will be written
    param  len       Number of bytes to read
    returns the number of bytes that have been copied to buff (< len) or 0 on error
**************************************************************************/
byte PN532::ReadData(byte* buff, byte len) 
{ 
    byte RxBuffer[PN532_PACKBUFFSIZE];
        
    const byte MIN_PACK_LEN = 2 /*start bytes*/ + 2 /*length + length checksum */ + 1 /*checksum*/;
    assertDo (len < MIN_PACK_LEN || len > PN532_PACKBUFFSIZE, LL_ERROR, LM_PN532, "ReadData(): len is invalid", return 0;);
    
    if (!ReadPacket(RxBuffer, len))
        return 0; // timeout

    // The following important validity check was completely missing in Adafruit code (added by Elmü)
    // PN532 documentation says (chapter 6.2.1.6): 
    // Before the start code (0x00 0xFF) there may be any number of additional bytes that must be ignored.
    // After the checksum there may be any number of additional bytes that must be ignored.
    // This function returns ONLY the pure data bytes:
    // any leading bytes -> skipped (never seen, but documentation says to ignore them)
    // preamble   0x00   -> skipped (optional, the PN532 does not send it always!!!!!)
    // start code 0x00   -> skipped
    // start code 0xFF   -> skipped
    // length            -> skipped
    // length checksum   -> skipped
    // data[0...n]       -> returned to the caller (first byte is always 0xD5)
    // checksum          -> skipped
    // postamble         -> skipped (optional, the PN532 may not send it!)
    // any bytes behind  -> skipped (never seen, but documentation says to ignore them)

    const char* Error = NULL;
    int Brace1 = -1;
    int Brace2 = -1;
    int dataLength = 0;
    do
    {
        int startCode = -1;
        for (int i=0; i<=len-MIN_PACK_LEN; i++)
        {
            if (RxBuffer[i]   == PN532_STARTCODE1 && 
                RxBuffer[i+1] == PN532_STARTCODE2)
            {
                startCode = i;
                break;
            }
        }

        if (startCode < 0)
        {
            Error = "ReadData() -> No Start Code\r\n";
            break;
        }
        
        int pos = startCode + 2;
        dataLength      = RxBuffer[pos++];
        int lengthCheck = RxBuffer[pos++];
        if ((dataLength + lengthCheck) != 0x100)
        {
            Error = "ReadData() -> Invalid length checksum\r\n";
            break;
        }
    
        if (len < startCode + MIN_PACK_LEN + dataLength)
        {
            Error = "ReadData() -> Packet is longer than requested length\r\n";
            break;
        }

        Brace1 = pos;
        for (int i=0; i<dataLength; i++)
        {
            buff[i] = RxBuffer[pos++]; // copy the pure data bytes in the packet
        }
        Brace2 = pos;

        // All returned data blocks must start with PN532TOHOST (0xD5)
        if (dataLength < 1 || buff[0] != PN532_PN532TOHOST) 
        {
            Error = "ReadData() -> Invalid data (no PN532TOHOST)\r\n";
            break;
        }
    
        byte checkSum = 0;
        for (int i=startCode; i<pos; i++)
        {
            checkSum += RxBuffer[i];
        }
    
        if (checkSum != (byte)(~RxBuffer[pos]))
        {
            Error = "ReadData() -> Invalid checksum\r\n";
            break;
        }
    }
    while(false); // This is not a loop. Avoids using goto by using break.

    // Always print the package, even if it was invalid.
    log_hexdump(LL_DEBUG, LM_PN532, "Response:", len, RxBuffer);
    
    assertDo(Error, LL_ERROR, LM_PN532, Error, return 0;);

    return dataLength;
}

/**************************************************************************
    Reads n bytes of data from the PN532 via SPI or I2C and does NOT check for valid data.
    param  buff      Pointer to the buffer where data will be written
    param  len       Number of bytes to read
**************************************************************************/
bool PN532::ReadPacket(byte* buff, byte len)
{ 
    if (!WaitReady())
        return false;
        
    #if (USE_HARDWARE_SPI || USE_SOFTWARE_SPI) 
    {
        Utils::WritePin(mu8_SselPin, LOW);
        Utils::DelayMilli(2); // INDISPENSABLE!! Otherwise reads bullshit

        log(LL_DEBUG, LM_PN532, "ReadPacket(): write DATAREAD");
        SpiWrite(PN532_SPI_DATAREAD);
    
        for (byte i=0; i<len; i++) 
        {
            Utils::DelayMilli(1);
            buff[i] = SpiRead();
        }
    
        Utils::WritePin(mu8_SselPin, HIGH);
        Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
        return true;
    }
    #elif USE_HARDWARE_I2C
    {
        Utils::DelayMilli(2);
    
        // read (n+1 to take into account leading Ready byte)
        I2cClass::RequestFrom((byte)PN532_I2C_ADDRESS, (byte)(len+1));

        // PN532 Manual chapter 6.2.4: Before the data bytes the chip sends a Ready byte.
        // It is ignored here because it has been checked already in isready()
        byte u8_Ready = I2cClass::Read();
        log_hexdump(LL_DEBUG, LM_PN532, "ReadPacket(): read", 1, &u8_Ready);
        
        for (byte i=0; i<len; i++) 
        {
            Utils::DelayMilli(1);
            buff[i] = I2cClass::Read();
        }
        return true;
    }
    #endif
}

/**************************************************************************
    SPI write one byte
**************************************************************************/
void PN532::SpiWrite(byte c) 
{
    #if USE_HARDWARE_SPI
    {
        SpiClass::Transfer(c);
    }
    #elif USE_SOFTWARE_SPI
    {
        Utils::WritePin(mu8_ClkPin, HIGH);
        Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
    
        for (int i=1; i<=128; i<<=1) 
        {
            Utils::WritePin(mu8_ClkPin, LOW);
            Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
            
            byte level = (c & i) ? HIGH : LOW;
            Utils::WritePin(mu8_MosiPin, level);
            Utils::DelayMicro(PN532_SOFT_SPI_DELAY);        
      
            Utils::WritePin(mu8_ClkPin, HIGH);
            Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
        }
    }
    #endif
}

/**************************************************************************
    SPI read one byte
**************************************************************************/
byte PN532::SpiRead(void) 
{
    #if USE_HARDWARE_SPI 
    {
        return SpiClass::Transfer(0x00);
    }
    #elif USE_SOFTWARE_SPI
    {
        Utils::WritePin(mu8_ClkPin, HIGH);
        Utils::DelayMicro(PN532_SOFT_SPI_DELAY);

        int x=0;    
        for (int i=1; i<=128; i<<=1) 
        {
            if (Utils::ReadPin(mu8_MisoPin)) 
            {
                x |= i;
            }
            Utils::WritePin(mu8_ClkPin, LOW);
            Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
            Utils::WritePin(mu8_ClkPin, HIGH);
            Utils::DelayMicro(PN532_SOFT_SPI_DELAY);
        }
        return x;
    }
    #else
    {
        return 0; // This code will never execute. Just for the compiler not to complain.
    }
    #endif
}