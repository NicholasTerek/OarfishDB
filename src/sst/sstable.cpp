#include "src/include/sst/sstable.hpp"

#include <algorithm>
#include <fstream>

#include "src/include/util/encoding.hpp"

namespace {
std::string encode_block_meta(const std::vector<BlockMeta>& meta) {
    std::string out;
    out.reserve(4);
    put_u32(out, static_cast<std::uint32_t>(meta.size()));
    for (const auto& m : meta) {
        if (m.first_key.size() > 65535 || m.last_key.size() > 65535) {
            continue;
        }
        put_u16(out, static_cast<std::uint16_t>(m.first_key.size()));
        out.append(m.first_key);
        put_u16(out, static_cast<std::uint16_t>(m.last_key.size()));
        out.append(m.last_key);
        put_u32(out, m.offset);
        put_u32(out, m.size);
    }
    return out;
}

std::vector<BlockMeta> decode_block_meta(const std::string& data) {
    std::vector<BlockMeta> meta;
    if (data.size() < sizeof(std::uint32_t)) {
        return meta;
    }
    std::uint32_t count = 0;
    if (!get_u32(data, 0, count)) {
        return meta;
    }
    std::size_t offset = sizeof(std::uint32_t);
    meta.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint16_t first_len = 0;
        std::uint16_t last_len = 0;
        if (!get_u16(data, offset, first_len)) {
            return meta;
        }
        offset += sizeof(std::uint16_t);
        if (offset + first_len > data.size()) {
            return meta;
        }
        std::string first_key = data.substr(offset, first_len);
        offset += first_len;

        if (!get_u16(data, offset, last_len)) {
            return meta;
        }
        offset += sizeof(std::uint16_t);
        if (offset + last_len > data.size()) {
            return meta;
        }
        std::string last_key = data.substr(offset, last_len);
        offset += last_len;

        std::uint32_t block_offset = 0;
        std::uint32_t block_size = 0;
        if (!get_u32(data, offset, block_offset)) {
            return meta;
        }
        offset += sizeof(std::uint32_t);
        if (!get_u32(data, offset, block_size)) {
            return meta;
        }
        offset += sizeof(std::uint32_t);

        meta.push_back(BlockMeta{std::move(first_key), std::move(last_key), block_offset, block_size});
    }
    return meta;
}

} // namespace

SsTable::SsTable() = default;

SsTable::SsTable(std::string file_path, std::vector<BlockMeta> block_meta)
    : file_path_(std::move(file_path)), block_meta_(std::move(block_meta)) {}

SsTable SsTable::open_for_test(const std::string& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        return SsTable();
    }
    in.seekg(0, std::ios::end);
    std::size_t size = static_cast<std::size_t>(in.tellg());
    if (size < sizeof(std::uint32_t)) {
        return SsTable(file_path, {});
    }
    std::string buffer(size, '\0');
    in.seekg(0, std::ios::beg);
    in.read(&buffer[0], static_cast<std::streamsize>(size));
    if (!in) {
        return SsTable(file_path, {});
    }

    std::uint32_t meta_offset = 0;
    if (!get_u32(buffer, size - sizeof(std::uint32_t), meta_offset)) {
        return SsTable(file_path, {});
    }
    if (meta_offset > size - sizeof(std::uint32_t)) {
        return SsTable(file_path, {});
    }
    std::string meta_section = buffer.substr(meta_offset, size - sizeof(std::uint32_t) - meta_offset);
    std::vector<BlockMeta> meta = decode_block_meta(meta_section);
    return SsTable(file_path, std::move(meta));
}

const std::vector<BlockMeta>& SsTable::block_meta() const {
    return block_meta_;
}

std::string SsTable::first_key() const {
    if (block_meta_.empty()) {
        return "";
    }
    return block_meta_.front().first_key;
}

std::string SsTable::last_key() const {
    if (block_meta_.empty()) {
        return "";
    }
    return block_meta_.back().last_key;
}

Block SsTable::read_block(std::size_t block_id) const {
    if (block_id >= block_meta_.size()) {
        return Block();
    }
    const BlockMeta& meta = block_meta_[block_id];
    std::ifstream in(file_path_, std::ios::binary);
    if (!in) {
        return Block();
    }
    in.seekg(meta.offset, std::ios::beg);
    std::string encoded(meta.size, '\0');
    in.read(&encoded[0], static_cast<std::streamsize>(meta.size));
    if (!in) {
        return Block();
    }
    return Block::decode(encoded);
}

SsTableBuilder::SsTableBuilder(std::uint32_t block_size)
    : block_size_(block_size),
      block_builder_(static_cast<std::uint16_t>(std::min<std::uint32_t>(block_size, 65535))) {}

void SsTableBuilder::add(const std::string& key, const std::string& value) {
    if (block_builder_.num_entries() == 0) {
        first_key_in_block_ = key;
    }
    if (!block_builder_.add(key, value)) {
        flush_block();
        first_key_in_block_ = key;
        bool ok = block_builder_.add(key, value);
        (void)ok;
    }
    last_key_in_block_ = key;
}

SsTable SsTableBuilder::build_for_test(const std::string& file_path) {
    flush_block();
    std::string meta_section = encode_block_meta(meta);
    std::uint32_t meta_offset = static_cast<std::uint32_t>(data_.size());
    std::string file_bytes = data_;
    file_bytes.append(meta_section);
    put_u32(file_bytes, meta_offset);

    std::ofstream out(file_path, std::ios::binary);
    out.write(file_bytes.data(), static_cast<std::streamsize>(file_bytes.size()));
    out.flush();

    return SsTable(file_path, meta);
}

std::size_t SsTableBuilder::estimated_size() const {
    return data_.size();
}

void SsTableBuilder::flush_block() {
    if (block_builder_.num_entries() == 0) {
        return;
    }
    Block block = block_builder_.build();
    std::string encoded = block.encode();
    std::uint32_t offset = static_cast<std::uint32_t>(data_.size());
    data_.append(encoded);
    std::uint32_t size = static_cast<std::uint32_t>(encoded.size());
    meta.push_back(BlockMeta{first_key_in_block_, last_key_in_block_, offset, size});
    first_key_in_block_.clear();
    last_key_in_block_.clear();
}
