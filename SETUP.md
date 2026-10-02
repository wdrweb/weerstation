# Weerstation — setup for new developers

Zephyr v4.4.0 app (`weerstation/`, C++) for board `esp32c6_devkitc/esp32c6/hpcore`.
VS Code with the Zephyr IDE extension (`mylonics.zephyr-ide`) + Cortex-Debug.

**Repo root = west workspace = VS Code root.** Always open this folder itself in
VS Code, never a subfolder — the extension anchors on the opened root.

## 1. Install tooling (per OS)

1. Zephyr host dependencies for your OS:
   <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>
   (OS tab), plus a serial monitor.
2. Python 3.12+, then at the repo root:
   `python3 -m venv .venv` → activate it (`Linux/macOS: source .venv/bin/activate`,
   fish: `.../activate.fish`, Windows: `.venv\Scripts\Activate.ps1`) →
   `pip install west`. Activate the venv for **every** west command below.
3. Zephyr SDK (version matching Zephyr, see upstream guide) — note its install path.
4. **Espressif OpenOCD fork** (the SDK's own OpenOCD cannot debug ESP32-C6:
   `Error: Can't find board/esp32c6-builtin.cfg`). No package manager has it —
   download `openocd-esp32-<os>-<version>.tar.gz/.zip` from
   <https://github.com/espressif/openocd-esp32/releases> and extract to
   `~/.local/opt/openocd-esp32` (Windows: `%USERPROFILE%\.local\opt\openocd-esp32`).
5. ESP32-C6 SVD for the Peripherals view:
   <https://raw.githubusercontent.com/espressif/svd/main/svd/esp32c6.svd>
   → `~/.local/share/svd/esp32c6.svd`, then fix Espressif's undersized blocks:
   `python3 tools/fix_esp32c6_svd.py <that file>` (keeps a `.orig` backup).

## 2. First checkout

```bash
git clone <url> weerstation && cd weerstation
python3 -m venv .venv && <activate it> && pip install west
west update                    # pulls Zephyr + HALs into external/ (GBs)
west blobs fetch hal_espressif # RF blobs
west packages pip --install
west zephyr-export
cp .vscode/launch.json.example .vscode/launch.json
cp .vscode/zephyr-ide.json.example .vscode/zephyr-ide.json
cp .vscode/settings.json.example .vscode/settings.json
```

Then edit your copies (never commit them — they are gitignored):
replace every `ABSOLUTE_PATH_TO` with your machine's paths
(openocd-esp32 dir, `esp32c6.svd` file). Use forward slashes on all OSes.

Linux only: udev rules so OpenOCD may open the USB probe
(`SUBSYSTEM=="usb"` line for `303a:1001` — a tty-only rule lets flashing work
while debugging fails with `LIBUSB_ERROR_ACCESS`) and serial group membership.

## 3. Build, flash, debug

Board (qualifiers mandatory): `esp32c6_devkitc/esp32c6/hpcore`.
Two USB-C cables: `USB` port = flash + JTAG, `UART` port = console (115200).

```bash
west build -b esp32c6_devkitc/esp32c6/hpcore -d weerstation/build/esp32c6_devkitc/esp32c6/hpcore weerstation
west flash -d weerstation/build/esp32c6_devkitc/esp32c6/hpcore --esp-device <JTAG-PORT>
```

In VS Code: install the recommended extensions, pick project `weerstation` +
build in the Zephyr IDE panel, F5 on **Zephyr IDE West: Debug**
(launch — it runs to `main()`; Attach stops in ROM with no symbols).
Debug does **not** flash — always flash from the terminal first.

Expect: brief halt at `0x40000000` (`?? ()`), then stop at `main()` with
source, locals, threads (`main`, `logging`, `idle`) and a Peripherals view.

## 4. Debugging rules of thumb

- One debugger at a time (stale `west debugserver`/OpenOCD on ports 3333+4444
  freezes a new session). VS Code Restart = JTAG reset; don't press the board
  RST button mid-session (drops USB).
- Max 4 breakpoints on ESP32-C6 (hardware triggers). Variables/Watch update only
  when halted; Live Watch pane shows globals/`static`s while running
  (stack locals only valid inside their function).
- Console text comes from the UART port; everything in the Debug panel rides JTAG.
