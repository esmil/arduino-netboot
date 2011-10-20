#include <EEPROM.h>

#define EEPROM_SIZE 1024

static void setMAC(uint8_t *mac)
{
	uint16_t addr = EEPROM_SIZE - 8;
	uint8_t i;
	uint8_t sum = 0;

	for (i = 0; i < 6; i++)
		sum += mac[i];

	EEPROM.write(addr++, 0);
	EEPROM.write(addr++, 17 - sum);
	for (i = 0; i < 6; i++)
		EEPROM.write(addr++, mac[i]);
}

void setup(void) {
	uint8_t MACAddress[] = { 0x90, 0xA2, 0xDA, 0x00, 0x61, 0xEE };

	Serial.begin(9600);

	Serial.print("Please wait while I set the MAC address\n");
	setMAC(MACAddress);
	Serial.print("Done setting MAC address, "
		"you are free to continue whatever you want to do now.\n");
}

void loop(void) {
}

// vim: set syntax=cpp :
