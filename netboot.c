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

#include <stdint.h>
#include <avr/pgmspace.h>

#include <arduino/pins.h>
#include <arduino/spi.h>

#include "boot.h"
#include "w5100.h"

#define __noinit __attribute__((section (".noinit")))
#define hton(n) (((n) << 8) | ((n) >> 8))

#define LED   9

#define SS   10 /* slave select         */
#define MOSI 11 /* master out, slave in */
#define MISO 12 /* master in, slave out */
#define SCK  13 /* clock                */

#define BOOTP_CLIENT_PORT 68
#define BOOTP_SERVER_PORT 67

#define BOOTP_BOOTREQUEST    1
#define BOOTP_BOOTREPLY      2
#define BOOTP_ETHERNET       1
#define BOOTP_MULTICAST_HIGH 0x80

#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5

#define TFTP_PORT 69
#define TFTP_SOURCE_PORT 2000 /* choose your favourite number */

#define MASK_2K 0x7FF
#define SIZE_2K 0x800

#define MAC_ADDRESS { 0x00, 0x08, 0xDC, 0x00, 0x00, 0x4F }

#define XID0 0x12
#define XID1 0x34
#define XID2 0x56
#define XID3 0x78

union word {
	uint16_t word;
	struct {
		uint8_t low;
		uint8_t high;
	} byte;
};

struct wiz_udp_header {
	uint8_t ip[4];
	uint16_t port;
	uint16_t size;
};

#define BOOTP_MAX_FILENAME 128
struct bootp {
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint8_t xid[4];
	uint8_t secs[2];
	uint8_t flags[2];
	uint8_t ciaddr[4];
	uint8_t yiaddr[4];
	uint8_t siaddr[4];
	uint8_t giaddr[4];
	uint8_t chaddr[16];
	uint8_t sname[64];
	char file[BOOTP_MAX_FILENAME];
	uint8_t vend[64];
};

struct tftp {
	uint16_t op;
	uint16_t block;
	uint8_t data[512];
};

static void
mymemset(uint8_t *dst, uint8_t c, uint16_t len)
{
	for (; len; len--)
		*dst++ = c;
}

static inline void
watchdog_reset(void)
{
	__asm__ __volatile__ ("wdr\n");
}

static inline void
watchdog_config(uint8_t v)
{
	WDTCSR = _BV(WDCE) | _BV(WDE);
	WDTCSR = v;
}

static inline void
watchdog_noreset_500ms(void)
{
	MCUSR &= ~(_BV(WDRF));
	watchdog_config(_BV(WDIE) | _BV(WDP2) | _BV(WDP0));
}

static inline void
watchdog_off(void)
{
	watchdog_config(0x00);
}

static inline uint8_t
watchdog_interrupt_flag(void)
{
	return WDTCSR & _BV(WDIF);
}

static inline void
watchdog_interrupt_flag_clear(void)
{
	WDTCSR |= _BV(WDIF);
}

static void
watchdog_wait(void)
{
	while(!watchdog_interrupt_flag());
	watchdog_interrupt_flag_clear();
}

static void
spi_wait(void)
{
	while (!spi_interrupt_flag());
}

static uint8_t
wiz_get(uint16_t reg)
{
	union word wreg = { .word = reg };

	pin_low(SS);
	spi_write(0x0F);
	spi_wait();
	spi_write(wreg.byte.high);
	spi_wait();
	spi_write(wreg.byte.low);
	spi_wait();
	spi_write(0x00);
	spi_wait();
	pin_high(SS);

	return spi_read();
}

static void
wiz_set(uint16_t reg, uint8_t data)
{
	union word wreg = { .word = reg };

	pin_low(SS);
	spi_write(0xF0);
	spi_wait();
	spi_write(wreg.byte.high);
	spi_wait();
	spi_write(wreg.byte.low);
	spi_wait();
	spi_write(data);
	spi_wait();
	pin_high(SS);
}

static uint16_t
wiz_get_word(uint16_t reg)
{
	union word ret;

	ret.byte.high = wiz_get(reg++);
	ret.byte.low = wiz_get(reg++);
	return ret.word;
}

static void
wiz_set_word(uint16_t reg, uint16_t data)
{
	union word wdata = { .word = data };

	wiz_set(reg++, wdata.byte.high);
	wiz_set(reg++, wdata.byte.low);
}

static void
wiz_memcpy(uint16_t reg, const uint8_t *data, uint8_t len)
{
	for (; len; len--)
		wiz_set(reg++, *data++);
}

