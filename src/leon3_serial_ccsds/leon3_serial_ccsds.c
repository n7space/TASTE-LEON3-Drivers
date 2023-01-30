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

#include "leon3_serial_ccsds.h"

#include <assert.h>

#include <EscaperInternal.h>

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

static inline Uart_interrupt interruptNumber(Uart_Id id)
{
    switch(id) {
        case Uart_Id_0:
            return Uart0_interrupt;
        case Uart_Id_1:
            return Uart1_interrupt;
        case Uart_Id_2:
            return Uart2_interrupt;
        case Uart_Id_3:
            return Uart3_interrupt;
        case Uart_Id_4:
            return Uart4_interrupt;
        case Uart_Id_5:
            return Uart5_interrupt;
        default:
            return UartInvalid_interrupt;
    }
}

static inline Uart_BaudRate getUartBaudRate(
    const Serial_CCSDS_Leon3_Conf_T *const deviceConfiguration)
{
  switch(deviceConfiguration->speed) {
      case Serial_CCSDS_Leon3_Baudrate_T_b9600:
          return Uart_BaudRate_9600;
      case Serial_CCSDS_Leon3_Baudrate_T_b19200:
          return Uart_BaudRate_19200;
      case Serial_CCSDS_Leon3_Baudrate_T_b38400:
          return Uart_BaudRate_38400;
      case Serial_CCSDS_Leon3_Baudrate_T_b57600:
          return Uart_BaudRate_57600;
      case Serial_CCSDS_Leon3_Baudrate_T_b115200:
          return Uart_BaudRate_115200;
      default:
          return Uart_BaudRate_Invalid;
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
    config.baudRate = getUartBaudRate(deviceConfiguration);
    config.parity = getUartParity(deviceConfiguration);

    self->txHandler.callback = UartTxCallback;
    self->txHandler.arg = self;

    self->rxHandler.lengthCallback = UartRxCallback;
    self->rxHandler.characterCallback = UartRxCallback;
    self->rxHandler.characterArg = self;
    self->rxHandler.targetCharacter = STOP_BYTE;
    self->rxHandler.targetLength = Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE / 2;
    self->rxHandler.lengthArg = self;

    Uart_init(id, &self->uart);
    Uart_setConfig(&self->uart, &config);

    Escaper_init(&self->escaper, self->encodedPacketBuffer,
        Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE,
        self->decodedPacketBuffer,
        Serial_CCSDS_LEON3_DECODED_PACKET_MAX_SIZE);

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
      .storage_area = self->leon3SerialCcsdsPoll_TaskBuffer,
      .storage_size = sizeof(self->leon3SerialCcsdsPoll_TaskBuffer),
      .maximum_thread_local_storage_size = UART_TLS_SIZE,
      .storage_free = NULL,
      .initial_modes = RTEMS_PREEMPT,
      .attributes = RTEMS_DEFAULT_ATTRIBUTES | RTEMS_FLOATING_POINT
    };

    const rtems_status_code taskConstructionResult = rtems_task_construct(&taskConfig,
                         &self->leon3SerialCcsdsPoll_TCB);
    assert(taskConstructionResult == RTEMS_SUCCESSFUL);

    Uart_startup(&self->uart);

    const rtems_status_code taskStartStatus = rtems_task_start(self->leon3SerialCcsdsPoll_TCB,
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

  ByteFifo_init(&self->fifoRx, &self->fifoMemoryBlockRx, Serial_CCSDS_LEON3_ENCODED_PACKET_MAX_SIZE);
  Uart_readAsync(&self->uart, &self->fifoRx, self->rxHandler);

  while (true) {
    while (rtems_semaphore_obtain(self->semRx, RTEMS_WAIT, RTEMS_NO_WAIT) != RTEMS_SUCCESSFUL);

    rtems_interrupt_vector_disable(interruptNumber(self->uart.id));
    length = ByteFifo_getCount(&self->fifoRx);
    rtems_interrupt_vector_enable(interruptNumber(self->uart.id));

    for (size_t i = 0; i < length; i++) {
      rtems_interrupt_vector_disable(interruptNumber(self->uart.id));
      ByteFifo_pull(&self->fifoRx, &self->recvBuffer[i]);
      rtems_interrupt_vector_enable(interruptNumber(self->uart.id));
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
        &self->fifoTx,
        self->encodedPacketBuffer,
        packetLength);
    Uart_writeAsync(&self->uart, &self->fifoTx, self->txHandler);
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
