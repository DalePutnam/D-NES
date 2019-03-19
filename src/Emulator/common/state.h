#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <memory>
#include <vector>

#include "nes_exception.h"

class State
{
public:
    typedef std::unique_ptr<State> Ptr;

    static State::Ptr New()
    {
        return State::Ptr(new State());
    }

    static State::Ptr New(const std::unique_ptr<char[]>& data, size_t length)
    {
        return State::Ptr(new State(data, length));
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
        if (buffer.size() - index < sizeof(T))
        {
            buffer.resize(buffer.size() + sizeof(T));
        }

        memcpy(buffer.data() + index, &value, sizeof(T));
        index += sizeof(T);
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
        if (buffer.size() - index < (length * sizeof(T)) + sizeof(size_t))
        {
            buffer.resize(buffer.size() + (length * sizeof(T)) + sizeof(size_t));
        }

        memcpy(buffer.data() + index, &length, sizeof(size_t));
        index += sizeof(size_t);

        memcpy(buffer.data() + index, data, length * sizeof(T));
        index += length * sizeof(T);
    }

    void StoreSubState(const State::Ptr& subState)
    {
        if (buffer.size() - index < subState->GetSize() + sizeof(size_t))
        {
            buffer.resize(buffer.size() + subState->GetSize() + sizeof(size_t));
        }

        size_t subStateSize = subState->GetSize();
        memcpy(buffer.data() + index, &subStateSize, sizeof(size_t));
        index += sizeof(size_t);

        memcpy(buffer.data() + index, subState->GetBuffer(), subState->GetSize());
        index += subState->GetSize();
    }

    template<typename T>
    void ExtractValue(T& value) const
    {
        if (buffer.size() - index < sizeof(T))
        {
            throw NesException("State", "Tried to extract state value that was out of range");
        }

        memcpy(&value, buffer.data() + index, sizeof(T));
        index += sizeof(T);
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
    void ExtractBuffer(T* data)
    {
        if (buffer.size() - index < sizeof(size_t))
        {
            throw NesException("State", "Tried to extract state value that was out of range");
        }

        size_t length;
        memcpy(&length, buffer.data() + index, sizeof(size_t));
        index += sizeof(size_t);

        if (buffer.size() - index < length * sizeof(T))
        {
            throw NesException("State", "Tried to extract state value that was out of range");
        }

        memcpy(data, buffer.data() + index, length * sizeof(T));
        index += length * sizeof(T);
    }

    void ExtractSubState(State::Ptr& subState)
    {
        if (buffer.size() - index < sizeof(size_t))
        {
            throw NesException("State", "Tried to extract state value that was out of range");
        }

        size_t subStateSize;
        memcpy(&subStateSize, buffer.data() + index, sizeof(size_t));
        index += sizeof(size_t);

        if (buffer.size() - index < subStateSize)
        {
            throw NesException("State", "Tried to extract state value that was out of range");
        }

        subState = State::New(std::make_unique<char[]>(subStateSize), subStateSize);

        memcpy(subState.get()->buffer.data(), buffer.data() + index, subState->GetSize());
        index += subState->GetSize();
    }

private:
    State() = default;
    explicit State(const std::unique_ptr<char[]>& data, size_t length)
        : index(0)
        , buffer(data.get(), data.get() + length)
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

    mutable size_t index{0};
    std::vector<char> buffer;
};