static void
wiz_memcpy_P(uint16_t reg, const uint8_t *data, uint8_t len)
{
	for (; len; len--)
		wiz_set(reg++, pgm_read_byte(data++));
}

static void
wiz_memset(uint16_t reg, uint8_t c, uint8_t len)
{
	for (; len; len--)
		wiz_set(reg++, c);
}

static const uint8_t mac_addr[] PROGMEM = MAC_ADDRESS;

static union {
	uint8_t a[sizeof(struct wiz_udp_header)];
	struct wiz_udp_header s;
} in_header __noinit;

static union {
		uint8_t a[sizeof(struct tftp)];
		struct bootp bootp;
		struct tftp tftp;
} in __noinit;

static union {
	uint8_t a[sizeof(struct bootp)];
	struct bootp bootp;
} out __noinit;

static uint16_t out_size __noinit;
static union word block __noinit;

#ifdef NDEBUG
static inline void debug_init(void) {}
#define printf(...)

static inline int __attribute__((noreturn))
exit_bootloader(void)
{
	watchdog_reset();
	watchdog_off();
	__asm__ __volatile__ ("jmp 0\n");
	__builtin_unreachable();
}

static void
writepage(uint16_t addr, const uint8_t *data)
{
	uint16_t i;

	boot_page_erase(addr);
	boot_spm_busy_wait();

	for (i = 0; i < SPM_PAGESIZE; i += 2) {
		union word w;

		w.byte.low = *data++;
		w.byte.high = *data++;

		boot_page_fill(addr + i, w.word);
	}

	boot_page_write(addr);
	boot_spm_busy_wait();
	boot_rww_enable();
}

#else /* !NDEBUG */

#include "serial_busy_output.c"
/*#include "serial_sync_output.c"*/
/*#include "serial_async_output.c"*/

static void
debug_init(void)
{
	serial_baud_9600();
	serial_mode_8n1();
	serial_init_stdout();
}

static int __attribute__((noreturn))
exit_bootloader(void)
{
	printf("EXIT\r\n");
	while (1) {
		pin_toggle(LED);
		watchdog_wait();
	}
}

static void
writepage(uint16_t addr, const uint8_t *data)
{
	(void)data;
	printf("Flashing 0x%04X\r\n", addr);
}
#endif

static void
sock0_open(uint16_t port)
{
	while (1) {
		printf("Opening UDP socket");

		/* set multicast UDP mode for socket 0 */
		wiz_set(WIZ_Sn_MR(0), WIZ_UDP);
		/* set source port */
		wiz_set_word(WIZ_Sn_PORT(0), port);
		/* open socket */
		wiz_set(WIZ_Sn_CR(0), WIZ_OPEN);

		if (wiz_get(WIZ_Sn_SR(0)) == WIZ_SOCK_UDP)
			break;

		printf(", error: status = 0x%02hx\r\n", wiz_get(WIZ_Sn_SR(0)));
		wiz_set(WIZ_Sn_CR(0), WIZ_CLOSE);
	}
	printf("\r\n");
}

static void
sock0_close(void)
{
	wiz_set(WIZ_Sn_CR(0), WIZ_CLOSE);
}

static void
sock0_sendpacket(void)
{
	uint16_t tx_addr = WIZ_SOCKET0_TX_BASE
		+ (wiz_get_word(WIZ_Sn_TX_WR(0)) & MASK_2K);
	uint16_t i;

	for (i = 0; i < out_size; i++) {
		wiz_set(tx_addr++, out.a[i]);
		if (tx_addr == WIZ_SOCKET0_TX_BASE + SIZE_2K)
			tx_addr = WIZ_SOCKET0_TX_BASE;
	}

	/* S0_TX_WR += len */
	wiz_set_word(WIZ_Sn_TX_WR(0),
	               wiz_get_word(WIZ_Sn_TX_WR(0)) + out_size);

	/* send! */
	wiz_set(WIZ_Sn_CR(0), WIZ_SEND);
	while (wiz_get(WIZ_Sn_CR(0)) != 0);
}

static uint16_t
sock0_rx_read(uint16_t rx_addr, uint8_t *buf, uint16_t len)
{
	for (; len; len--) {
		*buf++ = wiz_get(rx_addr++);
		if (rx_addr == WIZ_SOCKET0_RX_BASE + SIZE_2K)
			rx_addr = WIZ_SOCKET0_RX_BASE;
	}

	return rx_addr;
}

