# iOS Feasibility Spike

Date: 2026-05-27

## Scope

This is an audit-only spike for compiling the current portable BennuGD/SorR runtime toward iOS. No iOS implementation was started, no asset trimming was done, and the known-good desktop proof was not modified.

Prepared desktop baseline remains:

- `build-portable/bgdi.exe` launches.
- `sorr-vita-master/data/SorR.dat` boots.
- First scene renders and the SDL window remains responsive.
- Keyboard controls work through the known-good save state.

## Verdict

Minimal iOS app shell is feasible as the first milestone.

Running `SorR.dat` on real iOS is blocked by 64-bit pointer handling in the Bennu runtime and modules. The current successful desktop proof is effectively a 32-bit-compatible runtime path: the VM stack, system-function ABI, file handles, memory handles, directory handles, audio handles, and script pointer values are frequently represented as `int` or `uint32_t`. iOS is 64-bit-only, so these paths will truncate host pointers and are not safe for a real game-data launch until the pointer model is fixed or isolated behind handles.

Recommended path:

1. Build an iOS app shell that reaches `main()`/runtime entry and initializes SDL, without loading game data.
2. Fix or isolate pointer-through-int paths.
3. Bundle prepared data and attempt `SorR.dat` load/render after pointer remediation.

## Audit Commands And Artifacts

Generated scan artifacts:

- `reports/ios_pointer_int_rg_scan.txt`
- `reports/ios_platform_assumptions_rg_scan.txt`

Additional targeted audit commands used:

```powershell
rg -n "TYPE_POINTER|int \* stack|typedef int SYSFUNC|\( uint32_t \) &|\( void \* \)params|SDL_CreateWindow|Mix_OpenAudio|file_open|getfullpath|SDL_GetKeyboardState|SDL_NumJoysticks" sorr-vita-master/core sorr-vita-master/modules sorr-vita-master/cmake
Get-ChildItem -Path "sorr-vita-master/data" -Recurse -File | Group-Object { $_.Extension.ToLowerInvariant() }
```

Data extension inventory from the prepared data folder:

| Extension | Count | iOS implication |
| --- | ---: | --- |
| `.pal` | 1308 | Internal palette/resource data. |
| `.fpg` | 349 | Core Bennu graphic packs. |
| `.wav` | 258 | SDL_mixer WAV support needed. |
| `.ogg` | 237 | OGG/Vorbis support needed. |
| `.smk` | 113 | Cutscene/video path needs separate review. |
| `.txt` | 102 | Script/config/read-only data. |
| `.dll` | 22 | PC leftover files; should not be loaded on monolithic iOS. |
| `.png` | 3 | libpng/zlib needed. |
| `.sor` | 3 | Save/unlock data; writable-path issue. |
| `.cfg` | 1 | Config/control file; writable-path issue if mutated. |
| `.dat` | 1 | Main DCB/game data. |

## Primary Blocker: 32-Bit Pointer Model

The current runtime stores VM values in 32-bit `int` slots:

- `core/include/instance_st.h:48` has `int * code`.
- `core/include/instance_st.h:91` and `:92` have `int * stack` and `int * stack_ptr`.
- `core/include/i_procdef_st.h:73` defines system calls as `typedef int SYSFUNC (INSTANCE *, int *)`.
- `core/include/typedef_st.h:50` defines `TYPE_POINTER`.
- `core/include/bgddl.h:88-90` stores system-function return type plus a `void *func`, but the call ABI still passes `int *params`.

The interpreter pushes real host addresses into 32-bit stack slots:

- `core/bgdrtm/src/interpreter.c:472` casts `&PRIDWORD(...)` to `uint32_t`.
- `core/bgdrtm/src/interpreter.c:484` casts `&PUBDWORD(...)` to `uint32_t`.
- `core/bgdrtm/src/interpreter.c:496` casts `&LOCDWORD(...)` to `uint32_t`.
- `core/bgdrtm/src/interpreter.c:508` casts `&GLODWORD(...)` to `uint32_t`.
- `core/bgdrtm/src/interpreter.c:527` and `:546` write instance variable addresses into the stack as `uint32_t`.

The interpreter later treats those 32-bit stack values as pointers:

