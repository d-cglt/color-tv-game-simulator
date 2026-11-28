#include "core_test_harness.h"

int main() {
    core_test::ContentContext context;
    core_test::MenuSnapshot snapshot;
    // Keep the original scenario order and frontend/core lifecycle transitions.
    if (int result = core_test::boot_generator_scenarios(context)) return result;
    if (int result = core_test::demo_preview_scenarios(context)) return result;
    if (int result = core_test::gameplay_rendering_scenarios(context, snapshot)) return result;
    if (int result = core_test::menus_options_scenarios(context, snapshot)) return result;
    if (int result = core_test::persistence_selector_demo_scenarios(context)) return result;
    if (int result = core_test::video_rumble_scenarios(context)) return result;
    if (int result = core_test::direct_launch_lock_scenarios(context)) return result;
    return 0;
}
