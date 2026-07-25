<h1 align="center">Ghast Clicker</h1>

<p align="center">
  A humanized auto-clicker for Linux/Wayland, built on the humanization engine from <a href="https://github.com/IcySnex/GhostTap">GhostTap</a> and <a href="https://github.com/b1scoito/clicker">b1scoito/clicker</a>.
</p>

---

## What Is This?

A standalone Linux auto-clicker with deep humanization — Gaussian CPS distributions, random spikes/stutters, variable hold times, and rhythmic drift. Runs on Wayland (and X11 via XWayland) with an ImGui GUI.

Injects clicks via `/dev/uinput` and reads keyboard/mouse input via evdev. Works with any application.

---

## Features

- **ImGui GUI** with full parameter control (GLFW + OpenGL3)
- **Independent Left & Right clickers** with separate CPS, humanization, and activation
- **3 Activation Modes:** Toggle, Hold, Mouse (arm with key, hold real mouse to click)
- **Deep humanization:** Gaussian CPS, spikes, stutters, heavy holds, rhythm volatility/tension
- **Config profiles** saved to `~/.config/ghast-clicker/`
- **Analytics recording** with CSV export
- **Interactive keybind picker** in the GUI
- **Inventory pause** (E key pauses clicking, E/Esc resumes)
- **Physical device detection** (skips virtual keyboards/mice)
- **Headless CLI mode** (`--headless`)

---

## Building

Requires: `cmake`, `glfw`, `opengl`, a C++17 compiler.

```bash
# Arch
sudo pacman -S glfw cmake base-devel

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## Running

You need access to `/dev/uinput` and `/dev/input/*`:

```bash
# Option 1: Add yourself to input group (persistent, recommended)
sudo usermod -aG input $USER
# Log out and back in, then:
./ghast-clicker

# Option 2: Run with sudo
sudo -E ./ghast-clicker
```

### CLI Options

```
--lcps <n>      Left CPS mean (default: 13.0)
--rcps <n>      Right CPS mean (default: 15.0)
--lkey <code>   Left toggle key (evdev code, default: F6)
--rkey <code>   Right toggle key (evdev code, default: F7)
--invkey <code> Inventory key (default: E)
--toggle        Toggle mode (default)
--hold          Hold mode
--mouse         Mouse mode
--headless      CLI only, no GUI
--save          Save config to ~/.config/ghast-clicker/config
```

---

## Activation Modes

| Mode | Behavior |
|------|----------|
| **Toggle** | Press key to start, press again to stop |
| **Hold** | Clicks while key is held |
| **Mouse** | Press key to arm, then hold real mouse button to auto-click. Real clicks are masked. |

---

## Credits

- [IcySnex/GhostTap](https://github.com/IcySnex/GhostTap) — humanization engine, variance model
- [b1scoito/clicker](https://github.com/b1scoito/clicker) — original Windows clicker
- [ocornut/imgui](https://github.com/ocornut/imgui) — GUI framework

---

## License

Licensed under the **GNU General Public License v3.0** — see [LICENSE](LICENSE).
