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

BYTE_FIFO_CREATE(uartFifoTx, 256);
BYTE_FIFO_CREATE(uartFifoRx, 256);

static void UartRxCallback(void *private_data) {
  // samv71_serial_ccsds_private_data *self =
  //     (samv71_serial_ccsds_private_data *)private_data;
  // xSemaphoreGiveFromISR(self->m_rx_semaphore, NULL);
}

static void *UartTxCallback(void *private_data) {
  // samv71_serial_ccsds_private_data *self =
  //     (samv71_serial_ccsds_private_data *)private_data;

  // xSemaphoreGiveFromISR(self->m_tx_semaphore, NULL);
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

static inline void getUartParity(const Serial_CCSDS_Leon3_Conf_T *const device_configuration)
{
  if (device_configuration->use_paritybit) {
    if ( device_configuration->parity == Serial_CCSDS_Leon3_Parity_T_odd) {
      return Uart_Parity_Odd;
    } else {
      return Uart_Parity_Even;
    }
  } else {
    return Uart_Parity_None;
  }
}

static inline void Leon3SerialCcsdsInit_uart_init(
    leon3_serial_ccsds_private_data *const self,
    const Serial_CCSDS_Leon3_Conf_T *const device_configuration) {

    Uart_Id id = getUartId(device_configuration->devname);
    Uart_Config config = (Uart_Config){ 0 };
    config.isTxEnabled = true;
    config.isRxEnabled = true;
    config.isLoopbackModeEnabled = false;
    config.parity = getUartParity(device_configuration);

    self->txHandler.callback = UartTxCallback;
    self->txHandler.arg = self;

    self->rxHandler.callback = UartRxCallback;
    self->rxHandler.arg = self;
    
    self->fifoTx = &uartFifoTx;
    self->fifoRx = &uartFifoRx;

    Uart_init(id, &self->uart);
    Uart_setConfig(&self->uart, &config);
    Uart_startup(&self->uart);

    //TODO: add rtems task for poll
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

  self->m_ip_device_bus_id = bus_id;

  Leon3SerialCcsdsInit_uart_init(self, device_configuration);
}

void Leon3SerialCcsdsPoll(void *private_data) {
  leon3_serial_ccsds_private_data *self =
      (leon3_serial_ccsds_private_data *)private_data;
  size_t length = 0;

  Escaper_start_decoder(&self->m_escaper);

  // xSemaphoreTake(self->m_rx_semaphore, portMAX_DELAY);
  // Hal_uart_read(&self->m_hal_uart, self->m_fifo_memory_block,
  //               Serial_CCSDS_SAMV71_RECV_BUFFER_SIZE, self->m_uart_rx_handler);
  // while (true) {
  //   /// Wait for data to arrive. Semaphore will be given
  //   xSemaphoreTake(self->m_rx_semaphore, portMAX_DELAY);

  //   length = ByteFifo_getCount(&self->m_hal_uart.rxFifo);

  //   for (size_t i = 0; i < length; i++) {
  //     SamV71SerialCcsdsInterrupt_rx_disable(self);
  //     ByteFifo_pull(&self->m_hal_uart.rxFifo, &self->m_recv_buffer[i]);
  //     SamV71SerialCcsdsInterrupt_rx_enable(self);
  //   }

  //   Escaper_decode_packet(&self->m_escaper, self->m_ip_device_bus_id, self->m_recv_buffer,
  //                         length, Broker_receive_packet);
  // }
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
    // xSemaphoreTake(self->m_tx_semaphore, portMAX_DELAY);
    
    // ByteFifo_push(self->fifoRx, )

    Uart_writeAsync(&self->uart, self->fifoRx, self->uart->txHandler);
    // Hal_uart_write(&self->m_hal_uart,
    //                (uint8_t *const) & self->m_encoded_packet_buffer,
    //                packetLength, &self->m_uart_tx_handler);
  }
}