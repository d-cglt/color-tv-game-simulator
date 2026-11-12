#include "generator.h"
#include "content.h"
#include <cerrno>
#include <filesystem>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ctg {
const char* const generated_filenames[slot_count] = {
    "Tennis Variant - Singles (Color TV Game 15).ctg", "Tennis Variant - Doubles (Color TV Game 15).ctg",
    "Tennis Original - Singles (Color TV Game 6 + 15).ctg", "Tennis Original - Doubles (Color TV Game 6 + 15).ctg",
    "Hockey Variant - Singles (Color TV Game 15).ctg", "Hockey Variant - Doubles (Color TV Game 15).ctg",
    "Hockey Original - Singles (Color TV Game 6 + 15).ctg", "Hockey Original - Doubles (Color TV Game 6 + 15).ctg",
    "Volleyball Variant - Singles (Color TV Game 15).ctg", "Volleyball Variant - Doubles (Color TV Game 15).ctg",
    "Volleyball Original - Singles (Color TV Game 6 + 15).ctg", "Volleyball Original - Doubles (Color TV Game 6 + 15).ctg",
    "Ping-Pong - Singles (Color TV Game 15).ctg", "Ping-Pong - Doubles (Color TV Game 15).ctg",
    "Shooting Game (Color TV Game 15).ctg",
    "Racing 112 (Color TV Game Racing 112).ctg"
};
static_assert(slot_count == 16, "Update generated filenames for new slots");

GenerationResult generate_game_files(const char* system_directory) noexcept {
    GenerationResult result;
    if (!system_directory || !*system_directory) {
        result.failed = slot_count;
        return result;
    }
    try {
        const auto directory = std::filesystem::u8path(system_directory) / "Nintendo Color TV-Game";
        std::error_code ec;
        std::filesystem::create_directories(directory, ec);
        if (ec) {
            result.failed = slot_count;
            return result;
        }
        for (unsigned id = 0; id < slot_count; ++id) {
            const auto path = directory / generated_filenames[id];
#ifdef _WIN32
            int fd = _wopen(path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                            _S_IREAD | _S_IWRITE);
#else
            int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
#endif
            if (fd < 0) {
                if (errno == EEXIST) ++result.existing;
                else ++result.failed;
                continue;
            }
            const unsigned char bytes[] = {'C', 'T', 'G', '1', static_cast<unsigned char>(id)};
            unsigned offset = 0;
            while (offset < sizeof(bytes)) {
#ifdef _WIN32
                const int n = _write(fd, bytes + offset, static_cast<unsigned>(sizeof(bytes) - offset));
#else
                const auto n = write(fd, bytes + offset, sizeof(bytes) - offset);
#endif
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) break;
                offset += static_cast<unsigned>(n);
            }
#ifdef _WIN32
            const int closed = _close(fd);
#else
            const int closed = close(fd);
#endif
            if (offset == sizeof(bytes) && closed == 0) ++result.created;
            else ++result.failed;
        }
    } catch (...) {
        // Preserve counts for completed files and report all unprocessed slots.
        result.failed = slot_count - result.created - result.existing;
    }
    return result;
}
}