- `core/bgdrtm/src/interpreter.c:611` dereferences `r->stack_ptr[-1]` as `int32_t *`.
- `core/bgdrtm/src/interpreter.c:674` dereferences a string pointer.
- `core/bgdrtm/src/interpreter.c:1339-1357` writes through pointer values stored on the stack.
- `core/bgdrtm/src/interpreter.c:1438` converts a stack slot to `void **` for `POINTER2STR`.
- `core/bgdrtm/src/interpreter.c:1491-1614` has many assignment/math operations through `int32_t *` or `uint32_t *` addresses stored in the stack.

This is the main reason not to try a full `SorR.dat` iOS run immediately.

## Pointer/Int Cast Issue Catalog

### VM and DCB data model

- `TYPE_POINTER` is serialized as a 32-bit value in `core/bgdrtm/src/varspace_file.c:199` and `:291`.
- `core/bgdrtm/src/sysprocs.c:200` treats `TYPE_POINTER` like `TYPE_STRING`/`TYPE_DWORD` sizing.
- `core/include/dlvaracc.h` uses fixup offsets through casts to `uint32_t`; this is probably an offset rather than a host pointer, but it should be made explicit with `uintptr_t` or a true offset type before an iOS port.

### System function ABI

All exported module functions receive `int *params` and return `int`. Pointer arguments use the `P` param signature but still arrive as `int`.

Examples:

- `modules/mod_mem/mod_mem.c:217`, `:222`, `:227` return `calloc`, `malloc`, and `realloc` pointers as `int`.
- `modules/mod_mem/mod_mem.c:174-191` casts `params[n]` to `void *` for `memcmp`, `memmove`, `memcpy`, and `memset`.
- `modules/mod_file/mod_file.c:112` casts a `file *` to `int`.
- `modules/mod_file/mod_file.c:120-167` casts `params[0]` back to `file *` for close/read/write/seek/tell/flush/size.
- `modules/mod_file/mod_file.c:126`, `:131`, `:136`, `:141` cast params to both `file *` and `void *`.
- `modules/mod_dir/mod_dir.c:265` and `:278` cast `params[0]` back to `__DIR_ST *`.
- `modules/libbgload/bgload.c:51` stores `params[1]` as an `int *` status pointer for background loading.
- `modules/mod_sound/mod_sound.c:289` and `:547` return `Mix_Music *` / `Mix_Chunk *` as `int`.
- `modules/mod_sound/mod_sound.c:1684`, `:1686`, `:1698`, and `:1700` expose pointer-taking overloads in the exported ABI.
- `modules/mod_flic/mod_flic.c:716-733` uses FLIC pointers passed through script params.

### Practical risk

On iOS arm64, any host pointer outside the low 32-bit address range will be truncated. The first broken path could be file handles, memory buffers, sound handles, background-load status pointers, script variable references, or FLIC handles. This is not a one-line compiler warning cleanup; it is a runtime ABI problem.

## iOS Pointer Remediation Options

### Option A: Handle table for host objects

Keep Bennu VM values as 32-bit script values, but never expose raw host pointers to scripts. Return small integer handles for host objects and map them to real pointers in native tables.

Best fit for:

- `file *`
- `Mix_Music *`
- `Mix_Chunk *`
- `__DIR_ST *`
- `FLIC *`
- Background-load records

Limitations:

- Does not solve interpreter stack addresses for script variable references.
- Does not solve generic `MEM_ALLOC`/`MEM_FREE` if the script expects raw pointer arithmetic.

### Option B: Widen VM stack and params to `intptr_t`/`uintptr_t`

Change VM stack slots and sysproc params from `int` to pointer-width integer types, while preserving 32-bit DCB numeric semantics at load/decode boundaries.

Best fit for:

- Interpreter address stack operations.
- Generic pointer params.
- `TYPE_POINTER` runtime behavior.

Limitations:

- Larger invasive change.
- Needs careful DCB compatibility testing.
- Save/load of `TYPE_POINTER` should not persist raw host pointers.

### Option C: Hybrid

Use pointer-width VM internals for interpreter address operations, and use handle tables for long-lived external objects returned to scripts.

This is the recommended eventual path if `SorR.dat` really exercises pointer-heavy sysprocs. It is larger than a shell milestone but safer than hoping low addresses hide truncation.

