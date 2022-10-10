#!/bin/bash

PREFIX=/home/taste/tool-inst
SOURCES=$(dirname $0)

mkdir -p "${PREFIX}/include/TASTE-LEON3-Drivers"
mkdir -p "${PREFIX}/include/TASTE-LEON3-Runtime/src"

rm -rf "${PREFIX}/include/TASTE-LEON3-Drivers/src/*"
cp -r "${SOURCES}/src/" "${PREFIX}/include/TASTE-LEON3-Drivers/src/"
cp -r "${SOURCES}/configurations" "${PREFIX}/include/TASTE-LEON3-Drivers/configurations"

cp -r "${SOURCES}/TASTE-LEON3-Runtime/Leon3-BSP" "${PREFIX}/include/TASTE-LEON3-Runtime/src/"
cp -r "${SOURCES}/TASTE-LEON3-Runtime/BrokerLock" "${PREFIX}/include/TASTE-LEON3-Runtime/"
cp -r "${SOURCES}/TASTE-LEON3-Runtime/Leon3-BSP/src/Uart" "${PREFIX}/include/TASTE-LEON3-Runtime/"
cp -r "${SOURCES}/TASTE-LEON3-Runtime/Leon3-BSP/src/Utils" "${PREFIX}/include/TASTE-LEON3-Runtime/"
cp -r "${SOURCES}/RuntimeMocks" "${PREFIX}/include/TASTE-LEON3-Runtime/"

cp -r "${SOURCES}/TASTE-Runtime-Common/src/Broker" "${PREFIX}/include/TASTE-LEON3-Runtime/"
cp -r "${SOURCES}/TASTE-Runtime-Common/src/Escaper" "${PREFIX}/include/TASTE-LEON3-Runtime/"
cp -r "${SOURCES}/TASTE-Runtime-Common/src/Packetizer" "${PREFIX}/include/TASTE-LEON3-Runtime/"
cp -r "${SOURCES}/TASTE-Runtime-Common/src/RuntimeCommon" "${PREFIX}/include/TASTE-LEON3-Runtime/"
