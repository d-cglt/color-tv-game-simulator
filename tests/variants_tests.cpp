#include "libretro.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
unsigned current_id = 0;
unsigned buttons[2]{};
unsigned video_calls = 0, poll_calls = 0, audio_calls = 0;
bool bad_video = false, bad_audio = false;
std::vector<unsigned char> frame;

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "variant %u, %s:%d: %s\n", current_id, __FILE__, __LINE__, #condition); \
    return false; \
} } while (0)

bool environment(unsigned command, void* data) {
    switch (command) {
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *static_cast<retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888;
    default:
        return false;
    }
}

void video(const void* data, unsigned width, unsigned height, size_t pitch) {
    ++video_calls;
    if (!data || width != 320 || height != 240 || pitch != width * 4) {
        bad_video = true;
        return;
    }
    const auto* bytes = static_cast<const unsigned char*>(data);
    frame.assign(bytes, bytes + pitch * height);
}

void poll() { ++poll_calls; }
int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    return port < 2 && device == RETRO_DEVICE_JOYPAD && index == 0 && id < 32 &&
           (buttons[port] & (1u << id)) ? 1 : 0;
}
size_t audio(const int16_t* data, size_t count) {
    ++audio_calls;
    if (!data || count != 800) bad_audio = true;
    return count;
}

void press(unsigned button) {
    buttons[0] = 0;
    retro_run();
    buttons[0] = 1u << button;
    retro_run();
    buttons[0] = 0;
    retro_run();
}

bool test_variant(unsigned id) {
    current_id = id;
    buttons[0] = buttons[1] = 0;
    std::array<unsigned char, 5> content{{'C', 'T', 'G', '1', static_cast<unsigned char>(id)}};
    retro_game_info game{};
    game.data = content.data();
    game.size = content.size();
    CHECK(retro_load_game(&game));
    const unsigned first_video = video_calls, first_poll = poll_calls, first_audio = audio_calls;
    retro_run();
    CHECK(video_calls == first_video + 1 && poll_calls == first_poll + 1 && audio_calls == first_audio + 1);
    CHECK(!bad_video && !bad_audio && !frame.empty());
    const auto default_menu = frame;

    // Mode for competitive games, automatic/manual target for shooting.
    press(RETRO_DEVICE_ID_JOYPAD_DOWN);
    auto before = frame;
    press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    CHECK(frame != before);
    // Exercise speed, both sizes, and score/shots through the same menu.
    for (unsigned option = 0; option < 4; ++option) {
        press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        before = frame;
        press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        CHECK(frame != before);
    }
    retro_reset();
    retro_run();
    const auto configured_menu = frame;
    CHECK(configured_menu != default_menu);
    press(RETRO_DEVICE_ID_JOYPAD_A);
    CHECK(frame != configured_menu);
    const auto initial_game = frame;

    buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    buttons[1] = 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    for (unsigned n = 0; n < 20; ++n) retro_run();
    CHECK(frame != initial_game);
    buttons[0] = buttons[1] = 0;
    if (id == 14) press(RETRO_DEVICE_ID_JOYPAD_Y);
    for (unsigned n = 0; n < 60; ++n) retro_run();
    before = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(frame != before);
    before = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(frame != before);
    CHECK(!bad_video && !bad_audio);

    retro_reset();
    retro_run();
    CHECK(frame == configured_menu);
    retro_unload_game();
    const unsigned unloaded_video = video_calls, unloaded_audio = audio_calls;
    retro_run();
    CHECK(video_calls == unloaded_video && audio_calls == unloaded_audio);
    CHECK(retro_load_game(&game));
    retro_run();
    CHECK(frame == configured_menu);
    retro_unload_game();
    return true;
}
}

int main() {
    retro_set_environment(environment);
    retro_set_video_refresh(video);
    retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll);
    retro_set_input_state(input);
    retro_init();
    if (retro_api_version() != RETRO_API_VERSION) {
        std::fprintf(stderr, "libretro API version mismatch\n");
        retro_deinit();
        return 1;
    }
    for (unsigned id = 0; id < 15; ++id) {
        if (!test_variant(id)) {
            retro_unload_game();
            retro_deinit();
            return 1;
        }
        std::printf("variant %u passed\n", id);
    }
    retro_deinit();
    return 0;
}
