#include <dlfcn.h>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    void *core = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!core) {
        std::cerr << dlerror() << '\n';
        return 1;
    }
    using ApiVersion = unsigned (*)();
    auto api_version = reinterpret_cast<ApiVersion>(dlsym(core, "retro_api_version"));
    const bool ok = api_version && api_version() == 1;
    dlclose(core);
    return ok ? 0 : 1;
}
