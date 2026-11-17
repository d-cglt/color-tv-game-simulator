#include "preferences.h"
#include "content.h"
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ctg::preferences {
namespace {
bool atomic_write(const std::filesystem::path& path, const unsigned char* data,
                  std::size_t size, std::mt19937& random) noexcept {
    try {
        if (path.empty()) return false;
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            auto temporary = path;
            temporary += ".tmp-" + std::to_string(random());
#ifdef _WIN32
            const int fd = _wopen(temporary.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                  _S_IREAD | _S_IWRITE);
#else
            const int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
#endif
            if (fd < 0) { if (errno == EEXIST) continue; return false; }
            std::size_t offset = 0;
            while (offset < size) {
#ifdef _WIN32
                const int written = _write(fd, data + offset, static_cast<unsigned>(size - offset));
#else
                const ssize_t written = ::write(fd, data + offset, size - offset);
#endif
                if (written < 0 && errno == EINTR) continue;
                if (written <= 0) break;
                offset += static_cast<std::size_t>(written);
            }
#ifdef _WIN32
            const int closed = _close(fd);
            const bool replaced = offset == size && closed == 0 &&
                MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
            const int closed = close(fd);
            const bool replaced = offset == size && closed == 0 &&
                std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
            if (!replaced) { std::error_code ec; std::filesystem::remove(temporary, ec); }
            return replaced;
        }
    } catch (...) {}
    return false;
}

bool valid(const Data& data) {
    if (data[0] != 'C' || data[1] != 'T' || data[2] != 'G' || data[3] != 'P' || data[4] != 1 ||
        data[5] > 1 || data[6] > 1 || data[7] > 1 ||
        data[8] < 5 || data[8] > 30 || data[8] % 5 ||
        data[9] < 5 || data[9] > 30 || data[9] % 5 ||
        data[10] > 4 || data[11] > 2 || data[12] > 2) return false;
    for (std::size_t i = 13; i < data.size(); ++i) if (data[i] > 3) return false;
    return true;
}
}

bool read(const std::filesystem::path& path, Data& data) noexcept {
    try {
        if (path.empty()) return false;
        std::ifstream file(path, std::ios::binary);
        Data record{};
        if (!file.read(reinterpret_cast<char*>(record.data()), record.size()) ||
            file.peek() != std::char_traits<char>::eof() || file.bad() || !valid(record)) return false;
        data = record;
        return true;
    } catch (...) { return false; }
}

bool write(const std::filesystem::path& path, const Data& data, std::mt19937& random) noexcept {
    return atomic_write(path, data.data(), data.size(), random);
}

bool read_diagnostic(const std::filesystem::path& path, DiagnosticData& data) noexcept {
    try {
        if (path.empty()) return false;
        std::ifstream file(path, std::ios::binary);
        DiagnosticData record{};
        if (!file.read(reinterpret_cast<char*>(record.data()), record.size()) ||
            file.peek() != std::char_traits<char>::eof() || file.bad() ||
            record[0] != 'C' || record[1] != 'T' || record[2] != 'G' || record[3] != 'D' ||
            record[4] != 1 || record[5] > 1 || record[6] > 1 ||
            record[7] > 30 || record[8] > 30 || record[9] > 1) return false;
        data = record;
        return true;
    } catch (...) { return false; }
}

bool write_diagnostic(const std::filesystem::path& path, const DiagnosticData& data, std::mt19937& random) noexcept {
    return atomic_write(path, data.data(), data.size(), random);
}

unsigned read_last_game(const std::filesystem::path& path, unsigned fallback) noexcept {
    try {
        if (path.empty()) return fallback;
        std::ifstream file(path, std::ios::binary);
        char id;
        if (!file.get(id) || file.peek() != std::char_traits<char>::eof() || file.bad()) return fallback;
        const auto value = static_cast<unsigned char>(id);
        return value < ctg::slot_count ? value : fallback;
    } catch (...) { return fallback; }
}

bool write_last_game(const std::filesystem::path& path, unsigned id, std::mt19937& random) noexcept {
    const unsigned char value = static_cast<unsigned char>(id);
    return atomic_write(path, &value, 1, random);
}
}
