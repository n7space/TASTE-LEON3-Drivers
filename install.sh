#!/bin/bash

PREFIX=/home/taste/tool-inst
SOURCES=$(dirname $0)

mkdir -p "${PREFIX}/include/TASTE-LEON3-Drivers/src"
rm -rf "${PREFIX}/include/TASTE-LEON3-Drivers/src/*"
cp -r "${SOURCES}/src/leon3_serial_ccsds" "${PREFIX}/include/TASTE-LEON3-Drivers/src"
cp -r "${SOURCES}/configurations" "${PREFIX}/include/TASTE-LEON3-Drivers/configurations"
