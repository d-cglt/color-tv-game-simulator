#include "generator.h"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: generate-selectors SYSTEM_DIRECTORY\n");
        return 2;
    }
    const auto result = ctg::generate_game_files(argv[1]);
    if (result.created != ctg::slot_count || result.existing || result.failed) {
        std::fprintf(stderr, "Selector generation incomplete: %u created, %u existing, %u failed\n",
                     result.created, result.existing, result.failed);
        return 1;
    }
    return 0;
}
