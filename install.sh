#!/usr/bin/env bash
# One-step setup for Ghast Clicker: installs build deps, builds, installs the
# binary + app launcher, and grants access to /dev/uinput and /dev/input.
#
#   ./install.sh              build and install
#   ./install.sh --uninstall  remove everything this script installed
set -euo pipefail
cd "$(dirname "$0")"

PREFIX=/usr/local
BIN="$PREFIX/bin/ghast-clicker"
DESKTOP="$PREFIX/share/applications/ghast-clicker.desktop"
RULES=/etc/udev/rules.d/99-ghast-clicker.rules
MODULES=/etc/modules-load.d/ghast-clicker.conf

say()  { printf '\033[1;35m==>\033[0m \033[1m%s\033[0m\n' "$*"; }
warn() { printf '\033[1;33m!!\033[0m  %s\n' "$*"; }

if [[ "${1:-}" == "--uninstall" ]]; then
    say "Removing Ghast Clicker"
    sudo rm -f "$BIN" "$DESKTOP" "$RULES" "$MODULES"
    sudo udevadm control --reload-rules
    say "Done. (Your settings in ~/.config/ghast-clicker were kept.)"
    exit 0
fi

say "Installing build dependencies"
if command -v pacman >/dev/null; then
    sudo pacman -S --needed --noconfirm glfw cmake base-devel
elif command -v apt-get >/dev/null; then
    sudo apt-get install -y libglfw3-dev libgl-dev cmake build-essential
elif command -v dnf >/dev/null; then
    sudo dnf install -y glfw-devel mesa-libGL-devel cmake gcc-c++
else
    warn "Unknown package manager. Make sure glfw, OpenGL headers, cmake and a C++17 compiler are installed."
fi

say "Building"
# A cache left over from building somewhere else makes cmake refuse to run.
if [[ -f build/CMakeCache.txt ]] && ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=$PWD\$" build/CMakeCache.txt; then
    rm -rf build/CMakeCache.txt build/CMakeFiles
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

say "Installing to $PREFIX"
sudo install -Dm755 build/ghast-clicker "$BIN"
sudo install -Dm644 ghast-clicker.desktop "$DESKTOP"

say "Granting device access"
sudo install -Dm644 99-ghast-clicker.rules "$RULES"
echo uinput | sudo tee "$MODULES" >/dev/null
sudo modprobe uinput
sudo udevadm control --reload-rules
sudo udevadm trigger

relog=0
if ! id -nG "$USER" | grep -qw input; then
    sudo usermod -aG input "$USER"
    relog=1
fi

echo
say "Ghast Clicker is installed!"
echo "    Launch it from your app menu, or run: ghast-clicker"
if (( relog )); then
    warn "You were just added to the 'input' group. Log out and back in before the first launch."
fi
