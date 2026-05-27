# 64-bit Desktop Feasibility

Date: 2026-05-27

## Goal

Create an isolated 64-bit Windows desktop build of the portable BennuGD runtime so it exposes the same pointer/int ABI problems expected on iOS.

The known-good 32-bit desktop build was left untouched:

- `build-portable/`
- `portable-tools/w64devkit/`
- `portable-deps/msys2-mingw32/`
- `sorr-vita-master/data/`

## Build Area

- Build directory: `build-portable-x64`
- Toolchain: `portable-tools/w64devkit-x64`
- Dependency prefix: `portable-deps/msys2-mingw64/mingw64`
- Source CMake file: `sorr-vita-master/cmake/portable/CMakeLists.txt`
- No changes were made to the portable CMake file for this probe.

## Toolchain Setup

Existing bundled compiler was 32-bit only:

```powershell
.\portable-tools\w64devkit\bin\gcc.exe -dumpmachine
# i686-w64-mingw32
```

Downloaded and extracted x64 w64devkit:

```powershell
Invoke-WebRequest `
  -Uri https://github.com/skeeto/w64devkit/releases/download/v2.8.0/w64devkit-x64-2.8.0.7z.exe `
  -OutFile portable-tools\w64devkit-x64-2.8.0.7z.exe

.\portable-tools\w64devkit-x64-2.8.0.7z.exe -y -oportable-tools\w64devkit-x64-extract
Move-Item portable-tools\w64devkit-x64-extract\w64devkit portable-tools\w64devkit-x64

.\portable-tools\w64devkit-x64\bin\gcc.exe -dumpmachine
# x86_64-w64-mingw32
```

Verified compiler:

```text
gcc.exe (GCC) 16.1.0
```

## Dependency Setup

The existing 32-bit dependency packages were mirrored with `mingw-w64-x86_64-*` MSYS2 packages into:

- `portable-deps/packages-x64`
- `portable-deps/msys2-mingw64`

Packages extracted:

- `mingw-w64-x86_64-zlib-1.3.2-2`
- `mingw-w64-x86_64-libpng-1.6.58-1`
- `mingw-w64-x86_64-SDL2-2.32.10-1`
- `mingw-w64-x86_64-SDL2_mixer-2.8.2-1`
- `mingw-w64-x86_64-flac-1.5.0-1`
- `mingw-w64-x86_64-mpg123-1.33.5-1`
- `mingw-w64-x86_64-opusfile-0.12-4`
- `mingw-w64-x86_64-libvorbis-1.3.7-2`
- `mingw-w64-x86_64-wavpack-5.9.0-1`
- `mingw-w64-x86_64-libxmp-4.7.0-1`
- `mingw-w64-x86_64-libogg-1.3.6-1`
- `mingw-w64-x86_64-opus-1.6.1-1`
- `mingw-w64-x86_64-gcc-libs-16.1.0-5`
- `mingw-w64-x86_64-winpthreads-14.0.0.r47.g0636d42e1-1`
- `mingw-w64-x86_64-libwinpthread-14.0.0.r47.g0636d42e1-1`
- `mingw-w64-x86_64-winpthreads-git-12.0.0.r747.g1a99f8514-1`

Extraction note:

- `tar --zstd` failed because BusyBox `tar` looked for `unzstd`.
- Workaround used: decompress with `zstd.exe` to `.tar`, then extract with `tar.exe`.
- Full setup log: `reports/x64_dependency_setup.log`

## Configure Command

```powershell
$root = (Get-Location).Path
$env:PATH = (Join-Path $root 'portable-tools\w64devkit-x64\bin') + ';' +
            (Join-Path $root 'portable-deps\msys2-mingw64\mingw64\bin') + ';' +
            $env:PATH

$cmake = Join-Path $root 'portable-tools\w64devkit-x64\bin\cmake.exe'
$cc = Join-Path $root 'portable-tools\w64devkit-x64\bin\gcc.exe'
$src = Join-Path $root 'sorr-vita-master\cmake\portable'
$build = Join-Path $root 'build-portable-x64'
$prefix = Join-Path $root 'portable-deps\msys2-mingw64\mingw64'

& $cmake -S $src -B $build -G Ninja `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  "-DCMAKE_C_COMPILER=$cc" `
  "-DCMAKE_PREFIX_PATH=$prefix" `
  "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
```

Result:

