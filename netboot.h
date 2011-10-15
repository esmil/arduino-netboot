#ifndef _NETBOOT_H
#define _NETBOOT_H

extern uint8_t wiz_get(uint16_t reg);
extern void wiz_set(uint16_t reg, uint8_t data);
extern uint16_t wiz_get_word(uint16_t reg);
extern void wiz_set_word(uint16_t reg, uint16_t data);
extern void wiz_memcpy(uint16_t reg, const uint8_t *data, uint16_t len);

#endif
