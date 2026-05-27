# x64 First-Crash Analysis

Date: 2026-05-27

Scope:
- x64-only investigation of the first runtime crash.
- Known-good 32-bit desktop state was not rebuilt or modified.
- iOS work was not resumed.
- No broad pointer remediation was started.

## Starting Point

The previous x64 probe built and linked `build-portable-x64/bgdi.exe`, reached `set_mode`, then crashed with `0xC0000005`.

Initial GDB backtrace:

```text
msvcrt!_strdup
string_new() at strings.c
instance_go() at interpreter.c
MN_A2STR
```

## Source Inspection

Runtime stack storage is still 32-bit-cell based:

```c
int * stack;
int * stack_ptr;
```

Address-producing interpreter opcodes write pointers into those 32-bit cells:

```c
MN_PRIVATE -> (uint32_t)&PRIDWORD(...)
MN_PUBLIC  -> (uint32_t)&PUBDWORD(...)
MN_LOCAL   -> (uint32_t)&LOCDWORD(...)
MN_GLOBAL  -> (uint32_t)&GLODWORD(...)
```

`MN_A2STR` originally read a pointer by treating the address of one stack cell as a `char **`:

```c
str = *( char ** )( &r->stack_ptr[-ptr[1] - 1] );
```

That is valid only when a pointer fits in one stack cell. On x64 it reads 8 bytes from a 4-byte cell, combining the intended cell with the next stack entry.

Compiler-side inspection in `core/bgdc/src/c_code.c` shows `MN_A2STR` is emitted for fixed-length `char[]` values used as strings. It expects a pointer to char data in Bennu VM memory, not a Bennu string handle or string ID.

## Instrumentation Added

x64-only diagnostics were added under:

```c
#if defined(PORTABLE_RUNTIME_DIAG) && defined(_WIN64)
```

Files touched:
- `sorr-vita-master/core/bgdrtm/src/interpreter.c`
- `sorr-vita-master/core/bgdrtm/src/strings.c`

Diagnostics added:
- `MN_A2STR` stack slot logging.
- `MN_A2STR` raw cell logging.
- stitched 8-byte pointer logging.
- pointer readability checks with `VirtualQuery`.
- nearby stack cell logging.
- `string_new()` argument logging.
- optional per-opcode step logging via `SORR_PORTABLE_DIAG_STEPS=1`.

Note: these source files are legacy encoded. `apply_patch` could not safely edit them, so byte-preserving PowerShell edits were used with ISO-8859-1 encoding.

## Instrumented Crash Evidence

Command shape:

```powershell
$env:SORR_PORTABLE_DIAG_SCRIPT='1'
$env:SORR_PORTABLE_DIAG_VIDEO='1'
Push-Location out\x64-launch-data
..\..\build-portable-x64\bgdi.exe SorR.dat
Pop-Location
```

GDB/instrumented run showed:

```text
MN_A2STR proc=CARGAR_PARTIDA id=65538 code_offset=641 arg=0 slot=-1 depth=3
raw0=0x025efe78
raw1=0x0000000a
cell_ptr=00000000025efe78 readable=1
stitched_ptr=0000000a025efe78 readable=0
string_new ptr=0000000a025efe78 readable=0
```

Meaning:
- The stack cell contained a plausible pointer-sized value in its low 32 bits.
- The old code read two cells as one 64-bit pointer.
- The high 32 bits came from the next stack value, `0x0000000a`.
- `_strdup()` crashed because `string_new()` received `0x0000000a025efe78`.

## Targeted Fix Attempted

Applied x64-only tactical fix at `MN_A2STR`:

```c
#if defined(_WIN64)
    str = ( char * )( uintptr_t )( uint32_t )r->stack_ptr[-ptr[1] - 1];
#else
    str = *( char ** )( &r->stack_ptr[-ptr[1] - 1] ) ;
#endif
```

Build command:

```powershell
$env:PATH="$pwd\portable-tools\w64devkit-x64\bin;$pwd\portable-deps\msys2-mingw64\mingw64\bin;$env:PATH"
portable-tools\w64devkit-x64\bin\cmake.exe --build build-portable-x64 --verbose
```

Result:
- Build succeeded.
- Existing pointer-width warnings remain, as expected.

## After-Fix Result

Under GDB, where allocations landed under 4 GB, the tactical `MN_A2STR` fix got past repeated `MN_A2STR` conversions in `CARGAR_PARTIDA` and reached later runtime activity:

