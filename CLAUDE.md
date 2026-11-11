# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Uni IoT course project: festival totems on an ESP32-S3 board driving a small e-ink display, drawn
with GxEPD2 + Adafruit GFX. The user writes most of the application code; Claude's job is the
toolchain — keeping includes resolvable and the project compiling. **No hardware is attached on this
machine**: a successful build is the only check that matters, nothing is flashed or monitored here,
and code need not be semantically correct, as it will assume the team's results to be present even
when they aren't. Source must stay plain Arduino (`setup()`/`loop()`, Arduino libraries) so it can be
copy-pasted into the uni VSCode setup.

Four-person group project. The user owns the drawing and the sequencing/ordering of screens;
teammates own RFID (MFRC522), the WebServer/WiFi AP, and the backend. `IoT/` is a local working
directory, not the repo — only files from `src/` get pushed to a teammate-owned GitHub repo.

## Environment

PlatformIO, via a Nix flake dev shell entered automatically by direnv (`.envrc`).

This is NixOS without nix-ld, and PlatformIO downloads prebuilt toolchains that expect a normal FHS
filesystem — so `pio` in the dev shell is a `buildFHSEnv` wrapper (see `flake.nix`), not
`platformio-core` on `$PATH` directly. Always invoke the toolchain through `pio`; calling a
downloaded `packages/**` binary from outside that wrapper fails on the dynamic linker.
Outside the shell, prefix with `nix develop --command`.

**The root filesystem is a 4 GB tmpfs** (NixOS impermanence; only a few paths are bind-mounted from
persistent disk, `~/Desktop` among them). The default `~/.platformio` therefore lives in RAM, where
the ~2 GB of toolchain does not fit and is wiped on every reboot. `.envrc` redirects it:

```
export PLATFORMIO_CORE_DIR="$(expand_path ./.platformio)"
```

So the core dir is `IoT/.platformio`, on persisted disk. Two consequences:

- After editing `.envrc`, direnv blocks it until the user runs `direnv allow` — Claude must not run
  that for them. To build in the meantime, pass the variable explicitly:
  `PLATFORMIO_CORE_DIR=/home/nebu/Desktop/IoT/.platformio nix develop --command pio run`
- If a build dies with `MissingPackageManifestError` or a zero-byte `package.json`, suspect
  `ENOSPC` rather than a bad download: check `df -h /`, then delete the truncated package under
  `.platformio/packages/` so pio refetches it.

## Commands

```
pio run                  # compile — the check that matters here
pio run -t clean
pio run -t compiledb     # regenerate compile_commands.json
pio pkg install          # after editing lib_deps
pio pkg outdated
```

`pio run -t upload` / `pio device monitor` need a board and are unusable here.
First build downloads the platform + toolchain (~1 GB) and takes several minutes.

`main.cpp` selects what is built via `TOTEM_PAPEL`, set in `platformio.ini` `build_flags`:
1=entrada, 2=saida, 3=comida, 4=palco, 5=teste de telas. Two envs:

```
pio run                      # env esp32-s3-devkitc-1 (default_envs), the real totem
pio run -e teste -t upload   # screen-layout test: only ESP32-S3 + e-paper needed
pio run -e esp32-s3-devkitc-1 -e teste   # build both
```

To check a role other than the one in `build_flags`, edit that line. All five values must compile;
the modules themselves are always compiled regardless of which is selected.

## Layout

Standard PlatformIO layout; `platformio.ini` is the single source of truth for board, framework
and libraries.

- `platformio.ini` — env `esp32-s3-devkitc-1`, `framework = arduino`. Libraries go in `lib_deps`,
  not vendored: Adafruit GFX, GxEPD2 (panel), U8g2_for_Adafruit_GFX (fonts/UTF-8), QRCodeGFX
  (pulled from its git URL, not the registry), GFButton, MFRC522.
