<div align="center">

# 👻 Ghast Clicker

**A humanized auto-clicker for Linux & Wayland**

Built on the humanization engine from [GhostTap](https://github.com/IcySnex/GhostTap), with a native ImGui interface.

![Platform](https://img.shields.io/badge/platform-Linux-informational?style=flat-square&logo=linux&logoColor=white)
![Wayland](https://img.shields.io/badge/Wayland-%20%2B%20X11-blueviolet?style=flat-square)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![License](https://img.shields.io/badge/license-GPLv3-green?style=flat-square)

[Quick Start](#-quick-start) •
[Features](#-features) •
[Using It](#-using-it) •
[Combat Modules](#-combat-modules) •
[Troubleshooting](#-troubleshooting)

</div>

---

## ✨ Features

| | |
|---|---|
| 🖱️ **Dual clickers** | Independent left & right clickers, each with its own CPS, timing and hotkey |
| 🧠 **Deep humanization** | Gaussian / Poisson / Uniform CPS, spikes, stutters, heavy holds, rhythm drift, fatigue breaks, double-clicks |
| 🎛️ **3 activation modes** | Toggle, Hold, or Mouse (arm with a key, then just hold your real mouse button) |
| 🦘 **Jump Reset** | Taps Space right after you attack to cut the knockback you take in trades |
| 🏃 **Sprint Reset** | W-Tap, S-Tap or Shift-Tap after each hit for full sprint knockback |
| 🎯 **Presets** | Normal, Jitter, Butterfly and Drag Click profiles, one click each |
| 🔁 **Macros** | Chain clicks, delays and hotbar switches, bound to any key |
| 🎒 **Smart pausing** | Pauses while your inventory is open; optional hotbar slot filter |
| 📊 **Analytics** | Live CPS graphs, session stats, CSV export for clicks *and* combat decisions |
| 💾 **Profiles** | Save and load named configs in `~/.config/ghast-clicker/` |
| 🔊 **Audio cues** | Optional beep when anything turns on or off |

Clicks and key presses are injected through `/dev/uinput`, so it works with **any** application, on Wayland and X11 alike.

---

## 🚀 Quick Start

```bash
git clone https://github.com/aubergine-ux/Ghast-Clicker.git
cd Ghast-Clicker
./install.sh
```

That's it. The script installs dependencies (Arch, Debian/Ubuntu, Fedora), builds, adds **Ghast Clicker** to your app menu, and sets up device permissions.

> [!IMPORTANT]
> If the installer adds you to the `input` group, **log out and back in** once before the first launch.

To remove it later: `./install.sh --uninstall`

---

## 🎮 Using It

Open **Ghast Clicker** from your app menu. The **Home** tab shows every module at a glance: its hotkey, mode and live status. Click any hotkey button and press a key to rebind it.

Then switch to your game and press the hotkeys:

| Default key | Action |
|:---:|---|
| <kbd>F6</kbd> | Left clicker |
| <kbd>F7</kbd> | Right clicker |
| <kbd>F9</kbd> | Jump Reset on/off |
| <kbd>F10</kbd> | Sprint Reset on/off |
| <kbd>E</kbd> | Inventory open/close (pauses clicking) |

### Activation modes

| Mode | How it works |
|---|---|
| **Toggle** | Press the key to start, press again to stop |
| **Hold** | Clicks only while the key is held |
| **Mouse** | Press the key once to *arm*, then hold your real mouse button to auto-click. Your physical clicks are masked. Best for PvP. |

Settings save automatically when you close the window.

---

## 💥 Combat Modules

Ported from GhostTap's Jump Reset and Sprint Reset. Configure both in the **Combat** tab.

| Module | What it does | Key settings |
|---|---|---|
| **Jump Reset** | Presses Space shortly after your attack so you jump as the return hit lands | Chance, reaction delay, hold time, cooldown, "only while holding W" |
| **Sprint Reset** | Breaks sprint for 1–3 ticks after your attack so your next hit deals full sprint knockback | Mode (W-Tap / S-Tap / Shift-Tap), chance, delay, duration in ticks, cooldown |

> [!NOTE]
> GhostTap is an in-game mod and reacts to Minecraft's own hit packets. Ghast Clicker runs outside the game, so both modules fire off **your left clicks** (real or auto-clicked) instead. A **cooldown** stands in for hit detection: at most one reset per window, matching how often a hit can actually land (every 500 ms).

> [!TIP]
> **W-Tap** has to release <kbd>W</kbd> while you're still holding it, so while it's on, Ghast Clicker grabs your keyboard and passes every key through a virtual one. The grab starts and stops only when no keys are held, so nothing gets stuck. **S-Tap** and **Shift-Tap** don't need the grab.

Neither module fires while the Ghast Clicker window itself is focused.

---

## 🔧 Troubleshooting

<details>
<summary><b>"Setup needed" window / permission errors</b></summary>

<br>

Ghast Clicker needs access to `/dev/uinput` and `/dev/input/*`. Run `./install.sh`, or do it by hand:

```bash
sudo usermod -aG input $USER
sudo cp 99-ghast-clicker.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo modprobe uinput
```

Then log out and back in. The setup window has a **Copy fix commands** button for this.

</details>

<details>
<summary><b>Mouse feels faster/slower in Mouse mode</b></summary>

<br>

Mouse mode re-emits your mouse through a virtual device. The bundled udev rule (installed by `install.sh`) gives that device a flat acceleration profile so it matches your real mouse.

</details>

<details>
<summary><b>W-Tap says "Release all keys to finish grabbing the keyboard"</b></summary>

<br>

The keyboard is only grabbed or released while no keys are held, so a key can't get stuck down in your compositor. Let go of everything for a moment and it'll switch.

</details>

<details>
<summary><b>Combat modules never trigger</b></summary>

<br>

Check the **Combat** tab's decision counter and turn on **Record Combat** in **Analytics** to see why each decision was skipped (`SKIP_NOT_FORWARD`, `SKIP_MENU`, `SKIP_SLOT`…). By default both modules only fire while you're holding <kbd>W</kbd>.

</details>

---

## 🧰 Advanced

<details>
<summary><b>Manual build</b></summary>

<br>

Requires `cmake`, `glfw`, OpenGL headers and a C++17 compiler.

```bash
# Arch
sudo pacman -S glfw cmake base-devel

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/ghast-clicker
```

</details>

<details>
<summary><b>Command-line options</b></summary>

<br>

```
--lcps <n>      Left CPS mean (default: 13.0)
--rcps <n>      Right CPS mean (default: 15.0)
--lkey <code>   Left toggle key (evdev code, default: F6)
--rkey <code>   Right toggle key (evdev code, default: F7)
--jkey <code>   Jump Reset toggle key (default: F9)
--skey <code>   Sprint Reset toggle key (default: F10)
--invkey <code> Inventory key (default: E)
--toggle        Toggle mode (default)
--hold          Hold mode
--mouse         Mouse mode
--headless      CLI only, no GUI
--save          Save config to ~/.config/ghast-clicker/config
```

</details>

<details>
<summary><b>Analytics CSV files</b></summary>

<br>

**Analytics → Export CSV** writes to `~/.config/ghast-clicker/`:

| File | Columns |
|---|---|
| `analytics_left_*.csv` / `analytics_right_*.csv` | Timestamp, TargetCPS, ActualCPS, HoldMS, IntervalMS, Trend |
| `analytics_jump_reset_*.csv` / `analytics_sprint_reset_*.csv` | Timestamp, Outcome, Action, DelayMS, HoldMS |

</details>

---

## 🙏 Credits

- [IcySnex/GhostTap](https://github.com/IcySnex/GhostTap): humanization engine, variance model, Jump Reset & Sprint Reset
- [b1scoito/clicker](https://github.com/b1scoito/clicker): original Windows clicker
- [ocornut/imgui](https://github.com/ocornut/imgui): GUI framework

## 📄 License

Licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE).