## Windows/MinGW-Only Code Audit

The original Vita CMake remains untouched. The portable CMake is desktop oriented:

- `sorr-vita-master/cmake/portable/CMakeLists.txt:17-20` uses desktop `find_path`/`find_library` for SDL2 and SDL2_mixer.
- `sorr-vita-master/cmake/portable/CMakeLists.txt:171-175` suppresses warnings including `int-conversion`, which is a red flag for iOS rather than a solution.
- `sorr-vita-master/cmake/portable/CMakeLists.txt:227-228` has a MinGW-only link option guarded by `if(MINGW)`.
- `sorr-vita-master/cmake/common_defs.cmake:1` sets `__MONOLITHIC__`, which is good for iOS because it avoids runtime DLL/dlopen loading.
- `sorr-vita-master/cmake/common_defs.cmake:14-19` defines only Linux/Vita target flags; there is no portable `TARGET_IOS` build file yet.
- `sorr-vita-master/cmake/compiler_flags.cmake:8-9` uses GNU/Clang C flags that are probably acceptable under AppleClang, but the iOS target should own its flags instead of inheriting desktop assumptions blindly.

Windows-specific runtime code is mostly guarded:

- `modules/libvideo/g_video.c:45`, `:179`, `:196`, `:225`, `:517`, and `:537` wrap DirectDraw/DDRAW usage in `_WIN32`.
- `core/include/loadlib.h:43-56` uses `dlopen` only when not monolithic; the current portable build defines `__MONOLITHIC__`.
- `core/common/files.c:997-1001` has a Windows `GetFullPathName` branch, but the non-Windows `realpath` call is commented out. This matters for iOS path setup.

## DLL And Dynamic Loading Assumptions

The prepared data folder still contains 22 `.dll` files, but the current portable runtime is monolithic. For iOS:

- Keep `__MONOLITHIC__`.
- Do not attempt to load PC `.dll` files.
- Keep modules linked statically into the iOS app.
- Treat `.dll` files in data as ignored PC leftovers unless a file-open log proves the game script expects to inspect them.

## Relative Filesystem And Working Directory Assumptions

Current desktop proof depends on launching from:

```text
sorr-vita-master/data
```

with:

```text
../../build-portable/bgdi.exe SorR.dat
```

Important code paths:

- `core/bgdi/src/main.c:103-162` derives executable path from `argv[0]` and adds it via `file_addp`.
- `core/bgdi/src/main.c:205-208` has a Vita-specific search path and hardcoded data filename branch.
- `core/bgdi/src/main.c:213-235` handles desktop command-line args and `-p` search paths.
- `core/common/files.c:784` is the central `file_open`.
- `core/common/files.c:866-870` tries `possible_paths`.
- `core/common/files.c:902-918` appends search paths.
- `core/common/files.c:993-1002` currently returns useful full paths only on Windows.
- `modules/mod_dir/mod_dir.c:123` exposes `CHDIR` to scripts.

iOS cannot rely on a mutable current directory inside the app bundle. Reads and writes need explicit routing.

## Savegame Writable Path Assumptions

Known-good desktop requires:

- `data/savegame/savegame.sor`
- `data/xbox/xbox.cfg`
- `data/mod/system.txt`

The app bundle should be treated as read-only. Writable files should live under:

```text
Library/Application Support/SORR/
```

or, if user-visible file sharing is wanted later:

```text
Documents/SORR/
```

Recommended first writable layout:

```text
Library/Application Support/SORR/savegame/savegame.sor
Library/Application Support/SORR/savegame/completed.sor
Library/Application Support/SORR/savegame/trophies.sor
Library/Application Support/SORR/xbox/xbox.cfg
```

Read lookup order should be:

1. Writable support root, so saves/config overrides win.
2. App bundle resource root, for read-only game assets.

Write/delete/move operations should always resolve to the writable support root unless explicitly blocked.

## SDL Window/Event Assumptions

Video:

- `modules/libvideo/g_video.c:344-356` creates a desktop-style SDL window with `SDL_WINDOW_SHOWN`, `SDL_WINDOW_BORDERLESS`, `SDL_WINDOW_FULLSCREEN_DESKTOP`, and `SDL_WINDOW_INPUT_GRABBED`.
- `modules/libvideo/g_video.c:500-501` already has a `TARGET_ANDROID || TARGET_IOS` branch that calls `gr_set_mode(0, 0, 0)`.
- `modules/libvideo/g_video.c:514` initializes the SDL video subsystem.