- `src/` — application code, compiled automatically. `.cpp`, not `.c` — Arduino, GxEPD2 and
  Adafruit headers are C++.
- `include/`, `lib/` — own headers and own local libraries (empty; everything lives in `src/`).
- `.pio/` — generated build dir. `compile_commands.json` is written to the **project root**
  (not `.pio/build/<env>/`), which is where clangd finds it by walking up from `src/`.
- `.platformio/` — the PlatformIO core dir for this project (see Environment). Multi-GB, generated.

### Module structure

Dependencies run one way only — `totem_* → telas → display`, and `totem_* → equipe`. `telas` and
`display` must never include a totem or depend on teammate code; totems pass them plain data
(`String`, `float`, `Show`). Keeping that direction is what prevents include cycles.

- `pinos.h` — every pin define, in one place. Shared SPI bus with the teammate's MFRC522, so this
  is the file the team coordinates on. Still placeholder values.
- `equipe.h` — the contract with teammates: `User`, `PendingOrder`, `Show`, and the functions the
  others owe (RFID, pricing, PIX, AP state). Implemented nowhere in this repo.
- `equipe_stub.cpp` — no-op implementations of that contract so the project links.
  **Delete on integration.** Everything returns `false`, which parks each totem in its initial
  state — correct behaviour for "nobody has arrived yet".
- `display.h/.cpp` — panel, fonts and QR objects (declared `extern`, defined once here in
  dependency order) plus drawing primitives. `textoCentrado`/`textoCentradoEntre` measure strings
  with `getUTF8Width()`; prefer them over hardcoded x coordinates. `qrAjustado()` fits the largest
  QR into a given box and returns `false` rather than drawing an unscannable one.
- `telas.h/.cpp` — the screen templates. Each one clears the panel, draws, and refreshes. They
  decide nothing and read no sensors.
- `aviso.h/.cpp` — organizer announcement that overrides any totem's own screen.
- `totem_entrada`, `totem_saida`, `totem_comida` — one state machine each. A totem is single
  purpose, so the roles never share a state machine.
- `totem_palco` — no state machine; rotates server-pushed `Show`s on a timer.
- `teste_telas.h/.cpp` — not a totem: cycles every screen with fake data so the layout can be
  checked on real hardware. Touches no peripheral but the panel (no RFID, no QR reader, no button),
  because the stubs would otherwise park every totem on its first screen forever. `TESTE_FIXA`
  pins it to one screen while tweaking coordinates.
- `main.cpp` — compile scaffold only, replaced at integration. Holds `setup()`/`loop()` because the
  Arduino core requires them, calls `Serial.begin()` once, and dispatches to one totem.

### Invariants worth preserving

- **Only `entra()` calls a `tela*()` function.** Each totem has `entra(novo)` which stamps
  `estadoDesde = millis()` and redraws; `desenha()` is the single switch over states. Drawing from a
  loop body would redraw thousands of times a minute — each e-ink refresh blocks ~1-2 s and costs
  panel life.
- Timeouts use `millis() - estadoDesde >= X`, never `millis() > estadoDesde + X` (rollover).
- Every `tela*()` starts with `telaComeca()`. E-ink is persistent; without the clear, the new screen
  draws on top of the old one.
- User-facing strings are Portuguese, in the *você* imperative ("Escaneie", "Pegue", "Passe").
  Comments are English. Accented text needs the `_te` font suffix — `_tr` drops accents silently.
- `GFButton`'s constructor calls `pinMode()`, so the buttons are function-local statics created
  inside `*Begin()`, never globals (static init runs before the Arduino core is up).

Open questions for the group are marked in-code; `grep -n "EQUIPE\|DECISÃO" src/*` lists them. The
pin defines in `pinos.h`, the panel type in `display.h` (`GxEPD2_290_T94_V2`, 296x128) and the
`LINK_*` placeholder URLs are chosen to compile, not the real hardware or endpoints — expect the
user to replace them.
