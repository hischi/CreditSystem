#pragma once

#include "Platform_Types.h"

using StdErrorCode = uint8;
const StdErrorCode E_OK = 0x00;
const StdErrorCode E_NOT_OK = 0x01;

template <typename T>
class StdReturn {

public:
    StdReturn(StdErrorCode error) : error(error) { }
    StdReturn(T value) : error(E_OK), value(value) { }

    StdReturn(const StdReturn& other) : error(other.error), value(other.value) { }
    void operator= (const StdReturn& other) {
        this->error = other.error;
        this->value = other.value;
    }

    operator bool() const {
        return (error == E_OK);
    }

    bool IsError() const {
        return (error != E_OK);
    }

    bool HasValue() const {
        return (error == E_OK);
    }

    T& operator() () const {
        return value;
    }

    const T* operator& () const{
        return &value;
    }

    StdErrorCode GetError() const {
        return error;
    }

private:
    StdErrorCode error;
    T value;
};

template <>
class StdReturn<void> {

public:
    StdReturn(StdErrorCode error) : error(error) { }
    StdReturn() : error(E_OK) { }

    StdReturn(const StdReturn& other) : error(other.error) { }
    void operator= (const StdReturn& other) {
        this->error = other.error;
    }

    operator bool() const {
        return (error == E_OK);
    }

    bool IsError() const {
        return (error != E_OK);
    }

    bool HasValue() const {
        return (error == E_OK);
    }

    StdErrorCode GetError() const {
        return error;
    }

private:
    StdErrorCode error;
};