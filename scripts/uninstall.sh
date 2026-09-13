#!/usr/bin/env sh
set -eu
install_prefix=${PREFIX:-"$HOME/.local"}
config_dir=${XDG_CONFIG_HOME:-"$HOME/.config"}
systemctl --user disable --now razer-mixer-bridge.service
rm -f "$config_dir/systemd/user/razer-mixer-bridge.service"
systemctl --user daemon-reload
rm -f "$install_prefix/bin/razer-mixer" "$install_prefix/bin/razer-mixer-driver" \
      "$install_prefix/share/applications/razer-mixer.desktop" \
      "$install_prefix/share/icons/hicolor/scalable/apps/razer-mixer.svg" \
      "$install_prefix/share/razer-mixer/razer-mixer-bridge.service"
printf '%s\n' "Removed the driver and app. Your settings and capture data remain."
