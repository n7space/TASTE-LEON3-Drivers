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

#include <stdint.h>

typedef uint64_t asn1SccUint64;
typedef asn1SccUint64 asn1SccUint;
typedef bool flag;

typedef enum { uart0, uart1, uart2, uart3, uart4, uart5 } Serial_CCSDS_Leon3_Device_T;
typedef asn1SccUint Serial_CCSDS_Leon3_Conf_T_bits;
typedef flag Serial_CCSDS_Leon3_Conf_T_use_paritybit;

typedef enum
{
  Serial_CCSDS_Leon3_Baudrate_T_b9600 = 0,
  Serial_CCSDS_Leon3_Baudrate_T_b19200 = 1,
  Serial_CCSDS_Leon3_Baudrate_T_b38400 = 2,
  Serial_CCSDS_Leon3_Baudrate_T_b57600 = 3,
  Serial_CCSDS_Leon3_Baudrate_T_b115200 = 4,
  Serial_CCSDS_Leon3_Baudrate_T_b230400 = 5
} Serial_CCSDS_Leon3_Baudrate_T;

typedef enum {
    Serial_CCSDS_Leon3_Parity_T_even = 0,
    Serial_CCSDS_Leon3_Parity_T_odd = 1
} Serial_CCSDS_Leon3_Parity_T;

typedef struct
{
  Serial_CCSDS_Leon3_Device_T devname;
  Serial_CCSDS_Leon3_Baudrate_T speed;
  Serial_CCSDS_Leon3_Parity_T parity;
  Serial_CCSDS_Leon3_Conf_T_bits bits;
  Serial_CCSDS_Leon3_Conf_T_use_paritybit use_paritybit;

  struct
  {
    unsigned int speed : 1;
    unsigned int parity : 1;
    unsigned int bits : 1;
    unsigned int use_paritybit : 1;
  } exist;

} Serial_CCSDS_Leon3_Conf_T;