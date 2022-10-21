/**@file
 * This file is part of the Leon3 Serial CCSDS Driver for the TASTE Leon3 Drivers.
 *
 * @copyright 2022 N7 Space Sp. z o.o.
 * 
 * Leon3 Serial CCSDS Driver for the TASTE Leon3 Drivers was developed under the project AURORA.
 * This project has received funding from the European Union’s Horizon 2020
 * research and innovation programme under grant agreement No 101004291”
 *
 * Leon3 Serial CCSDS Driver for the TASTE Leon3 Drivers is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Leon3 Serial CCSDS Driver for the TASTE Leon3 Drivers is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Leon3 Serial CCSDS Driver for the TASTE Leon3 Drivers. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <assert.h>

#include <Broker.h>
#include <Escaper.h>

#include <drivers_config.h>
#include <rtems.h>

#include "Uart.h"

#define DRIVER_TASK_PRIORITY 1
#define Serial_CCSDS_LEON3_BUFFER_SIZE 256
#define Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE 256
#define Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE BROKER_BUFFER_SIZE

#define MAX_TLS_SIZE RTEMS_ALIGN_UP( 0, RTEMS_TASK_STORAGE_ALIGNMENT )
#define LEON3_SERIAL_CCSDS_POLL_STACK_SIZE (128 > RTEMS_MINIMUM_STACK_SIZE ?  128 : RTEMS_MINIMUM_STACK_SIZE)

typedef struct final {
    Escaper escaper;
    Uart uart;
    enum SystemBus ipDeviceBusId;
    Uart_TxHandler txHandler;
    Uart_RxHandler rxHandler;
    ByteFifo fifoTx;
    ByteFifo fifoRx;
    uint8_t fifoMemoryBlockRx[Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE];
    uint8_t fifoMemoryBlockTx[Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE];
    rtems_id semRx;
    rtems_id semTx;
    rtems_id taskId;
    rtems_id leon3SerialCcsdsPoll_TCB;
    uint8_t recvBuffer[Serial_CCSDS_LEON3_BUFFER_SIZE];
    uint8_t encodedPacketBuffer[Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE];
    uint8_t decodedPacketBuffer[Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE];
    RTEMS_ALIGNED(RTEMS_TASK_STORAGE_ALIGNMENT)  
    char leon3SerialCcsdsPoll_TaskBuffer[RTEMS_TASK_STORAGE_SIZE(
        LEON3_SERIAL_CCSDS_POLL_STACK_SIZE + MAX_TLS_SIZE, RTEMS_FLOATING_POINT)];
} leon3_serial_ccsds_private_data;

/**
 * @brief Initialize driver.
 *
 * Function is used by runtime to initialize the driver.
 *
 * @param private_data                  Driver private data, allocated by
 * runtime
 * @param bus_id                        Identifier of the bus, which is driver
 * @param device_id                     Identifier of the device
 * @param device_configuration          Configuration of device
 * @param remote_device_configuration   Configuration of remote device
 */
void Leon3SerialCcsdsInit(
    void *private_data, const enum SystemBus bus_id,
    const enum SystemDevice device_id,
    const Serial_CCSDS_Leon3_Conf_T *const device_configuration,
    const Serial_CCSDS_Leon3_Conf_T *const remote_device_configuration);

/**
 * @brief Function which implements receiving data from remote partition.
 *
 * Functions works in separate thread, which is initialized by
 * Leon3SerialCcsdsSend
 *
 * @param private_data   Driver private data, allocated by runtime
 */
void Leon3SerialCcsdsPoll(void *private_data);

/**
 * @brief Send data to remote partition.
 *
 * Function is used by runtime.
 *
 * @param private_data   Driver private data, allocated by runtime
 * @param data           The Buffer which data to send to connected remote
 * partition
 * @param length         The size of the buffer
 */
void Leon3SerialCcsdsSend(void *private_data, const uint8_t *const data, const size_t length);
