#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

void put_u16(std::string& dst, std::uint16_t value);
void put_u32(std::string& dst, std::uint32_t value);
bool get_u16(const std::string& src, std::size_t offset, std::uint16_t& value);
bool get_u32(const std::string& src, std::size_t offset, std::uint32_t& value);
