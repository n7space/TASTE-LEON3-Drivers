/**@file
 * This file is part of the Leon3 Serial CCSDS Driver for the Test Environment.
 *
 * @copyright 2022 N7 Space Sp. z o.o.
 * 
 * Leon3 Serial CCSDS Driver for the Test Environment was developed under the project AURORA.
 * This project has received funding from the European Union’s Horizon 2020
 * research and innovation programme under grant agreement No 101004291”
 *
 * Leon3 Serial CCSDS Driver for the Test Environment is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Leon3 Serial CCSDS Driver for the Test Environment is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Leon3 Serial CCSDS Driver for the Test Environment. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "leon3_serial_ccsds.h"

#include <assert.h>

#include <EscaperInternal.h>

#define MAX_TLS_SIZE RTEMS_ALIGN_UP( 0, RTEMS_TASK_STORAGE_ALIGNMENT )
#define LEON3_SERIAL_CCSDS_POLL_STACK_SIZE (128 > RTEMS_MINIMUM_STACK_SIZE ?  128 : RTEMS_MINIMUM_STACK_SIZE)

rtems_id brokerLock;

static rtems_id leon3SerialCcsdsPoll_TCB = {0};

RTEMS_ALIGNED(RTEMS_TASK_STORAGE_ALIGNMENT) 
    static char leon3SerialCcsdsPoll_TaskBuffer[RTEMS_TASK_STORAGE_SIZE(
    LEON3_SERIAL_CCSDS_POLL_STACK_SIZE + MAX_TLS_SIZE, RTEMS_FLOATING_POINT)];

BYTE_FIFO_CREATE(uartFifoTx, Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE);
BYTE_FIFO_CREATE(uartFifoRx, Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE);

static void UartRxCallback(volatile void *private_data) {
  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;

  rtems_semaphore_release(self->semRx);
}

static void UartTxCallback(volatile void *private_data) {
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

static inline Uart_Parity getUartParity(
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

    self->rxHandler.lengthCallback = UartRxCallback;
    self->rxHandler.characterCallback = UartRxCallback;
    self->rxHandler.characterArg = self;
    self->rxHandler.targetCharacter = STOP_BYTE;
    self->rxHandler.targetLength = Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE;
    self->rxHandler.lengthArg = self;
    
    self->fifoTx = &uartFifoTx;
    self->fifoRx = &uartFifoRx;

    Uart_init(id, &self->uart);
    Uart_setConfig(&self->uart, &config);
    Uart_startup(&self->uart);

    Escaper_init(&self->escaper, self->encodedPacketBuffer,
        Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE,
        self->decodedPacketBuffer,
        Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE);

    const rtems_status_code lockSemaphoreCreateResult = rtems_semaphore_create(
        rtems_build_name('l', 'o', 'c', 'k'),
        1,
        RTEMS_SIMPLE_BINARY_SEMAPHORE,
        RTEMS_PRIORITY_CEILING,
        &brokerLock);
    assert(lockSemaphoreCreateResult == RTEMS_SUCCESSFUL);

    const rtems_status_code rxSemaphoreCreateResult = rtems_semaphore_create(
        rtems_build_name('s', 'm', 'R', 'x'),
        1,
        RTEMS_SIMPLE_BINARY_SEMAPHORE,
        RTEMS_PRIORITY_CEILING,
        &self->semRx);
    assert(rxSemaphoreCreateResult == RTEMS_SUCCESSFUL);

    const rtems_status_code txSemaphoreCreateResult = rtems_semaphore_create(
        rtems_build_name('s', 'm', 'T', 'x'),
        1,
        RTEMS_SIMPLE_BINARY_SEMAPHORE,
        RTEMS_PRIORITY_CEILING,
        &self->semTx);
    assert(txSemaphoreCreateResult == RTEMS_SUCCESSFUL);

    rtems_task_config taskConfig = {
 	    .name = rtems_build_name('p', 'o', 'l', 'l'),
 	    .initial_priority =  1,
      .storage_area = leon3SerialCcsdsPoll_TaskBuffer,
      .storage_size = sizeof(leon3SerialCcsdsPoll_TaskBuffer),
      .maximum_thread_local_storage_size = 0,
      .storage_free = NULL,
      .initial_modes = RTEMS_PREEMPT,
      .attributes = RTEMS_DEFAULT_ATTRIBUTES | RTEMS_FLOATING_POINT
    };

    const rtems_status_code taskConstructionResult = rtems_task_construct(&taskConfig,
                         &leon3SerialCcsdsPoll_TCB);
    assert(taskConstructionResult == RTEMS_SUCCESSFUL);

    const rtems_status_code taskStartStatus = rtems_task_start(leon3SerialCcsdsPoll_TCB,
                    (rtems_task_entry)Leon3SerialCcsdsPoll,
                    (intptr_t)self);
    assert(taskStartStatus == RTEMS_SUCCESSFUL);
}

static inline void Leon3SerialCcsdsPollUartPoll(
    leon3_serial_ccsds_private_data *const self)
{
  size_t length = 0;

  Escaper_start_decoder(&self->escaper);

  while (rtems_semaphore_obtain(self->semRx, RTEMS_WAIT, RTEMS_NO_WAIT) != RTEMS_SUCCESSFUL);

  Uart_readAsync(&self->uart, self->fifoRx, self->rxHandler);

  while (true) {
    while (rtems_semaphore_obtain(self->semRx, RTEMS_WAIT, RTEMS_NO_WAIT) != RTEMS_SUCCESSFUL);

    length = ByteFifo_getCount(self->fifoRx);

    for (size_t i = 0; i < length; i++) {
      ByteFifo_pull(self->fifoRx, &self->recvBuffer[i]);
    }

    Escaper_decode_packet(
        &self->escaper,
        self->ipDeviceBusId,
        self->recvBuffer,
        length, Broker_receive_packet);
  }
}

static inline void Leon3SerialCcsdsSendUartSend(
    leon3_serial_ccsds_private_data *const self,
    const uint8_t *const data,
    const size_t length)
{
  size_t index = 0;
  size_t packetLength = 0;

  Escaper_start_encoder(&self->escaper);
  while (index < length) {
    packetLength =
        Escaper_encode_packet(&self->escaper, data, length, &index);

    while (rtems_semaphore_obtain(self->semTx, RTEMS_WAIT, RTEMS_NO_WAIT) != RTEMS_SUCCESSFUL);

    ByteFifo_initFromBytes(
        self->fifoTx,
        self->encodedPacketBuffer,
        packetLength);
    Uart_writeAsync(&self->uart, self->fifoTx, self->txHandler);
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

  Leon3SerialCcsdsSendUartSend(self, data, length);
}
