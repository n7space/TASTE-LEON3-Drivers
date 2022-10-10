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