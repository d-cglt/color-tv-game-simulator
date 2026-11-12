#include "libretro.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
unsigned current_id = 0;
unsigned buttons[2]{};
int16_t steering_axis = 0;
unsigned video_calls = 0, poll_calls = 0, audio_calls = 0;
bool bad_video = false, bad_audio = false;
std::vector<unsigned char> frame;
std::vector<std::int16_t> audio_frame;

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
    if (port == 0 && device == RETRO_DEVICE_ANALOG && index == RETRO_DEVICE_INDEX_ANALOG_LEFT &&
        id == RETRO_DEVICE_ID_ANALOG_X) return steering_axis;
    return port < 2 && device == RETRO_DEVICE_JOYPAD && index == 0 && id < 32 &&
           (buttons[port] & (1u << id)) ? 1 : 0;
}
size_t audio(const int16_t* data, size_t count) {
    ++audio_calls;
    if (!data || count != 800) bad_audio = true;
    else audio_frame.assign(data, data + count * 2);
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

bool test_racing() {
    current_id = 15;
    buttons[0] = buttons[1] = 0;
    std::array<unsigned char, 5> content{{'C', 'T', 'G', '1', 15}};
    retro_game_info game{};
    game.data = content.data(); game.size = content.size();
    CHECK(retro_load_game(&game));
    retro_run();
    CHECK(!bad_video && !bad_audio && !frame.empty());
    const auto menu = frame;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(frame != menu);
    CHECK(!audio_frame.empty() && audio_frame[0] != 0);
    const auto pixel = [](unsigned x, unsigned y) {
        std::uint32_t value;
        std::memcpy(&value, frame.data() + (y * 320 + x) * 4, sizeof(value));
        return value;
    };
    CHECK(pixel(0, 100) == 0x66594a && pixel(319, 100) == 0x66594a);
    CHECK(pixel(40, 100) == 0xc3f5df && pixel(43, 100) == 0xc3f5df);
    CHECK(pixel(276, 100) == 0xc3f5df && pixel(279, 100) == 0xc3f5df);
    CHECK(pixel(40, 200) == 0xc3f5df && pixel(279, 200) == 0xc3f5df);
    const std::array<const char*, 14> car_mask{{
        "-------00000000-------", "-------00000000-------",
        "0000---00000000---0000", "0000---00000000---0000", "0000000000000000000000",
        "0000---00000000---0000", "0000---00000000---0000",
        "-------00000000-------", "-------00000000-------",
        "0000---00000000---0000", "0000---00000000---0000", "0000000000000000000000",
        "0000---00000000---0000", "0000---00000000---0000"
    }};
    for (unsigned y = 0; y < 28; ++y)
        for (unsigned x = 0; x < 22; ++x)
            CHECK(pixel(149 + x, 198 + y) == (car_mask[y / 2][x] == '0' ? 0xeb65e7u : 0x39b800u));
    const auto countdown_frame = frame;
    buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    for (unsigned n = 0; n < 20; ++n) retro_run();
    CHECK(frame != countdown_frame);
    CHECK(pixel(160, 210) == 0xeb65e7);
    buttons[0] = 0;
    retro_reset(); retro_run();
    CHECK(frame == menu);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (unsigned n = 0; n < 241; ++n) retro_run();
    buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    for (unsigned n = 0; n < 40; ++n) retro_run();
    unsigned gap_pixels = 0;
    for (unsigned y = 0; y < 240; ++y) gap_pixels += pixel(40, y) == 0x39b800;
    CHECK(gap_pixels == 24);
    const auto left = frame;
    retro_reset(); buttons[0] = 0; retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (unsigned n = 0; n < 241; ++n) retro_run();
    buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    for (unsigned n = 0; n < 40; ++n) retro_run();
    CHECK(frame != left);
    buttons[0] = 0;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    const auto paused = frame;
    for (unsigned n = 0; n < 20; ++n) retro_run();
    CHECK(frame == paused);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(frame != paused);
    retro_unload_game();
    const unsigned unloaded_video = video_calls, unloaded_audio = audio_calls;
    retro_run();
    CHECK(video_calls == unloaded_video && audio_calls == unloaded_audio);
    CHECK(retro_load_game(&game)); retro_run();
    CHECK(frame == menu);

    // Compare identical timelines so traffic and animation cannot mask steering.
    auto steer = [&](int16_t axis, unsigned digital, bool activate_first, unsigned frames = 10) {
        steering_axis = 0; buttons[0] = 0;
        retro_reset(); retro_run();
        press(RETRO_DEVICE_ID_JOYPAD_START);
        for (unsigned n = 0; n < 241; ++n) retro_run();
        if (activate_first) {
            steering_axis = 20000;
            retro_run();
        }
        steering_axis = axis; buttons[0] = digital;
        for (unsigned n = 0; n < frames; ++n) retro_run();
        return frame;
    };
    // A centered player hits traffic; the synthesized effect alternates pitch.
    steering_axis = 0; buttons[0] = 0;
    retro_reset(); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START);
    const auto transitions = []() {
        unsigned count = 0;
        for (unsigned i = 2; i < audio_frame.size(); i += 2)
            if (audio_frame[i] != audio_frame[i - 2]) ++count;
        return count;
    };
    CHECK(transitions() >= 5 && transitions() <= 6);
    for (unsigned n = 0; n < 35; ++n) retro_run();
    CHECK(audio_frame[0] == 0);
    for (unsigned n = 0; n < 160; ++n) retro_run();
    CHECK(transitions() >= 15 && transitions() <= 17);
    for (unsigned n = 0; n < 45; ++n) retro_run();
    unsigned frames_until_sound = 0;
    while (frames_until_sound++ < 400) {
        retro_run();
        if (transitions() >= 20 && transitions() <= 22) break;
    }
    CHECK(frames_until_sound <= 400 && audio_frame[0] != 0);
    CHECK(transitions() >= 20 && transitions() <= 22);
    CHECK(pixel(160, 210) != 0xeb65e7);
    unsigned high_samples = 0;
    for (unsigned i = 0; i < audio_frame.size(); i += 2) {
        CHECK(audio_frame[i] == audio_frame[i + 1]);
        CHECK(audio_frame[i] == 6000 || audio_frame[i] == -667);
        high_samples += audio_frame[i] == 6000;
    }
    CHECK(high_samples >= 65 && high_samples <= 95);
    for (unsigned n = 0; n < 17; ++n) retro_run();
    CHECK(transitions() >= 10 && transitions() <= 12);
    CHECK(pixel(160, 210) == 0xeb65e7);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(audio_frame[0] == 0);
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(audio_frame[0] != 0);
    for (unsigned n = 0; n < 100; ++n) retro_run();
    CHECK(pixel(160, 210) == 0xeb65e7);
    CHECK(audio_frame[0] != 0);
    CHECK(transitions() >= 4 && transitions() <= 7);

    const unsigned left_button = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    const unsigned right_button = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    steer(-32768, 0, false, 120);
    CHECK(pixel(39, 202) == 0x66594a && pixel(40, 202) == 0xeb65e7);
    steer(32767, 0, false, 120);
    CHECK(pixel(280, 202) == 0x66594a && pixel(279, 202) == 0xeb65e7);
    const auto neutral = steer(0, 0, false);
    CHECK(steer(-32768, 0, false) == steer(0, left_button, false));
    CHECK(steer(32767, 0, false) == steer(0, right_button, false));
    CHECK(steer(0, left_button, false) != neutral);
    CHECK(steer(0, right_button, false) != neutral);
    const auto released_axis = steer(0, 0, true);
    CHECK(steer(0, left_button, true) != released_axis);
    CHECK(steer(0, right_button, true) != released_axis);
    const auto analog_right = steer(20000, 0, false);
    CHECK(steer(20000, right_button, false) == analog_right);
    CHECK(steer(20000, left_button, false) == analog_right);
    const auto analog_left = steer(-20000, 0, false);
    CHECK(steer(-20000, left_button, false) == analog_left);
    CHECK(steer(-20000, right_button, false) == analog_left);
    CHECK(analog_left != analog_right);

    // Move only far enough to clear approaching traffic, starting when it appears.
    // Keep the speed-selection buttons independent of the avoidance input.
    const auto avoid_traffic = [&](unsigned speed_buttons = 0) {
        int player_x = 160;
        for (unsigned x = 40; x < 280; ++x) {
            if (pixel(x, 210) == 0xeb65e7) {
                player_x = static_cast<int>(x) + 11;
                break;
            }
        }
        unsigned avoidance = 0;
        for (unsigned y = 9; y < 190; ++y) {
            bool found = false;
            for (unsigned x = 40; x < 280; ++x) {
                if (pixel(x, y) == 0xf5ed48) {
                    const int traffic_x = static_cast<int>(x) + 4;
                    if (player_x > traffic_x - 30 && player_x < traffic_x + 30) {
                        if (traffic_x < 85) avoidance = right_button;
                        else if (traffic_x > 235) avoidance = left_button;
                        else avoidance = player_x < traffic_x ? left_button : right_button;
                    }
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        buttons[0] = avoidance | speed_buttons;
        retro_run();
    };
    steering_axis = 0; buttons[0] = 0;
    retro_reset(); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (unsigned n = 0; n < 245; ++n) avoid_traffic();
    CHECK(transitions() >= 4 && transitions() <= 5);
    for (unsigned n = 0; n < 120; ++n) avoid_traffic();
    CHECK(transitions() >= 6 && transitions() <= 7);
    for (unsigned n = 0; n < 124; ++n) avoid_traffic(1u << RETRO_DEVICE_ID_JOYPAD_R2);
    CHECK(transitions() >= 12 && transitions() <= 14);
    for (unsigned n = 0; n < 30; ++n) avoid_traffic();
    CHECK(transitions() >= 12 && transitions() <= 14);
    for (unsigned n = 0; n < 30; ++n) avoid_traffic(1u << RETRO_DEVICE_ID_JOYPAD_L2);
    CHECK(transitions() >= 11 && transitions() <= 13);
    for (unsigned n = 0; n < 94; ++n) avoid_traffic();
    CHECK(transitions() >= 6 && transitions() <= 7);
    buttons[0] = 0;
    press(RETRO_DEVICE_ID_JOYPAD_START);
    CHECK(audio_frame[0] == 0);
    steering_axis = 0; buttons[0] = 0;
    retro_reset(); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (unsigned n = 0; n < 245; ++n) avoid_traffic();
    unsigned until_bonus = 0;
    while (until_bonus++ < 3500) {
        avoid_traffic(1u << RETRO_DEVICE_ID_JOYPAD_R2);
        if (transitions() >= 15 && transitions() <= 17) break;
    }
    CHECK(until_bonus <= 3500);
    std::vector<std::int16_t> bonus_audio = audio_frame;
    for (unsigned n = 1; n < 79; ++n) {
        avoid_traffic(1u << RETRO_DEVICE_ID_JOYPAD_R2);
        bonus_audio.insert(bonus_audio.end(), audio_frame.begin(), audio_frame.end());
    }
    CHECK(bonus_audio.size() == 79 * 800 * 2);
    const unsigned pitches[] = {475, 627, 874, 475, 627, 874, 475, 627, 874};
    for (unsigned tone = 0; tone < 9; ++tone) {
        const unsigned start = tone * 6336;
        const unsigned end = tone == 8 ? 8 * 6336 + 12288 : start + 6336;
        unsigned changes = 0;
        for (unsigned i = start + 1; i < end; ++i)
            changes += bonus_audio[2 * i] != bonus_audio[2 * (i - 1)];
        const unsigned expected = 2 * pitches[tone] * (end - start) / 48000;

        CHECK(changes + 2 >= expected && changes <= expected + 2);
    }
    avoid_traffic(1u << RETRO_DEVICE_ID_JOYPAD_R2);
    CHECK(transitions() <= 14);
    buttons[0] = 0;
    retro_reset(); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START);
    for (unsigned n = 0; n < 3850; ++n) retro_run();
    const auto game_over = frame;
    std::array<bool, 54 * 33> final_score_pixels{};
    unsigned score_ink = 0;
    for (unsigned y = 0; y < 33; ++y) {
        for (unsigned x = 0; x < 54; ++x) {
            const bool ink = pixel(52 + x, 6 + y) == 0xffffff;
            final_score_pixels[y * 54 + x] = ink;
            score_ink += ink;
        }
    }
    CHECK(score_ink > 0);
    for (unsigned n = 0; n < 5; ++n) retro_run();
    CHECK(frame != game_over);
    for (unsigned y = 0; y < 33; ++y)
        for (unsigned x = 0; x < 54; ++x)
            CHECK((pixel(52 + x, 6 + y) == 0xffffff) == final_score_pixels[y * 54 + x]);

    buttons[0] = 0;
    retro_reset(); retro_run();
    press(RETRO_DEVICE_ID_JOYPAD_START);
    buttons[0] = left_button;
    bool seen_top = false, seen_bottom = false;
    for (unsigned n = 0; n < 500; ++n) {
        retro_run();
        for (unsigned x = 40; x < 280; ++x) {
            CHECK(pixel(x, 8) != 0xf5ed48 && pixel(x, 224) != 0xf5ed48);
            seen_top |= pixel(x, 9) == 0xf5ed48;
            seen_bottom |= pixel(x, 223) == 0xf5ed48;
        }
    }
    CHECK(seen_top && seen_bottom);
    buttons[0] = 0;
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
    if (!test_racing()) {
        retro_unload_game();
        retro_deinit();
        return 1;
    }
    std::printf("variant 15 passed\n");
    retro_deinit();
    return 0;
}
