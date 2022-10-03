/**@file
 * This file is part of the TASTE Linux Runtime.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * TASTE Linux Runtime was developed under a programme of,
 * and funded by, the European Space Agency (the "ESA").
 *
 * Licensed under the ESA Public License (ESA-PL) Permissive,
 * Version 2.3 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://essr.esa.int/license/list
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <Broker.h>
#include <Escaper.h>

#include <drivers_config.h>

#include "Uart.h"

typedef struct final {
    Escaper m_escaper;
    Uart uart;
    enum SystemBus m_ip_device_bus_id;
    Uart_TxHandler txHandler;
    Uart_RxHandler rxHandler;
    ByteFifo *fifoTx;
    ByteFifo *fifoRx;
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