#pragma once

#include <SD.h>
#include <SPI.h>
#include "../util/Std_Types.h"
#include "../util/Logger.h"

class FileSystem : public LoggerInterface {

public:
    static const StdErrorCode HwNotReady = 0x02;
    static const StdErrorCode NoFileSystemFound = 0x03;
    static const StdErrorCode NoSuchFile = 0x04;
    static const StdErrorCode NoSuchDir = 0x05;
    static const StdErrorCode OtherFileInUse = 0x06;
    static const StdErrorCode NoFileOpened = 0x07;
    static const StdErrorCode DirAlreadyExisted = 0x08;
    static const StdErrorCode EndOfFile = 0x09;

public:
    static FileSystem& Get() {
        static FileSystem fileSystem;
        return fileSystem;
    }

    ~FileSystem() { }

    StdNoReturn Init();

    StdNoReturn OpenFile(const char* path);
    StdNoReturn CloseFile();
    StdNoReturn MakeDir(const char* path, const char* name);

    StdReturn<uint32> Read(uint32 pos, char* buffer, uint32 len);
    StdReturn<uint32> Write(uint32 pos, char* buffer, uint32 len);
    StdReturn<uint32> Append(char* buffer, uint32 len);
    StdReturn<uint32> Size();
    StdNoReturn Clear();

private:
    FileSystem() : LoggerInterface("FileSys"), ready(false) { }
    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    void OpenRoot();
    StdNoReturn Open(const char* path);

    bool ready;
    SdFile currentFile;
    SdVolume fileSystem;
    Sd2Card sdCard;
};