Events:

- `modules/libsdlhandler/libsdlhandler.c:55-58` now pumps SDL events in the portable proof.
- `modules/libkey/libkey.c:462` consumes SDL keydown/keyup events.
- `modules/libkey/libkey.c:589` reads `SDL_GetKeyboardState`.
- `modules/libjoy/libjoy.c:535` detects joysticks with `SDL_NumJoysticks`.
- `modules/mod_multi/mod_multi.c:118-179` already consumes SDL finger events, but it is script-facing multi-touch support, not the desired keyboard bridge.

iOS implications:

- SDL iOS integration may require using SDL's expected `main`/app delegate flow. `SDL_MAIN_HANDLED` should be revisited for the iOS target.
- Real keyboard state is not available for touch controls, so the first input path should not depend on `SDL_GetKeyboardState` changing.
- Avoid joystick/gamepad emulation for the first iOS bridge; the desktop proof succeeded by making Player 1 keyboard-driven.
- Window flags should be normalized for iOS fullscreen/orientation behavior.

## Audio Dependency Assumptions

The runtime uses SDL_mixer:

- `modules/mod_sound/mod_sound.c:205-219` calls `Mix_OpenAudio`.
- `modules/mod_sound/mod_sound.c:287` loads music with `Mix_LoadMUS_RW`.
- `modules/mod_sound/mod_sound.c:545` loads WAV with `Mix_LoadWAV_RW`.
- `modules/mod_sound/mod_sound.c:246` closes with `Mix_CloseAudio`.

The data inventory has 258 `.wav` and 237 `.ogg` files. Minimum iOS audio dependencies:

- SDL2 for iOS.
- SDL2_mixer for iOS, statically linked or embedded as an iOS framework.
- libogg.
- libvorbis.
- libvorbisfile.

Likely not needed for the first shell milestone:

- FLAC.
- MP3/mpg123.
- MOD/MikMod.

Cutscene/video note:

- The prepared data has 113 `.smk` files, but the current runtime source includes `mod_flic`, not a clearly identified SMK decoder dependency in this pass. Treat video playback as a separate follow-up after title/city render.

## iOS Dependency Plan

First iOS target should be a separate build file, not a modification to the Vita CMake.

Recommended dependency shape:

```text
sorr-vita-master/cmake/ios/CMakeLists.txt
```

or an Xcode project that reuses the same source list from:

```text
sorr-vita-master/cmake/portable/CMakeLists.txt
```

Dependencies:

- SDL2 iOS: build from SDL2 source as static library/framework, or consume the official iOS Xcode project output.
- SDL2_mixer iOS: build against the same SDL2, with OGG/Vorbis enabled.
- zlib: use iOS SDK `libz.tbd` if compatible, otherwise vendored static zlib.
- libpng: vendored/static iOS build linked against zlib.
- libogg/libvorbis/libvorbisfile: vendored/static iOS builds.
- tre: existing `3rdparty/tre` can remain static if it compiles under AppleClang/iOS.
- math: replace desktop `m` link assumptions with whatever the iOS toolchain needs; many libm symbols are in system libraries on Apple platforms.

Build definitions to preserve or add:

- Preserve `__MONOLITHIC__`.
- Add `TARGET_IOS`.
- Keep `NO_MODCHIPMUNK`, `NO_MODICONV`, `NO_MODFMODEX`, `NO_MODCURL`, `NO_MODMATHI`, `NO_MODSENSOR`, `NO_FSOCK`, `NO_MODIAP`, and `NO_MODIMAGE` unless a missing symbol proves otherwise.
- Do not carry `-Wno-error=int-conversion` into iOS as a blanket solution; use it only as a temporary diagnostic escape hatch if the shell build is blocked.

## iOS Data Layout Plan

Read-only app bundle:

```text
SorR.app/
  SorR.dat
  data/
  mod/
  palettes/
  music/
  sfx/
  video/
```

