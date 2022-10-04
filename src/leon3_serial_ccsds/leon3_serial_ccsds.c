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

#include "leon3_serial_ccsds.h"

#include <assert.h>

#include <EscaperInternal.h>

BYTE_FIFO_CREATE(uartFifoTx, Serial_CCSDS_LEON3_BUFFER_SIZE);
BYTE_FIFO_CREATE(uartFifoRx, Serial_CCSDS_LEON3_BUFFER_SIZE);

static void UartRxCallback(void *private_data) {
  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;

  rtems_semaphore_release(self->semRx);
}

static void *UartTxCallback(void *private_data) {
  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;

  rtems_semaphore_release(self->semTx);
}

static inline Uart_Id getUartId(const Serial_CCSDS_Leon3_Device_T dev)
{
  switch (dev) {
    case uart0: {
      return Uart_Id_0;
      break;
    }
    case uart1: {
      return Uart_Id_1;
      break;
    }
    case uart2: {
      return Uart_Id_2;
      break;
    }
    case uart3: {
      return Uart_Id_3;
      break;
    }
    case uart4: {
      return Uart_Id_4;
      break;
    }
    case uart5: {
      return Uart_Id_5;
      break;
    }
    default: {
      return Uart_Id_Invalid;
    }
  }
}

static inline void getUartParity(
    const Serial_CCSDS_Leon3_Conf_T *const deviceConfiguration)
{
  if (deviceConfiguration->use_paritybit) {
    if ( deviceConfiguration->parity == Serial_CCSDS_Leon3_Parity_T_odd) {
      return Uart_Parity_Odd;
    } else {
      return Uart_Parity_Even;
    }
  } else {
    return Uart_Parity_None;
  }
}

static inline void Leon3SerialCcsdsInitUartInit(
    leon3_serial_ccsds_private_data *const self,
    const Serial_CCSDS_Leon3_Conf_T *const deviceConfiguration) {

    Uart_Id id = getUartId(deviceConfiguration->devname);
    Uart_Config config = (Uart_Config){ 0 };
    config.isTxEnabled = true;
    config.isRxEnabled = true;
    config.isLoopbackModeEnabled = false;
    config.parity = getUartParity(deviceConfiguration);

    self->txHandler.callback = UartTxCallback;
    self->txHandler.arg = self;

    self->rxHandler.callback = UartRxCallback;
    self->rxHandler.arg = self;
    
    self->fifoTx = &uartFifoTx;
    self->fifoRx = &uartFifoRx;

    Uart_init(id, &self->uart);
    Uart_setConfig(&self->uart, &config);
    Uart_startup(&self->uart);

    Escaper_init(&self->escaper, self->encodedPacketBuffer,
        Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE,
        self->decodedPacketBuffer,
        Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE);

    rtems_semaphore_create(
        rtems_build_name('s', 'm', 'R', 'x'),
        0,
        SIMPLE_BINARY_SEMAPHORE,
        0,
        &self->semRx);

    rtems_semaphore_create(
        rtems_build_name('s', 'm', 'T', 'x'),
        0,
        SIMPLE_BINARY_SEMAPHORE,
        0,
        &self->semTx);

    rtems_task_create(rtems_build_name('p', 'o', 'l', 'l'),
        DRIVER_TASK_PRIORITY,
        RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,
        RTEMS_LOCAL,
        &self->taskId);

    rtems_task_start(self->tasksId, Leon3SerialCcsdsPoll, self);
}

static inline void Leon3SerialCcsdsPollUartPoll(
    leon3_serial_ccsds_private_data *const self)
{
  size_t length = 0;

  Escaper_start_decoder(&self->escaper);

  rtems_semaphore_obtain(self->semRx, RTEMS_WAIT, MAX_DELAY);

  Uart_readAsync(&self->uart, self->fifoRx, self->rxHandler);

  while (true) {
    rtems_semaphore_obtain(self->semRx, RTEMS_WAIT, MAX_DELAY);

    length = ByteFifo_getCount(&self->fifoRx);
    for (size_t i = 0; i < length; i++) {
      ByteFifo_pull(&self->rxFifo, &self->recvBuffer[i]);
    }

    Escaper_decode_packet(
        &self->escaper,
        self->ipDeviceBusId,
        self->recvBuffer,
        length, Broker_receive_packet);
  }
}

void Leon3SerialCcsdsInit(
    void *private_data, const enum SystemBus bus_id,
    const enum SystemDevice device_id,
    const Serial_CCSDS_Leon3_Conf_T *const device_configuration,
    const Serial_CCSDS_Leon3_Conf_T *const remote_device_configuration) {

  (void)device_id;
  (void)remote_device_configuration;

  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;

  self->ipDeviceBusId = bus_id;

  Leon3SerialCcsdsInitUartInit(self, device_configuration);
}

void Leon3SerialCcsdsPoll(void *private_data) {
  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;

  Leon3SerialCcsdsPollUartPoll(self);
}

void Leon3SerialCcsdsSend(void *private_data, const uint8_t *const data,
                           const size_t length) {
  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;

  size_t index = 0;
  size_t packetLength = 0;

  Escaper_start_encoder(&self->m_escaper);
  while (index < length) {
    packetLength =
        Escaper_encode_packet(&self->m_escaper, data, length, &index);

    rtems_semaphore_obtain(self->semTx, RTEMS_WAIT, MAX_DELAY);

    ByteFifo_initFromBytes(
        self->fifoTx,
        self->encodedPacketBuffer,
        packetLength);
    Uart_writeAsync(&self->uart, self->fifoTx, self->txHandler);
  }
}