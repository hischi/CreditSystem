#include "FileSystem.h"


StdNoReturn FileSystem::Init() {
    currentFile.close();
    ready = false;
    
    if(!sdCard.init(SPI_HALF_SPEED, BUILTIN_SDCARD)) {
        Log("Initialisation of SD-Card failed", Logger::LL_ERROR);
        return HwNotReady;
    }

    if(!fileSystem.init(sdCard)) {
        Log("Could not find FAT16/FAT32 partition on SD-Card", Logger::LL_ERROR);
        return NoFileSystemFound;
    }

    ready = true;
    return StdNoReturn();
}

StdNoReturn FileSystem::OpenFile(const char* path) {
    Log("Opening file: ", path, Logger::LL_DEBUG);

    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }
    
    if(currentFile.isOpen()) {
        Log("Other file already opened", Logger::LL_WARNING);
        return OtherFileInUse;
    }

    // Always start at root as we assume path is absolute
    OpenRoot();
    StdNoReturn ret = Open(path);
    if(ret.IsError()) {
        currentFile.close();
        return ret;
    }

    if(currentFile.isOpen() && currentFile.isFile()) {
        Log("Opened file: ", path, Logger::LL_DEBUG);
        return StdNoReturn();
    } else {
        currentFile.close();
        Log("Path has not lead to a file: ", path, Logger::LL_ERROR);
        return NoSuchFile;
    }
}

StdNoReturn FileSystem::CloseFile() {
    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }

    if(currentFile.isOpen()) {
        currentFile.close();
        return StdNoReturn();
    } else {
        Log("No file was opened to be closed", Logger::LL_WARNING);
        return NoFileOpened;
    }
}

StdNoReturn FileSystem::MakeDir(const char* path, const char* name) {
    Log("Opening file: ", path, Logger::LL_DEBUG);

    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }
    
    if(currentFile.isOpen()) {
        Log("Other file already opened", Logger::LL_WARNING);
        return OtherFileInUse;
    }

    // Always start at root as we assume path is absolute
    OpenRoot();
    StdNoReturn ret = Open(path);
    if(ret.IsError()) {
        currentFile.close();
        return ret;
    }

    // Check if path lead to a dir
    if(!currentFile.isOpen() || currentFile.isFile()) {
        Log("Path has not lead to a file: ", path, Logger::LL_ERROR);
        return NoSuchDir;
    }

    // Create new dir
    SdFile newDir;
    if(newDir.makeDir(&currentFile, name) > 0) {
        currentFile.close();
        Log("New subdir created at: ", path);
        Log("Sub-Dir Name: ", name);
        return StdNoReturn();
    }
    else {
        currentFile.close();
        Log("Subdir already exists. Data could be overwritten. Dir-Name: ", name, Logger::LL_WARNING);
        return DirAlreadyExisted;
    }
}

StdReturn<uint32> FileSystem::Read(uint32 pos, char* buffer, uint32 len) {
    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }
    
    if(!currentFile.isOpen() || !currentFile.isFile()) {
        Log("No file opened", Logger::LL_WARNING);
        return NoFileOpened;
    }

    if(!currentFile.seekSet(pos)) {
        Log("Can't set file seek", Logger::LL_WARNING);
        return EndOfFile;
    }

    int32 readLen = currentFile.read(buffer, len);
    if(readLen < 0) {
        Log("Unknown read error", Logger::LL_ERROR);
        return E_NOT_OK;
    } else if(readLen == 0) {
        Log("End of File reached", Logger::LL_DEBUG);
        return EndOfFile;
    } else {
        return (uint32) readLen;
    }
}

StdReturn<uint32> FileSystem::Write(uint32 pos, char* buffer, uint32 len) {
    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }
    
    if(!currentFile.isOpen() || !currentFile.isFile()) {
        Log("No file opened", Logger::LL_WARNING);
        return NoFileOpened;
    }

    if(!currentFile.seekSet(pos)) {
        Log("Can't set file seek", Logger::LL_WARNING);
        return EndOfFile;
    }

    int32 writeLen = currentFile.write(buffer, len);
    if(writeLen < 0) {
        Log("Unknown write error", Logger::LL_ERROR);
        return E_NOT_OK;
    }
    
    if(!currentFile.sync()) {
        Log("Can't sync with SD-Card", Logger::LL_ERROR);
        return HwNotReady;
    }

    return (uint32) writeLen;

}

StdReturn<uint32> FileSystem::Append(char* buffer, uint32 len) {
    StdReturn<uint32> fileSize = Size();
    if(fileSize.HasValue()) {
        return Write(fileSize(), buffer, len);
    } else {
        return fileSize;
    }
}

StdReturn<uint32> FileSystem::Size() {
    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }
    
    if(!currentFile.isOpen() || !currentFile.isFile()) {
        Log("No file opened", Logger::LL_WARNING);
        return NoFileOpened;
    }

    return currentFile.fileSize();
}

StdNoReturn FileSystem::Clear() {
    if(!ready) {
        Log("File System not initialised", Logger::LL_ERROR);
        return HwNotReady;
    }
    
    if(!currentFile.isOpen() || !currentFile.isFile()) {
        Log("No file opened", Logger::LL_WARNING);
        return NoFileOpened;
    }

    if(!currentFile.truncate(0)) {
        return E_NOT_OK;
    }

    return StdNoReturn();
}

void FileSystem::OpenRoot() {
    currentFile.openRoot(fileSystem);
}

StdNoReturn FileSystem::Open(const char* path) {
    SdFile nextFile;
    static const uint32 MaxNameLen = 13;
    char name[MaxNameLen];
    uint32 pos = 0;

    // Find next seperator or end of path
    while(path[pos] != 0 && path[pos] != '/' && path[pos] != '\\') {
        pos++;
    }

    // Return on empty path or if it only contains seperator
    if(pos == 0) {
        return StdNoReturn();
    }

    // Extract next name from path
    if(pos >= MaxNameLen) {
        Log("Name in path too long: ", path, Logger::LL_ERROR);
        return E_NOT_OK;
    }
    memcpy(name, path, pos);
    name[pos] = 0;

    // Proceed if path continous
    if(path[pos] != 0) {
        if(!nextFile.open(currentFile, name, O_RDONLY)) {
            Log("Failed to open directory: ", name, Logger::LL_ERROR);
            return NoSuchDir;
        }
        currentFile.close();
        currentFile = nextFile;

        return Open(&path[pos+1]);
    } else {
        if(!nextFile.open(currentFile, name, O_RDWR | O_CREAT)) {
            Log("Failed to open file: ", name, Logger::LL_ERROR);
            return NoSuchDir;
        }
        currentFile.close();
        currentFile = nextFile;

        return StdNoReturn();
    }    
}