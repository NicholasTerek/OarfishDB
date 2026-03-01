#include "src/include/common/util/encoding.hpp"

void put_u16(std::string& dst, std::uint16_t value) {
    char buf[2];
    buf[0] = static_cast<char>(value & 0xFF);
    buf[1] = static_cast<char>((value >> 8) & 0xFF);
    dst.append(buf, sizeof(buf));
}

void put_u32(std::string& dst, std::uint32_t value) {
    char buf[4];
    buf[0] = static_cast<char>(value & 0xFF);
    buf[1] = static_cast<char>((value >> 8) & 0xFF);
    buf[2] = static_cast<char>((value >> 16) & 0xFF);
    buf[3] = static_cast<char>((value >> 24) & 0xFF);
    dst.append(buf, sizeof(buf));
}

bool get_u16(const std::string& src, std::size_t offset, std::uint16_t& value) {
    if (offset + sizeof(std::uint16_t) > src.size()) {
        return false;
    }
    const unsigned char* data = reinterpret_cast<const unsigned char*>(src.data() + offset);
    value = static_cast<std::uint16_t>(data[0]) |
            static_cast<std::uint16_t>(data[1]) << 8;
    return true;
}

bool get_u32(const std::string& src, std::size_t offset, std::uint32_t& value) {
    if (offset + sizeof(std::uint32_t) > src.size()) {
        return false;
    }
    const unsigned char* data = reinterpret_cast<const unsigned char*>(src.data() + offset);
    value = static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) << 8) |
            (static_cast<std::uint32_t>(data[2]) << 16) |
            (static_cast<std::uint32_t>(data[3]) << 24);
    return true;
}
