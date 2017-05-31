#!/bin/bash
set -euo pipefail

export IFTTT_URL="https://maker.ifttt.com/trigger/iodings/with/key/dki_-84D6U_se1F_Ktrv2pSeNQSDmIiF7o7arbmTb8C"
~milkey/iodings/iodings listen ~milkey/iodings/config | python3 ~milkey/iodings/notifier.py
