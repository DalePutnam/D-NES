#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <memory>
#include <vector>

#include "error.h"

class StateSave
{
public:
    typedef std::unique_ptr<StateSave> Ptr;

    static StateSave::Ptr New()
    {
        return StateSave::Ptr(new StateSave());
    }

    static StateSave::Ptr New(const std::unique_ptr<char[]>& data, size_t length)
    {
        return StateSave::Ptr(new StateSave(data, length));
    }

    const char* GetBuffer() const
    {
        return buffer.data();
    }

    size_t GetSize() const
    {
        return buffer.size();
    }

    template<typename T>
    void StoreValue(const T& value)
    {
        if (buffer.size() - writeIndex < sizeof(T))
        {
            buffer.resize(buffer.size() + sizeof(T));
        }

        memcpy(buffer.data() + writeIndex, &value, sizeof(T));
        writeIndex += sizeof(T);
    }

    template<typename... Ts,    
        typename = typename std::enable_if<
            // There can only be as many flags as can fit in a uint8_t
            sizeof...(Ts) <= std::numeric_limits<uint8_t>::digits
        >::type>
    void StorePackedValues(const Ts&... targs)
    {
        uint8_t packedData = 0;
        pack(packedData, targs...);
        StoreValue(packedData);
    }

    template<typename T>
    void StoreBuffer(const T* data, size_t length)
    {
        if (buffer.size() - writeIndex < length * sizeof(T))
        {
            buffer.resize(buffer.size() + (length * sizeof(T)));
        }

        memcpy(buffer.data() + writeIndex, data, length * sizeof(T));
        writeIndex += length * sizeof(T);
    }

    template<typename T>
    void ExtractValue(T& value) const
    {
        if (buffer.size() - readIndex < sizeof(T))
        {
            throw NesException(ERROR_STATE_LOAD_FAILED);
        }

        memcpy(&value, buffer.data() + readIndex, sizeof(T));
        readIndex += sizeof(T);
    }

    template<typename... Ts,
        typename = typename std::enable_if<
            // There can only be as many flags as can fit in a uint8_t
            sizeof...(Ts) <= std::numeric_limits<uint8_t>::digits
        >::type>
    void ExtractPackedValues(Ts&... targs)
    {
        uint8_t packedData;
        ExtractValue(packedData);
        unpack(packedData, targs...);
    }

    template<typename T>
    void ExtractBuffer(T* data, size_t length)
    {
        if (buffer.size() - readIndex < length * sizeof(T))
        {
            throw NesException(ERROR_STATE_LOAD_FAILED);
        }

        memcpy(data, buffer.data() + readIndex, length * sizeof(T));
        readIndex += length * sizeof(T);
    }

private:
    StateSave() = default;
    explicit StateSave(const std::unique_ptr<char[]>& data, size_t length)
        : buffer(data.get(), data.get() + length)
    {}

    template<typename T,
        typename = typename std::enable_if<
            // Flag must be convertible to bool
            std::is_convertible<T, bool>::value
        >::type>
    void pack(uint8_t& data, const T& flag)
    {
        data |= !!flag;
    }

    template<typename T, typename... Ts,    
        typename = typename std::enable_if<
            // All flags must be convertible to bool
            std::is_convertible<T, bool>::value
        >::type>
    void pack(uint8_t& data, const T& flag, const Ts&... targs)
    {
        data |= !!flag;
        data <<= 1;

        pack(data, targs...);
    }

    template<typename T,
        typename = typename std::enable_if<
            // Flag must be convertible to bool
            std::is_assignable<T&, bool>::value
        >::type>
    void unpack(uint8_t& data, T& flag)
    {
        flag = !!(data & 1);
    }

    template<typename T, typename... Ts, 
        typename = typename std::enable_if<
            // All flags must be able to be assigned a bool
            std::is_assignable<T&, bool>::value
        >::type>
    void unpack(uint8_t& data, T& flag, Ts&... targs)
    {
        unpack(data, targs...);

        data >>= 1;
        flag = !!(data & 1);
    }

    size_t writeIndex{0};
    mutable size_t readIndex{0};
    std::vector<char> buffer;
};
