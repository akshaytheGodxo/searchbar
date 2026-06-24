#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>

// ─── On-disk layouts ──────────────────────────────────────────────────────────
//
//  index.dat   [ FileRecord × N ]          (fixed-size, random-access)
//  ┌──────────────────────────────────────┐
//  │  uint32_t  id          (4 bytes)     │
//  │  uint64_t  pathOffset  (8 bytes)     │  ← byte offset into metadata.dat
//  └──────────────────────────────────────┘
//
//  metadata.dat  [ MetaDataRecord × N ]   (variable-length, sequential)
//  ┌──────────────────────────────────────┐
//  │  uint16_t  len  +  char[]  filename  │
//  │  uint16_t  len  +  char[]  filepath  │
//  │  uint16_t  len  +  char[]  extension │
//  │  uint16_t  len  +  char[]  file_id   │
//  └──────────────────────────────────────┘
// ─────────────────────────────────────────────────────────────────────────────

#pragma pack(push, 1)
struct FileRecord {
    uint32_t id;           // unique file ID
    uint64_t pathOffset;   // byte offset of this file's MetaData in metadata.dat
};
#pragma pack(pop)

struct MetaData {
    std::string filename;
    std::string filepath;
    std::string extension;
    std::string file_id;
};

// ─────────────────────────────────────────────────────────────────────────────

class FileStorage
{
public:
    explicit FileStorage(
        const std::string& indexPath = "index.dat",
        const std::string& metaPath = "metadata.dat")
        : m_indexPath(indexPath)
        , m_metaPath(metaPath)
    {
    }

    // ── Write ────────────────────────────────────────────────────────────────

    /**
     * Append one record.
     * Returns the FileRecord that was written to index.dat so the caller
     * can store or verify it.
     */
    FileRecord Write(uint32_t id, const MetaData& meta)
    {
        // 1. Open metadata.dat in append-binary mode and remember the offset
        std::fstream metaFile(m_metaPath,
            std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
        if (!metaFile.is_open()) {
            // File doesn't exist yet – create it
            metaFile.open(m_metaPath,
                std::ios::out | std::ios::binary | std::ios::trunc);
        }
        if (!metaFile.is_open())
            throw std::runtime_error("Cannot open " + m_metaPath);

        const uint64_t offset = static_cast<uint64_t>(metaFile.tellp());

        // 2. Serialise MetaData (length-prefixed strings)
        WriteString(metaFile, meta.filename);
        WriteString(metaFile, meta.filepath);
        WriteString(metaFile, meta.extension);
        WriteString(metaFile, meta.file_id);
        metaFile.close();

        // 3. Build and append the FileRecord to index.dat
        FileRecord record{ id, offset };

        std::ofstream indexFile(m_indexPath,
            std::ios::binary | std::ios::app);
        if (!indexFile.is_open())
            throw std::runtime_error("Cannot open " + m_indexPath);

        indexFile.write(reinterpret_cast<const char*>(&record),
            sizeof(FileRecord));
        indexFile.close();

        return record;
    }

    // ── Read all records from index.dat ──────────────────────────────────────

    std::vector<FileRecord> ReadAllRecords() const
    {
        std::ifstream indexFile(m_indexPath, std::ios::binary);
        if (!indexFile.is_open())
            throw std::runtime_error("Cannot open " + m_indexPath);

        std::vector<FileRecord> records;
        FileRecord rec{};
        while (indexFile.read(reinterpret_cast<char*>(&rec), sizeof(FileRecord)))
            records.push_back(rec);

        return records;
    }

    // ── Look up MetaData for a single FileRecord ──────────────────────────────

    MetaData ReadMeta(const FileRecord& record) const
    {
        std::ifstream metaFile(m_metaPath, std::ios::binary);
        if (!metaFile.is_open())
            throw std::runtime_error("Cannot open " + m_metaPath);

        metaFile.seekg(static_cast<std::streamoff>(record.pathOffset));
        if (!metaFile)
            throw std::runtime_error("Bad offset in metadata.dat");

        MetaData meta;
        meta.filename = ReadString(metaFile);
        meta.filepath = ReadString(metaFile);
        meta.extension = ReadString(metaFile);
        meta.file_id = ReadString(metaFile);
        return meta;
    }

    // ── Convenience: read everything at once ─────────────────────────────────

    /**
     * Returns a vector of { FileRecord, MetaData } pairs in insertion order.
     */
    std::vector<std::pair<FileRecord, MetaData>> ReadAll() const
    {
        auto records = ReadAllRecords();
        std::vector<std::pair<FileRecord, MetaData>> out;
        out.reserve(records.size());
        for (const auto& rec : records)
            out.emplace_back(rec, ReadMeta(rec));
        return out;
    }

    // ── Find by ID (linear scan of index) ────────────────────────────────────

    bool FindById(uint32_t id, MetaData& outMeta) const
    {
        for (const auto& rec : ReadAllRecords()) {
            if (rec.id == id) {
                outMeta = ReadMeta(rec);
                return true;
            }
        }
        return false;
    }

private:
    std::string m_indexPath;
    std::string m_metaPath;

    // ── Binary string helpers ─────────────────────────────────────────────────

    /** Write uint16_t length prefix then raw bytes. */
    static void WriteString(std::ostream& out, const std::string& s)
    {
        if (s.size() > 0xFFFF)
            throw std::runtime_error("String too long for uint16_t prefix");

        const uint16_t len = static_cast<uint16_t>(s.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(s.data(), len);
    }

    /** Read uint16_t length prefix then raw bytes. */
    static std::string ReadString(std::istream& in)
    {
        uint16_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!in) throw std::runtime_error("Truncated metadata.dat");

        std::string s(len, '\0');
        in.read(s.data(), len);
        if (!in) throw std::runtime_error("Truncated metadata.dat (data)");
        return s;
    }
};