- Configure succeeded.
- ZLIB found from `portable-deps/msys2-mingw64/mingw64/lib/libz.dll.a`.
- PNG found from `portable-deps/msys2-mingw64/mingw64/lib/libpng.dll.a`.
- Only configure warning was TRE's old `cmake_minimum_required`.
- Full configure log: `reports/x64_configure.log`

## Build Command

```powershell
$root = (Get-Location).Path
$env:PATH = (Join-Path $root 'portable-tools\w64devkit-x64\bin') + ';' +
            (Join-Path $root 'portable-deps\msys2-mingw64\mingw64\bin') + ';' +
            $env:PATH

$cmake = Join-Path $root 'portable-tools\w64devkit-x64\bin\cmake.exe'
$build = Join-Path $root 'build-portable-x64'

& $cmake --build $build --verbose
```

Result:

- Build succeeded.
- Output executable: `build-portable-x64/bgdi.exe`
- Link succeeded.
- No linker errors.
- Full raw build log: `reports/x64_build.log`
- Null-cleaned build log: `reports/x64_build_clean.log`

## Compiler Diagnostics

Build result:

```text
BUILD_EXIT=0
```

Warning summary from `reports/x64_build_clean.log`:

```text
total warning records: 674
pointer-to-int warnings: 357
int-to-pointer warnings: 271
matching diagnostic lines: 752
```

Top warning locations by file:

```text
310  sorr-vita-master/core/include/dlvaracc.h
 93  sorr-vita-master/core/bgdrtm/src/interpreter.c
 36  sorr-vita-master/modules/mod_map/mod_map.c
 33  sorr-vita-master/modules/mod_flic/mod_flic.c
 25  sorr-vita-master/modules/mod_file/mod_file.c
 16  sorr-vita-master/3rdparty/tre/lib/tre-internal.h
 14  sorr-vita-master/modules/mod_mem/mod_mem.c
 13  sorr-vita-master/modules/mod_sort/mod_sort.c
 12  sorr-vita-master/3rdparty/tre/lib/xmalloc.h
 10  sorr-vita-master/modules/mod_blendop/mod_blendop.c
 10  sorr-vita-master/modules/mod_sound/mod_sound.c
 10  sorr-vita-master/modules/librender/g_instance.c
```

Most important warning classes:

```text
357 cast from pointer to integer of different size [-Wpointer-to-int-cast]
271 cast to pointer from integer of different size [-Wint-to-pointer-cast]
 12 'free' called on pointer to an unallocated object [-Wfree-nonheap-object]
  8 taking address of packed member may result in an unaligned pointer value
  7 passing argument 3 of 'gr_new_object' from incompatible pointer type
  7 passing argument 2 of 'gr_new_object' from incompatible pointer type
  2 passing argument 4 of 'gr_new_object' makes pointer from integer without a cast
```

Representative high-risk sites:

```c
// sorr-vita-master/core/bgdrtm/src/interpreter.c
*r->stack_ptr++ = ( uint32_t ) & PRIDWORD( r, ptr[1] );
*r->stack_ptr++ = ( uint32_t ) & PUBDWORD( r, ptr[1] );
*r->stack_ptr++ = ( uint32_t ) & LOCDWORD( r, ptr[1] );
*r->stack_ptr++ = ( uint32_t ) & GLODWORD( ptr[1] );
```

```c
// sorr-vita-master/core/bgdrtm/src/interpreter.c
r->stack_ptr[-1] = *( int32_t * )r->stack_ptr[-1];
```

```c
// sorr-vita-master/core/include/dlvaracc.h
#define LOCADDR(m,a,b) ((uint8_t *)((a)->locdata) + (uint32_t)m##_locals_fixup[b].data_offset)
```

These confirm the VM stack/local/global access paths are still 32-bit-address shaped.

## Launch Probe

To avoid modifying the known-good desktop data folder, launch used a sandbox:

- Working directory: `out/x64-launch-data`
- `SorR.dat`, `savegame`, `xbox`, and top-level text files were copied.
- Large read-only content directories were junctioned:
  - `data`
  - `mod`
  - `palettes`

Launch command:

```powershell
$root = (Get-Location).Path
$env:PATH = (Join-Path $root 'portable-tools\w64devkit-x64\bin') + ';' +
            (Join-Path $root 'portable-deps\msys2-mingw64\mingw64\bin') + ';' +
            $env:PATH

Push-Location out\x64-launch-data
..\..\build-portable-x64\bgdi.exe SorR.dat
Pop-Location
```

Controlled process probe:

