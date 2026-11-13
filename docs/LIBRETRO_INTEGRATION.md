# Libretro integration preparation

Status checked 2026-11-13. This is preparation, not a claim of acceptance or Core Downloader availability. Do not treat older roadmap release-preparation entries as current release status.

## Verified project state

- CMake identifies the core as 0.1.4. The public [v0.1.4 release](https://github.com/d-cglt/color-tv-game-simulator/releases/tag/v0.1.4) contains Linux x86-64, Windows x86-64, macOS Intel and CTG selector archives. Racing is a separate one-player wide-road prototype in addition to 15 playable 6/15 variants, not 112 implemented combinations.
- The local `colortvgame_libretro.info` matches the `colortvgame_libretro` build target and declares `ctg`, no-content support and MIT. The core reads selectors from memory and does not expose libretro save memory or save states. Separately persisted preferences and last selection use the frontend save directory. The user confirmed the revised Core Information screen in RetroArch: version 0.1.4, name, category, MIT, CTG extension, no save-state support and metadata notes were visible. The user subsequently confirmed the split preference notes are fully visible at the observed screen width. The description and every individual capability flag were not visually confirmed.
- Linux has prior RetroArch testing. [0.1.4 GitHub Actions run 43240009525](https://github.com/d-cglt/color-tv-game-simulator/actions/runs/43240009525) passed automated builds/tests on Linux x86-64, Windows x86-64 and macOS Intel; the latter two remain frontend-unverified. GitHub Actions is not the Libretro buildbot.
- The [integration issue](https://github.com/libretro/libretro-super/issues/2195) records a positive maintainer response to the licensing clarification and links to the integration guide and Clownacy's experience. Acceptance and availability are not confirmed.

## Candidate upstream changes

1. Submit the reviewed `colortvgame_libretro.info` to `libretro-super/dist/info` after approval. The revised Core Information screen and split preference notes were checked in RetroArch; the description and all individual capability flags have not been separately verified.
2. Request maintainer review of the draft `.gitlab-ci.yml`, repository mirroring or buildbot registration, and an initial Linux x86-64 trial. Windows and macOS Intel have existing GitHub CI coverage, but no Libretro buildbot jobs are proposed until maintainers advise on targets, toolchains and runtime baselines. Additional architectures are not assumed.
3. Adapt `LIBRETRO_CORE.md` for `libretro-docs`. Its [contribution guide](https://github.com/libretro/docs#adding-a-new-core) asks for a `docs/library` page, navigation, core list and license updates, with other entries conditional on features. The draft follows the relevant [core template](https://github.com/libretro/docs/blob/master/docs/meta/core-template.md) sections and uses project URLs for supporting references; review its upstream placement and navigation before submission. No upstream files have been modified.
4. Discuss whether a CTG playlist database and icons are worthwhile. The [official core guide](https://docs.libretro.com/development/cores/developing-cores/#add-your-core-to-libretro-infrastructure) discusses those separately for playlist compatibility, not as stated Core Downloader prerequisites. Database matching and artwork for these custom selectors are not implemented or verified.

## Draft buildbot assumptions and questions

The `.gitlab-ci.yml` draft includes Libretro's [Linux CMake template](https://git.libretro.com/libretro-infrastructure/ci-templates/-/raw/master/linux-cmake.yml), extends `.libretro-linux-cmake-x86_64` and sets `CORENAME=colortvgame`. The template invokes the CMake target `${CORENAME}_libretro`, expects `build/linux-$ARCH/$EXTRA_PATH/${CORENAME}_libretro.so`, moves it to the repository root, and publishes it as a short-lived CI artifact. The local CMake target and output name were verified on the local Linux host; the template's image, toolchain, artifact collection and updater publication have not been run or verified. [Dolphin's configuration](https://raw.githubusercontent.com/libretro/dolphin/master/.gitlab-ci.yml) is an example of a core extending this CMake template; it is not evidence that this project builds on that runner.

- Does the template's Linux image provide CMake 3.16+ and a C++17 compiler, including suitable `std::filesystem` support, with an acceptable target runtime baseline? If not, which approved image or toolchain should be used?
- Does the runner provide at least five visible CPUs for the template's `nproc / 5` calculation? With fewer, GNU Make rejects the resulting `-j 0`. Is the resulting library path accepted by the publishing infrastructure?
- Is Linux x86-64 an appropriate first target? Which Windows and macOS templates and runtime baselines should be added, and by whom? Is CTest wanted in buildbot jobs?
- Who creates and maintains the Libretro GitLab mirror or buildbot registration, and can a maintainer run CI lint and a trial pipeline there? [Clownacy's account](https://clownacy.wordpress.com/2025/08/24/how-i-added-a-new-libretro-core-to-retroarch/) describes maintainer-assisted mirroring for another core; it is not a guarantee of the present process.


No external repository changes, pull requests or comments are authorized by this preparation.
