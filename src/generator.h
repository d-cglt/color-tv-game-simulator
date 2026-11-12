#pragma once
#include "content.h"

namespace ctg {
// Names are fixed basenames in slot-ID order; never supplied by the frontend.
extern const char* const generated_filenames[slot_count];

struct GenerationResult {
    unsigned created = 0;
    unsigned existing = 0;
    unsigned failed = 0;
};

// system_directory is the frontend's system directory path. No exceptions escape.
GenerationResult generate_game_files(const char* system_directory) noexcept;
}