```text
t=1s alive responding=True title='SorR.dat' cpu=0.46875 ws=108326912
t=2s exited
```

Direct run result:

```text
exit=-1073741819
```

`-1073741819` is Windows `0xC0000005`, an access violation.

Last runtime log line:

```text
INFO: Called set_mode with 320x240x0
```

Window behavior:

- A window appears briefly.
- Title during probe: `SorR.dat`
- It exits before the known-good title `Streets of Rage Remake - v5.2` is observed.
- No city scene render was verified in the x64 run.

Launch logs:

- `reports/x64_launch_probe.log`
- `reports/x64_launch_direct_probe.log`
- `reports/x64_launch_stdout.log`
- `reports/x64_launch_stderr.log`
- `reports/x64_launch_direct_stdout.log`
- `reports/x64_launch_direct_stderr.log`

## Debugger Crash Point

GDB command:

```powershell
Push-Location out\x64-launch-data
..\..\portable-tools\w64devkit-x64\bin\gdb.exe `
  -q -batch `
  -ex 'set pagination off' `
  -ex 'run SorR.dat' `
  -ex 'bt' `
  -ex 'info registers rip rsp rcx rdx r8 r9' `
  --args ..\..\build-portable-x64\bgdi.exe SorR.dat
Pop-Location
```

Backtrace:

```text
Thread 1 received signal SIGSEGV, Segmentation fault.
0x00007ff8cb36f3ee in msvcrt!_strdup () from C:\windows\System32\msvcrt.dll
#0  0x00007ff8cb36f3ee in msvcrt!_strdup ()
#1  0x00007ff7b00481ea in string_new
    at sorr-vita-master/core/bgdrtm/src/strings.c:437
#2  0x00007ff7b00469d2 in instance_go
    at sorr-vita-master/core/bgdrtm/src/interpreter.c:1463
#3  0x00007ff7b0044ba6 in instance_go
    at sorr-vita-master/core/bgdrtm/src/interpreter.c:390
#4  0x00007ff7b0044ba6 in instance_go
    at sorr-vita-master/core/bgdrtm/src/interpreter.c:390
#5  0x00007ff7b0047638 in instance_go_all
    at sorr-vita-master/core/bgdrtm/src/interpreter.c:170
#6  0x00007ff7b0089c80 in main
    at sorr-vita-master/core/bgdi/src/main.c:363
```

Register evidence:

```text
rcx  0xa00d6fe78
rdx  0x2d559ac
r8   0x0
r9   0x2d569d0
```

Crash source context:

```c
// strings.c:435-437
int string_new( const char * ptr )
{
    char * str = strdup( ptr ) ;
```

```c
// interpreter.c:1461-1463
case MN_A2STR:
    str = *( char ** )( &r->stack_ptr[-ptr[1] - 1] ) ;
    n = string_new( str );
```

The crash is consistent with a 64-bit pointer being reconstructed from a 32-bit VM stack cell or adjacent 32-bit stack cells. `string_new()` receives an invalid pointer and crashes inside `_strdup()`.

Full debugger log:

- `reports/x64_gdb_crash.log`

## Current Blocker

The first actual 64-bit breakage is not compile or link failure. It is runtime pointer truncation/invalid pointer reconstruction in the Bennu interpreter after `set_mode`.

The first confirmed crash path is:

```text
main()
instance_go_all()
instance_go()
MN_A2STR
string_new(str)
strdup(str)
SIGSEGV / 0xC0000005
```

The likely remediation area is the Bennu VM ABI:

- VM stack cell type
- pointer passing through `int`/`uint32_t`
- address opcodes that push pointers into 32-bit stack slots
- fixed/string conversion opcodes such as `MN_A2STR`
- local/global data access macros in `dlvaracc.h`
- module callbacks that pass opaque pointers as `int`

No pointer remediation was attempted in this phase.

## Status

- 64-bit toolchain available: yes
- 64-bit dependencies available: yes
- `build-portable-x64` configured: yes
- `build-portable-x64/bgdi.exe` built: yes
- Compile errors: none
- Linker errors: none
- Pointer/int warnings: many, confirmed
- Runtime reaches `main()`: yes
- Runtime opens `SorR.dat`: yes, enough to reach script execution and `set_mode`
- Runtime initializes video enough to create a brief window: yes
- Runtime reaches known-good title/city render: no
- First actual 64-bit breakage: access violation in `MN_A2STR` / `string_new()` after `set_mode`