static void
sock0_readpacket(void)
{
	uint16_t rx_addr = WIZ_SOCKET0_RX_BASE
		+ (wiz_get_word(WIZ_Sn_RX_RD(0)) & MASK_2K);
	uint16_t size;

	rx_addr = sock0_rx_read(rx_addr, in_header.a,
			sizeof(struct wiz_udp_header));

	/* in_header.s.port = hton(in_header.s.port); */
	in_header.s.size = hton(in_header.s.size);

	printf("\r\nReceived udp packet of size %u\r\n",
			in_header.s.size);

	size = in_header.s.size + sizeof(struct wiz_udp_header);
	while (wiz_get_word(WIZ_Sn_RX_RSR(0)) < size);

	(void)sock0_rx_read(rx_addr, in.a, in_header.s.size > sizeof(in) ?
			sizeof(in) : in_header.s.size);

	/* S0_RX_RD += size */
	wiz_set_word(WIZ_Sn_RX_RD(0),
	               wiz_get_word(WIZ_Sn_RX_RD(0)) + size);

	wiz_set(WIZ_Sn_CR(0), WIZ_RECV);
}

static uint8_t
eb_send(uint8_t (*check)(void))
{
	uint8_t next;

	for (next = 1; next < 30; next <<= 1) {
		uint8_t i;

		printf(">");
		sock0_sendpacket();

		i = next;
		watchdog_reset();
		do {
			if (wiz_get_word(WIZ_Sn_RX_RSR(0)) > 0) {
				sock0_readpacket();
				if (check())
					return 0;
			}

			if (watchdog_interrupt_flag()) {
				watchdog_interrupt_flag_clear();
				printf(".");
				i--;
			}
		} while (i);
	}

	return 1;
}

static void
bootp_prepare(void)
{
	uint8_t i;

	mymemset(out.a, 0, sizeof(struct bootp));

	out.bootp.op = BOOTP_BOOTREQUEST;
	out.bootp.htype = BOOTP_ETHERNET;
	out.bootp.hlen = 6;  /* hardware address length */
	out.bootp.xid[0] = XID0;
	out.bootp.xid[1] = XID1;
	out.bootp.xid[2] = XID2;
	out.bootp.xid[3] = XID3;
	out.bootp.flags[0] = BOOTP_MULTICAST_HIGH;
	for (i = 0; i < sizeof(mac_addr); i++)
		out.bootp.chaddr[i] = pgm_read_byte(&(mac_addr[i]));

	out_size = sizeof(struct bootp);
}

static uint8_t
bootp_check(void)
{
	if (in_header.s.port == hton(BOOTP_SERVER_PORT) &&
            in_header.s.size >= sizeof(struct bootp) &&
	    in.bootp.op == BOOTP_BOOTREPLY &&
	    in.bootp.xid[0] == XID0 &&
	    in.bootp.xid[1] == XID1 &&
	    in.bootp.xid[2] == XID2 &&
	    in.bootp.xid[3] == XID3)
		return 1;

	return 0;
}

static uint8_t
get_address(void)
{
	sock0_open(BOOTP_CLIENT_PORT);

	/* set destination IP to broadcast address */
	wiz_memset(WIZ_Sn_DIPR(0), 0xFF, 4);
	wiz_set_word(WIZ_Sn_DPORT(0), BOOTP_SERVER_PORT);

	bootp_prepare();

	printf("BOOTREQUEST");
	if (eb_send(bootp_check)) {
		printf("\r\nGiving up :(\r\n");
		return 1;
	}

	printf("\r\nBOOTREPLY:\r\n"
	       "  ciaddr: %hu.%hu.%hu.%hu\r\n"
	       "  yiaddr: %hu.%hu.%hu.%hu\r\n"
	       "  siaddr: %hu.%hu.%hu.%hu\r\n"
	       "  giaddr: %hu.%hu.%hu.%hu\r\n"
	       "  file:   %s\r\n",
	       in.bootp.ciaddr[0],
	       in.bootp.ciaddr[1],
	       in.bootp.ciaddr[2],
	       in.bootp.ciaddr[3],
	       in.bootp.yiaddr[0],
	       in.bootp.yiaddr[1],
	       in.bootp.yiaddr[2],
	       in.bootp.yiaddr[3],
	       in.bootp.siaddr[0],
	       in.bootp.siaddr[1],
	       in.bootp.siaddr[2],
	       in.bootp.siaddr[3],
	       in.bootp.giaddr[0],
	       in.bootp.giaddr[1],
	       in.bootp.giaddr[2],
	       in.bootp.giaddr[3],
	       in.bootp.file);

	sock0_close();

	/* set our IP address */
	wiz_memcpy(WIZ_SIPR, in.bootp.yiaddr, sizeof(in.bootp.yiaddr));

	return 0;
}