The exact subfolder names should follow the prepared `sorr-vita-master/data` layout. Do not flatten paths because the game expects relative paths.

Writable support root:

```text
Library/Application Support/SORR/
  savegame/
  xbox/
  logs/
```

Startup shim strategy:

1. Resolve bundle resource root.
2. Resolve/create Application Support root.
3. Copy seed writable files from bundle to Application Support only if missing:
   - `savegame/savegame.sor`
   - any required `completed.sor`/`trophies.sor`/unlock files.
   - `xbox/xbox.cfg` only if the script writes to it.
4. Add support root first in `possible_paths`.
5. Add bundle resource root second in `possible_paths`.
6. For writes, force `file_open`/remove/move/mkdir/rmdir to the support root.
7. Either avoid `chdir`, or `chdir` to support root and rely on explicit read search paths for bundle assets.

Do not copy all assets into the writable directory. It will waste storage and complicate updates.

## iOS Input Bridge Design

Use the confirmed desktop keyboard mapping. Do not emulate a gamepad first.

| Touch control | Emulated key |
| --- | --- |
| D-pad Up | Arrow Up / Bennu key `72` |
| D-pad Down | Arrow Down / Bennu key `80` |
| D-pad Left | Arrow Left / Bennu key `75` |
| D-pad Right | Arrow Right / Bennu key `77` |
| Attack | `C` / Bennu key `46` |
| Jump | `V` / Bennu key `47` |
| Special | `X` / Bennu key `45` |
| Police | `B` / Bennu key `48` |
| Start/Pause | Enter / Bennu key `28` |
| Back/Menu | Escape `1`, with Backspace `14` fallback |

Touch layout:

```text
Left side:
  D-pad

Right-side diamond:
          Special
  Attack          Police
           Jump

Top/right small:
  Start/Pause
```

Preferred injection point:

- Add a small virtual key state layer under `modules/libkey`.
- Merge virtual key state inside `modules/mod_key/mod_key.c::_get_key`.
- Keep it behind `TARGET_IOS` or `PORTABLE_INPUT_BRIDGE`.

Avoid first:

- SDL synthetic key events, because the game reads held key state through Bennu/SDL state paths.
- Mutating SDL's `SDL_GetKeyboardState` buffer directly.
- Joystick/gamepad emulation, because the known-good path is keyboard-driven.

## First Implementation Milestone

Goal: iOS app reaches runtime main and SDL initializes.

Scope:

- Add a new iOS build target/build file.
- Compile only enough runtime code to enter `main()` or a minimal SDL-compatible app entrypoint.
- Initialize SDL video.
- Create a blank fullscreen SDL surface/window/renderer if practical.
- Do not load `SorR.dat`.
- Do not bundle game data yet.
- Log basic lifecycle:
  - app entry
  - SDL init begin/end
  - video init begin/end
  - clean shutdown or idle loop

Success criteria:

- Builds for iOS simulator arm64 or device arm64.
- Launches as an iOS app.
- Reaches runtime entry logging.
- SDL initializes without app delegate/main conflicts.
- No pointer-through-int runtime path is exercised beyond safe initialization.

## Second Implementation Milestone

Goal: bundle prepared data, load `SorR.dat`, and render title/city scene.

Prerequisites:

- Pointer/int remediation strategy implemented enough for interpreter variable references and sysproc handles.
- iOS read/write path shim in place.
- SDL_mixer with OGG/Vorbis and WAV support linked.
- Keyboard-style touch bridge stubbed or testable.

Scope:

- Bundle prepared `sorr-vita-master/data` as read-only app resources.
- Seed known-good writable files into Application Support.
- Add support-root and bundle-root search paths.
- Launch runtime against `SorR.dat`.
- Render title/city scene.
- Confirm event pumping keeps app responsive.

Success criteria:

- `SorR.dat` opens.
- Window/screen renders.
- No crash from truncated file/audio/video/memory handles.
- Touch bridge can send Start/Confirm and basic navigation keys.

## Current Blocker

The exact current blocker for full game-data iOS launch is pointer truncation caused by the 32-bit VM/sysproc ABI. The first iOS shell can proceed without solving it, but the second milestone should not attempt full `SorR.dat` execution until the pointer plan is implemented or at least guarded with targeted handle tables and runtime diagnostics.

