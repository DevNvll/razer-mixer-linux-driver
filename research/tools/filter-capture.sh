#!/usr/bin/env sh
set -eu
if [ "$#" != 2 ]; then
  printf '%s\n' 'Usage: filter-capture.sh input.pcap output.pcapng' >&2
  exit 1
fi
# Keep controls and interrupts. Exclude USB string descriptors, including serials.
capture_filter='(usb.transfer_type == 1 || usb.transfer_type == 2) && !(usb.bDescriptorType == 3)'
tshark -r "$1" -Y "$capture_filter" -F pcapng -w "$2"
tshark -r "$1" -Y "$capture_filter" -T fields -e frame.number -e frame.time_relative \
  | awk 'BEGIN { print "filtered_frame,original_frame,relative_seconds" } { print NR "," $1 "," $2 }' > "${2%.pcapng}-frames.csv"