static void
tftp_rrq_prepare(void)
{
	static const char octet[] PROGMEM = "octet";
	uint8_t *p = out.a;
	const char *q;

	*p++ = 0;
	*p++ = TFTP_RRQ;
	for (q = in.bootp.file; (*p++ = *q++););
	for (q = octet;         (*p++ = pgm_read_byte(q++)););

	out_size = p - out.a;
}

static void
tftp_ack_prepare(void)
{
	uint8_t *p = out.a;

	*p++ = 0;
	*p++ = TFTP_ACK;
	*p++ = block.byte.high;
	*p++ = block.byte.low;

	out_size = 4;
}

static uint8_t
tftp_data_check(void)
{
	uint16_t inblock;

	if (in.tftp.op == hton(TFTP_ERROR)) {
		printf("Error code %u: %s\r\n",
		       hton(in.tftp.block),
		       (char *)(in.tftp.data));
		return 1;
	}

	if (in.tftp.op != hton(TFTP_DATA))
		return 0;

	inblock = hton(in.tftp.block);
	printf("block = %u\r\n", inblock);
	if (inblock != block.word + 1)
		return 0;

	block.word++;

	return 1;
}

static inline uint16_t
tftp_data_len(void)
{
	return in_header.s.size - 4;
}


static uint8_t
pagecmp(uint16_t addr, uint8_t *data)
{
	uint16_t i;

	for (i = 0; i < SPM_PAGESIZE; i++) {
		if (pgm_read_byte(addr++) != *data++)
			return 1;
	}

	return 0;
}

static void
prog(void)
{
	uint16_t addr = (block.word - 1) * 512;
	uint16_t i;

	for (i = 0; i < tftp_data_len(); i += SPM_PAGESIZE) {
		uint16_t j;

		for (j = tftp_data_len(); j < i + SPM_PAGESIZE; j++)
			in.tftp.data[j] = 0xFF;

		if (pagecmp(addr + i, in.tftp.data + i)) {
			pin_high(LED);
			writepage(addr + i, in.tftp.data + i);
			pin_low(LED);
		}
	}
}

static uint8_t
tftp_get(void)
{
	sock0_open(TFTP_SOURCE_PORT);

	/* set destination IP and port */
	wiz_memcpy(WIZ_Sn_DIPR(0), in.bootp.siaddr, sizeof(in.bootp.siaddr));
	wiz_set_word(WIZ_Sn_DPORT(0), TFTP_PORT);

	tftp_rrq_prepare();
	block.word = 0;

	printf("RRQ");
	if (eb_send(tftp_data_check))
		return 1;
	printf("\r\n");

	if (in.tftp.op == hton(TFTP_ERROR))
		return 1;

	prog();

	wiz_set_word(WIZ_Sn_DPORT(0), hton(in_header.s.port));

	while (in_header.s.size == 516) {
		printf("ACK");
		tftp_ack_prepare();
		if (eb_send(tftp_data_check))
			return 1;
		printf("\r\n");

		if (in.tftp.op == hton(TFTP_ERROR))
			return 1;

		prog();
	}

	tftp_ack_prepare();
	sock0_sendpacket();

	return 0;
}

int
main(void)
{
	watchdog_reset();
	watchdog_noreset_500ms();

	debug_init();

#if 0 /* this is what's going on */
	pin_mode_output(LED); pin_low(LED);

	pin_mode_output(SS); pin_high(SS);
	pin_mode_output(MOSI);
	pin_mode_input(MISO);
	pin_mode_output(SCK);
#else /* this saves a few bytes */
	DDRB = 0xEF;
	PORTB = 0x04;
#endif

	spi_mode_master();
	spi_clock_d64();
	spi_enable();

	printf("\r\n\r\nResetting chip");
	wiz_set(WIZ_MR, 0x80);

	/* wait for it to initialize */
	watchdog_wait();
	printf("\r\n");

	/* set MAC address */
	wiz_memcpy_P(WIZ_SHAR, mac_addr, sizeof(mac_addr));

	/* not needed after chip reset
	wiz_memset(WIZ_SIPR, 0x00, 4);
	wiz_memset(WIZ_GAR,  0x00, 4);
	wiz_memset(WIZ_SUBR, 0x00, 4);
	*/

	if (get_address())
		goto exit;

	if (tftp_get())
		goto exit;

	printf("Success!\r\n");

exit:
	return exit_bootloader();
}
