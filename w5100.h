/*
 * This file is part of arduino-netboot.
 * Copyright 2011 Emil Renner Berthing
 *
 * arduino-netboot is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or(at your option) any later version.
 *
 * arduino-netboot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with arduino-netboot.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _W5100_H
#define _W5100_H

/*
 * Common Registers
 */

#define WIZ_MR     0x00
#define WIZ_GAR    0x01
#define WIZ_SUBR   0x05
#define WIZ_SHAR   0x09
#define WIZ_SIPR   0x0F
#define WIZ_IR     0x15
#define WIZ_IMR    0x16
#define WIZ_RTR    0x17
#define WIZ_RCR    0x19
#define WIZ_RMSR   0x1A
#define WIZ_TMSR   0x1B
#define WIZ_PATR   0x1C
#define WIZ_PTIMER 0x28
#define WIZ_PMAGIC 0x29
#define WIZ_UIPR   0x2A
#define WIZ_UPORT  0x2E


/*
 * Socket Registers
 */

#define WIZ_SOCKET0_OFFSET  0x400
#define WIZ_SOCKET1_OFFSET  0x500
#define WIZ_SOCKET2_OFFSET  0x600
#define WIZ_SOCKET4_OFFSET  0x600

#define WIZ_SOCKET0_TX_BASE 0x4000
#define WIZ_SOCKET0_RX_BASE 0x6000

#define WIZ_Sn_MR(n)     (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x00)
#define WIZ_Sn_CR(n)     (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x01)
#define WIZ_Sn_IR(n)     (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x02)
#define WIZ_Sn_SR(n)     (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x03)
#define WIZ_Sn_PORT(n)   (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x04)
#define WIZ_Sn_DHAR(n)   (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x06)
#define WIZ_Sn_DIPR(n)   (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x0C)
#define WIZ_Sn_DPORT(n)  (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x10)
#define WIZ_Sn_MSSR(n)   (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x12)
#define WIZ_Sn_PROTO(n)  (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x14)
#define WIZ_Sn_TOS(n)    (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x15)
#define WIZ_Sn_TTL(n)    (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x16)
#define WIZ_Sn_TX_FSR(n) (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x20)
#define WIZ_Sn_TX_RD(n)  (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x22)
#define WIZ_Sn_TX_WR(n)  (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x24)
#define WIZ_Sn_RX_RSR(n) (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x26)
#define WIZ_Sn_RX_RD(n)  (WIZ_SOCKET0_OFFSET + (n)*0x100 + 0x28)


/*
 * Socket Flags
 */

#define WIZ_MULTI  0x80
#define WIZ_NDMC   0x20
#define WIZ_TCP    0x01
#define WIZ_UDP    0x02
#define WIZ_IPRAW  0x03
#define WIZ_MACRAW 0x03
#define WIZ_PPoE   0x03


#define WIZ_OPEN      0x01
#define WIZ_LISTEN    0x02
#define WIZ_CONNECT   0x04
#define WIZ_DISCON    0x08
#define WIZ_CLOSE     0x10
#define WIZ_SEND      0x20
#define WIZ_SEND_MAC  0x21
#define WIZ_SEND_KEEP 0x22
#define WIZ_RECV      0x40

#define WIZ_SOCK_CLOSED      0x00
#define WIZ_SOCK_INIT        0x13
#define WIZ_SOCK_LISTEN      0x14
#define WIZ_SOCK_ESTABLISHED 0x17
#define WIZ_SOCK_CLOSE_WAIT  0x1C
#define WIZ_SOCK_UDP         0x22
#define WIZ_SOCK_IPRAW       0x32
#define WIZ_SOCK_MACRAW      0x42
#define WIZ_SOCK_PPOE        0x5F

#define WIZ_SOCK_SYNSENT     0x15
#define WIZ_SOCK_SYNRECV     0x16
#define WIZ_SOCK_FIN_WAIT    0x18
#define WIZ_SOCK_CLOSING     0x1A
#define WIZ_SOCK_TIME_WAIT   0x1B
#define WIZ_SOCK_LAST_ACK    0x1C
#define WIZ_SOCK_ARP         0x01

#endif
