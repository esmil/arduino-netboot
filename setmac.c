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
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <arduino/pins.h>

#define EEPROM_SIZE 1024

#define LED 9

#include "serial_busy_output.c"

static void
setmac(uint8_t *mac)
{
	uint8_t *p = (uint8_t *)(EEPROM_SIZE - 8);
	uint8_t i;
	uint8_t sum = 0;

	for (i = 0; i < 6; i++)
		sum += mac[i];

	eeprom_update_byte(p++, 0);
	eeprom_update_byte(p++, 17 - sum);
	for (i = 0; i < 6; i++)
		eeprom_update_byte(p++, mac[i]);
}

int
main(void)
{
	uint8_t mac_addr[] = { 0x90, 0xA2, 0xDA, 0x00, 0x61, 0xEE };

	serial_baud_9600();
	serial_mode_8n1();
	serial_init_stdout();

	pin_mode_output(LED);
	pin_low(LED);

	printf("Writing %02hX:%02hX:%02hX:%02hX:%02hX:%02hX to EEPROM.. ",
			mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);
	setmac(mac_addr);
	printf("done\r\n");

	while (1) {
		pin_toggle(LED);
		_delay_ms(500.0);
	}
}
