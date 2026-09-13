#!/usr/bin/env sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
install_prefix=${PREFIX:-"$HOME/.local"}
config_dir=${XDG_CONFIG_HOME:-"$HOME/.config"}
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$install_prefix"
cmake --build "$project_dir/build" --parallel "${JOBS:-4}"
ctest --test-dir "$project_dir/build" --output-on-failure
cmake --install "$project_dir/build"
"$install_prefix/bin/razer-mixer-driver" init
mkdir -p "$config_dir/systemd/user"
install -m 0644 "$project_dir/build/razer-mixer-bridge.service" "$config_dir/systemd/user/razer-mixer-bridge.service"
systemctl --user daemon-reload
systemctl --user enable razer-mixer-bridge.service
systemctl --user restart razer-mixer-bridge.service
printf '%s\n' "Installed. Open Razer Audio Mixer from your application launcher."
