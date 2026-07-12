#!/bin/bash
set -e

cd "$(dirname "$0")"

pio run
pio run -t upload
