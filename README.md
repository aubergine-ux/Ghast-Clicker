<div align="center">

# Ghast Clicker

A humanized auto-clicker for Linux, Wayland included.

![Linux](https://img.shields.io/badge/linux-wayland%20%7C%20x11-8b5cf6?style=flat-square&logo=linux&logoColor=white)
![C++17](https://img.shields.io/badge/c%2B%2B-17-8b5cf6?style=flat-square)
![GPLv3](https://img.shields.io/badge/license-GPLv3-8b5cf6?style=flat-square)

</div>

<br>

Ghast Clicker started as a Linux port of [GhostTap](https://github.com/IcySnex/GhostTap). GhostTap is a Forge mod that clicks from inside Minecraft; this one is a regular program that sends clicks through `/dev/uinput`, so it doesn't care which game, launcher or compositor you use.

The clicking isn't a fixed interval. CPS is drawn from a distribution and drifts over time, with the occasional spike, stutter, long hold, double-click or short break thrown in, the way real clicking looks.

## What's in it

- Separate left and right clickers, each with its own CPS, timing and hotkey
- Toggle, hold and mouse activation modes
- Presets: Normal, Jitter, Butterfly, Drag Click
- Jump Reset and Sprint Reset (W-Tap / S-Tap / Shift-Tap)
- Macros for click, delay and hotbar-switch sequences
- Pauses while your inventory is open, plus an optional hotbar slot filter
- Live CPS graphs, session stats and CSV export
- Saved profiles in `~/.config/ghast-clicker/`

## Install

```bash
git clone https://github.com/aubergine-ux/Ghast-Clicker.git
cd Ghast-Clicker
./install.sh
```

The script installs the build dependencies (Arch, Debian/Ubuntu and Fedora are supported), builds the app, puts it in your app menu and sets up the device permissions it needs. If it adds you to the `input` group you'll have to log out and back in once.

`./install.sh --uninstall` removes it again. Your settings are left alone.

## Usage

Open Ghast Clicker from your app menu. The Home tab lists every module with its hotkey and current status; click a hotkey to rebind it. After that you can leave the window in the background and use the keys from inside the game.

| Key | Does |
|:---:|---|
| <kbd>F6</kbd> | Left clicker |
| <kbd>F7</kbd> | Right clicker |
| <kbd>F9</kbd> | Jump Reset on/off |
| <kbd>F10</kbd> | Sprint Reset on/off |
| <kbd>E</kbd> | Inventory, pauses everything while open |

Each clicker has three activation modes:

- **Toggle**: press the key to start, press it again to stop.
- **Hold**: clicks while the key is held down.
- **Mouse**: press the key once to arm it, then hold your actual mouse button to click. This is the one most people want for PvP.

Settings are saved when you close the window.

## Jump Reset and Sprint Reset

Both come from GhostTap and live in the Combat tab.

**Jump Reset** taps Space just after you hit someone, so you're already jumping when their hit comes back and you take less knockback.

**Sprint Reset** breaks your sprint for one to three ticks after a hit so the next one deals full sprint knockback. It can do this by letting go of W, tapping S, or tapping Shift.

There's one real difference from GhostTap. The mod can see Minecraft's hit packets; an outside program can't. So here both modules react to your left clicks, and a cooldown keeps them to one reset per hit window (a player can only take damage every 500 ms anyway).

W-Tap needs to let go of W while your finger is still on it, which means grabbing the keyboard and passing keys through a virtual one while it's turned on. It only grabs or releases while no keys are down, so nothing gets stuck. S-Tap and Shift-Tap don't need any of that.

Neither one fires while the Ghast Clicker window is focused.

## Troubleshooting

<details>
<summary>It opens a "Setup needed" window</summary>
<br>

It can't reach `/dev/uinput` or `/dev/input`. Run `./install.sh`, or do the same thing by hand:

```bash
sudo usermod -aG input $USER
sudo cp 99-ghast-clicker.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo modprobe uinput
```

Then log out and back in.

</details>

<details>
<summary>My mouse feels different in mouse mode</summary>
<br>

Mouse mode routes your mouse through a virtual device. The udev rule in this repo gives that device flat acceleration so it matches your real one. `install.sh` sets it up; if you built by hand, copy `99-ghast-clicker.rules` into `/etc/udev/rules.d/`.

</details>

<details>
<summary>W-Tap keeps saying "Release all keys…"</summary>
<br>

It's waiting for a moment where you aren't holding anything so it can grab the keyboard safely. Let go of every key briefly.

</details>

<details>
<summary>Jump Reset / Sprint Reset never do anything</summary>
<br>

By default both only fire while you're holding W. If that's not it, turn on "Record Combat" in the Analytics tab, export, and the CSV will tell you why each one was skipped.

</details>

## Building by hand

You need `cmake`, `glfw`, OpenGL headers and a C++17 compiler.

```bash
sudo pacman -S glfw cmake base-devel   # Arch

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/ghast-clicker
```

<details>
<summary>Command-line options</summary>
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
--headless      No GUI, status in the terminal
--save          Save config to ~/.config/ghast-clicker/config
```

</details>

<details>
<summary>Exported CSV files</summary>
<br>

Analytics → Export CSV writes to `~/.config/ghast-clicker/`:

- `analytics_left_*.csv`, `analytics_right_*.csv`: Timestamp, TargetCPS, ActualCPS, HoldMS, IntervalMS, Trend
- `analytics_jump_reset_*.csv`, `analytics_sprint_reset_*.csv`: Timestamp, Outcome, Action, DelayMS, HoldMS

</details>

## Credits

- [IcySnex/GhostTap](https://github.com/IcySnex/GhostTap) for the humanization model, Jump Reset and Sprint Reset
- [b1scoito/clicker](https://github.com/b1scoito/clicker), the original Windows clicker
- [ocornut/imgui](https://github.com/ocornut/imgui) for the GUI

## License

GPLv3, see [LICENSE](LICENSE).