```text
MN_A2STR proc=CARGAR_PARTIDA id=65538 code_offset=641
MN_A2STR proc=CARGAR_PARTIDA id=65538 code_offset=648
...
INFO: Called set_mode with 416x240x0
MN_FRAME proc=MENU id=65537 frame_percent=100
frame present count=1000
frame present count=2000
frame present count=3000
```

In a normal x64 run, allocations are above 4 GB. The tactical `MN_A2STR` change cannot recover high pointer bits already lost when address opcodes store pointers into `int` stack cells.

With step diagnostics enabled:

```powershell
$env:SORR_PORTABLE_DIAG_STEPS='1'
```

The normal x64 run now fails before reaching `MN_A2STR`:

```text
step=1 proc=MENU id=65537 code_offset=0 opcode=0x00000094 param=192 depth=1 top0=0x00001000
step=2 proc=MENU id=65537 code_offset=2 opcode=0x00000084 param=44100 depth=2 top0=0x3a2291e0
step=3 proc=MENU id=65537 code_offset=4 opcode=0x00000047 param=148 depth=3 top0=0x0000ac44 top1=0x3a2291e0
```

Opcode decode:
- `0x94` = `MN_GLOBAL`
- `0x84` = `MN_PUSH`
- `0x47` = `MN_LETNP`

Crash cause:
- `MN_GLOBAL 192` pushed a truncated global-variable address, `0x3a2291e0`.
- `MN_PUSH 44100` pushed the value.
- `MN_LETNP` tried to write `44100` through `0x3a2291e0`.
- The real pointer was above 4 GB, so the 32-bit stack cell lost the high bits.

Current normal-run exit:

```text
0xC0000005
```

## Candidate Fix Evaluation

A. Widen VM stack slots to `intptr_t` / `uintptr_t`

This is the correct architectural fix. It would allow pointer-carrying opcodes and integer VM values to coexist without truncation, but it is broad and likely touches DCB ABI assumptions, opcode handlers, sysproc boundaries, save/runtime structures, and any code assuming `int * stack`.

B. Patch `MN_A2STR` to use `uintptr_t` for this address path

This fixes the immediate stitched-pointer bug when the pointer value still fits in the low 32 bits. It is useful and exposed the next blocker under GDB, but it is not sufficient for a normal x64 run where VM memory addresses are above 4 GB.

C. Add explicit pointer-width helpers/macros for stack pointer reads/writes

Best next targeted direction. A minimal version could cover:
- address-producing opcodes: `MN_PRIVATE`, `MN_PUBLIC`, `MN_LOCAL`, `MN_GLOBAL`
- direct pointer-consuming opcodes hit early: `MN_LETNP`, then related `LET`, `INC`, `DEC`, `VAR*`, `STR2A`, `A2STR`

This is safer than ad hoc casting but still needs tight scoping to avoid becoming broad remediation.

D. Handle table

Potentially useful if the VM must keep 32-bit cells for DCB compatibility. Address-producing opcodes could store a 32-bit handle, and pointer-consuming opcodes could resolve that handle. This avoids widening every stack cell but requires every pointer consumer to use the resolver. For this first-crash path it would need at least `MN_GLOBAL -> MN_LETNP` and `MN_* -> MN_A2STR`.

## Current Blocker

`MN_A2STR` no longer fails from the original stitched-pointer read under the controlled debugger path. The next normal x64 blocker is earlier in `MENU`:

```text
MN_GLOBAL 192 -> MN_PUSH 44100 -> MN_LETNP
```

`MN_LETNP` dereferences a truncated global-variable pointer from the 32-bit VM stack cell.

## Recommendation

Do not continue with broad pointer remediation yet.

The next smallest useful spike is a pointer-stack helper experiment limited to address-producing opcodes plus the first confirmed consumers:
- `MN_GLOBAL`
- `MN_PRIVATE`
- `MN_PUBLIC`
- `MN_LOCAL`
- `MN_LETNP`
- `MN_A2STR`

That should be treated as a temporary x64-only probe to reveal the next actual blocker, not as the final ABI design.

Logs produced:
- `reports/x64_first_crash_instrumented_gdb.log`
- `reports/x64_first_crash_instrumented_stderr_clean.log`
- `reports/x64_first_crash_fix_build_clean.log`
- `reports/x64_first_crash_after_fix_gdb_clean.log`
- `reports/x64_step_diag_stderr_clean.log`
- `reports/x64_step_diag_build_clean.log`
