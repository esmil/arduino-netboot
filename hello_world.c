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

#include <arduino/pins.h>

#define LED 9

#include "serial_busy_output.c"

#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] PROGMEM = (s); &__c[0];}))

int
main(void)
{
	serial_baud_9600();
	serial_mode_8n1();
	serial_init_stdout();

	pin_mode_output(LED);

	while (1) {
		pin_toggle(LED);
		printf_P(PSTR("Hello, World!\r\n"));
		_delay_ms(500.0);
	}
}
