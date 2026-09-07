#pragma once
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <type_traits>

// Fixed-width little-endian fields; never persist object layout or pointers.
namespace VideoState
{
template<class T> void Write(std::ostream& out, T value)
{
    static_assert(std::is_integral_v<T>);
    uint64_t bits = static_cast<uint64_t>(value);
    for (size_t i = 0; i < sizeof(T); ++i) out.put(static_cast<char>(bits >> (i * 8)));
    if (!out) throw std::runtime_error("Unable to write video state");
}
template<class T> void Read(std::istream& in, T& value)
{
    static_assert(std::is_integral_v<T>);
    uint64_t bits = 0;
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        int byte = in.get();
        if (byte == std::char_traits<char>::eof()) throw std::runtime_error("Truncated video state");
        bits |= uint64_t(static_cast<uint8_t>(byte)) << (i * 8);
    }
    value = static_cast<T>(bits);
}
}
