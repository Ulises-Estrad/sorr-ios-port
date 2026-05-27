# iOS Input Bridge Plan

Date: 2026-05-27

## Goal

Prepare the iOS input bridge without starting iOS implementation yet.

First iOS input target: emulate the confirmed desktop keyboard defaults, not a gamepad.

## Confirmed Desktop Keyboard Map

| Action | Bennu key code | Desktop key |
| --- | ---: | --- |
| Up | 72 | Arrow Up |
| Down | 80 | Arrow Down |
| Left | 75 | Arrow Left |
| Right | 77 | Arrow Right |
| Attack | 46 | C |
| Jump | 47 | V |
| Special | 45 | X |
| Police | 48 | B |
| Start/Pause | 28 | Enter |
| Back/Menu | 1 or 14 | Escape or Backspace |

These key codes come from `modules/libkey/libkey.c`.

## iOS Touch Mapping

| iOS control | Emulated Bennu/SDL keyboard input |
| --- | --- |
| D-pad Up | Arrow Up / Bennu key `72` |
| D-pad Down | Arrow Down / Bennu key `80` |
| D-pad Left | Arrow Left / Bennu key `75` |
| D-pad Right | Arrow Right / Bennu key `77` |
| Attack | C / Bennu key `46` |
| Jump | V / Bennu key `47` |
| Special | X / Bennu key `45` |
| Police | B / Bennu key `48` |
| Start/Pause | Enter / Bennu key `28` |
| Back/Menu | Escape `1`, with Backspace `14` as fallback |

## Touch Layout

Left side:

```text
D-pad
```

Right-side diamond:

```text
        Special
Attack          Police
         Jump
```

Top/right small:

```text
Start/Pause
```

Back/Menu should be a small secondary button near the top edge, separated from Start/Pause to avoid accidental exits.

## Preferred Injection Point

Preferred approach: add a portable virtual key layer and merge it inside Bennu's `KEY(...)` path.

Current runtime path:

```text
game script KEY(code)
  -> modules/mod_key/mod_key.c::_get_key(code)
  -> modules/libkey/libkey.c key_table + SDL_GetKeyboardState()
```

Add a virtual key state array:

```c
static unsigned char portable_virtual_keys[127];
```

Expose a tiny bridge API:

```c
void portable_input_set_key(int bennu_key_code, int pressed);
int portable_input_get_key(int bennu_key_code);
```

Then in `_get_key(code)`, return:

```text
SDL keyboard state OR portable virtual key state
```

Why this is the cleanest first iOS path:

- The game is confirmed to work when the script polls keyboard codes.
- It avoids joystick/gamepad binding state entirely.
- It avoids depending on whether synthetic SDL key events update `SDL_GetKeyboardState()`.
- It can be compiled only for iOS/portable bridge builds.
- It keeps the bridge below the game script and above platform-specific touch UI.

## Alternatives Considered

### SDL Keyboard Event Simulation

Idea:

```text
touch down -> SDL_KEYDOWN
touch up   -> SDL_KEYUP
```

Risk:

- The game uses `SDL_GetKeyboardState()` through Bennu `KEY(...)`.
- Synthetic `SDL_PushEvent` key events may not reliably update that state table across SDL backends.
- Menus and gameplay are sensitive to held state, not just discrete events.

Use this only if the direct virtual key layer is blocked.

### Direct Mutation Of libkey keystate

Risk:

- `keystate` is a pointer returned by `SDL_GetKeyboardState()`.
- It is treated as SDL-owned state.
- Mutating it directly is brittle and backend-dependent.

Do not use this as the first iOS bridge.

### Gamepad/Joystick Emulation

Risk:

- We just fixed the desktop proof by avoiding the missing-gamepad path.
- The prepared save previously behaved like it had non-PC bindings.
- Joystick emulation would re-enter the subsystem that caused the P1 problem.

Defer gamepad support until the keyboard bridge is playable.

## Proposed Future File Changes

Do not implement yet. When iOS starts, keep changes small:

1. Add a bridge header/source near the input layer:

```text
modules/libkey/portable_input_bridge.h
modules/libkey/portable_input_bridge.c
```

2. Update:

```text
modules/mod_key/mod_key.c
```

to OR the virtual key state into `_get_key(code)`.

3. Add compile guard:

```text
PORTABLE_INPUT_BRIDGE
```

or platform guard:

```text
TARGET_IOS
```

4. In the iOS/touch layer, call:

```c
portable_input_set_key(72, dpad_up_pressed);
portable_input_set_key(80, dpad_down_pressed);
portable_input_set_key(75, dpad_left_pressed);
portable_input_set_key(77, dpad_right_pressed);
portable_input_set_key(46, attack_pressed);
portable_input_set_key(47, jump_pressed);
portable_input_set_key(45, special_pressed);
portable_input_set_key(48, police_pressed);
portable_input_set_key(28, start_pressed);
portable_input_set_key(1, back_pressed);
portable_input_set_key(14, back_pressed);
```

## Multi-Touch Rules

- Each touch button owns one or more active touch IDs.
- A virtual key stays pressed while at least one active touch remains inside that button.
- D-pad supports diagonals by allowing two direction keys at once.
- Moving a finger between D-pad zones should release the previous direction and press the new direction.
- Action buttons should allow chords, for example moving while attacking or jumping.
- Start/Pause and Back/Menu should have a short debounce to avoid accidental repeats.

## Desktop Test Harness Before iOS

Before wiring UIKit/SDL iOS touch events, test the bridge on desktop:

- Add a temporary keyboard-to-virtual-key mirror behind an env var.
- Confirm virtual `C`, `V`, `X`, `B`, arrows, Enter, Escape/Backspace all appear through `KEY(...)`.
- Reuse `PORTABLE_INPUT_DIAG=1` to verify the game script polls those same codes.

## Success Criteria

The first iOS input bridge is successful when:

- P1 does not require a detected gamepad.
- D-pad moves the selected menu cursor and character.
- Attack, jump, special, police match desktop keyboard behavior.
- Start/Pause maps to Enter behavior.
- Back/Menu maps to Escape/Backspace behavior.
- No changes are required to `savegame.sor` beyond the known-good keyboard state already frozen for desktop.

## Explicit Non-Goals For This Phase

- Do not implement iOS yet.
- Do not trim assets yet.
- Do not chase Options submenu restoration yet.
- Do not emulate a gamepad yet.
