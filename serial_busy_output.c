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

#include <stdio.h>
#include <arduino/serial.h>

static int
serial_putchar(char c, FILE *stream)
{
	(void)stream;
	while (!serial_writeable());
	serial_write(c);
	return 0;
}

static FILE serial_stdout = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);

static inline void
serial_init_stdout(void)
{
	serial_transmitter_enable();
	stdout = &serial_stdout;
}
