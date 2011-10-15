## Name your project
NAME = netboot

## Point this to the directory where you did
##   git clone git://github.com/esmil/oniudra-headers.git arduino
ARDUINO_HEADERS = .

## Change this according to your code to make the tty and cat targets work
BAUD     = 9600
MODE     = $(MODE_RAW) $(MODE_8) $(MODE_N) $(MODE_1)# 8N1

## Uncomment your arduino version below

## Duemilanove
#MCU       = atmega328p
#F_CPU     = 16000000UL
#PORT      = /dev/ttyUSB0
#PROG      = arduino
#PROG_BAUD = 57600

## Uno
MCU       = atmega328p
F_CPU     = 16000000UL
PORT      = /dev/ttyACM0
PROG      = arduino
PROG_BAUD = 115200

CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
NM      = avr-nm
AVRDUDE = avrdude
STTY    = stty
SED     = sed
CAT     = cat
STAT    = stat

MODE_RAW = raw -echo -hup
MODE_7   = cs7
MODE_8   = cs8
MODE_N   = -parenb
MODE_E   = parenb -parodd
MODE_O   = parenb parodd
MODE_1   = -cstopb
MODE_2   = cstopb

PROG_arduino = -D -P$(PORT) -b$(PROG_BAUD)
PROG_avrispmkII = -Pusb

OPT        = s
CFLAGS     = -O$(OPT) -pipe -gdwarf-2
CFLAGS    += -mmcu=$(MCU) -DF_CPU=$(F_CPU) -I$(ARDUINO_HEADERS)
CFLAGS    += -std=gnu99 -fstrict-aliasing
CFLAGS    += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
## Uncomment to create listing file
#CFLAGS    += -Wa,-adhlns=$(<:.c=.lst)
CFLAGS    += -Wall -Wextra -Wno-variadic-macros -pedantic

LDFLAGS    = -Wl,--relax
## Uncomment to create a map file
#LDFLAGS   += -Wl,-Map=$(NAME).map,--cref
## Uncomment for minimal printf suport
#LDFLAGS   += -Wl,-u,vfprintf -lprintf_min
## Uncomment for printf support with floating point (needs -lm below)
#LDFLAGS   += -Wl,-u,vfprintf -lprintf_flt
## Uncomment for minimal scanf support
#LDFLAGS   += -Wl,-u,vfscanf -lscanf_min
## Uncomment for scanf support with floating point (needs -lm below)
#LDFLAGS   += -Wl,-u,vfscanf -lscanf_flt
## Uncomment for trigonometry and other floating point functions
#LDFLAGS   += -lm

.PHONY: all list fuses tty cat
.PRECIOUS: %.elf

all: $(NAME).hex

bootloader: CFLAGS += -DNDEBUG
bootloader: LDFLAGS += -Wl,--section-start=.text=0x7800,--section-start,.noinit=0x800100,--gc-sections -nostartfiles -nodefaultlibs
bootloader: $(NAME).hex

%.elf: $(or $(FILES),%.c)
	@echo '  CC $@'
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

hello_world.elf: hello_world.c
	@echo '  CC $@'
	@$(CC) $(CFLAGS) $< netboot.s -o $@ $(LDFLAGS)

%.hex: %.elf
	@echo '  OBJCOPY $@'
	@$(OBJCOPY) -O ihex -R .eeprom -S $< $@
	@echo "  $$((0x$$($(OBJDUMP) -h $@ | $(SED) -n '6{s/^  0 \.sec1         //;s/ .*//;p}'))) bytes"

%.bin: %.elf
	@echo '  OBJCOPY $@'
	@$(OBJCOPY) -O binary -R .eeprom -S $< $@
	@$(STAT) -c '  %s bytes' $@

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo '  OBJDUMP > $@'
	@$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo '  NM $@'
	@$(NM) -n $< > $@

upload: $(NAME).hex
	$(AVRDUDE) -v -p$(MCU) -c$(PROG) $(PROG_$(PROG)) -Uflash:w:$<:i

fuses:
	$(AVRDUDE) -v -p$(MCU) -c$(PROG) $(PROG_$(PROG)) -Ulfuse:w:0xFF:m -Uhfuse:w:0xDA:m -Uefuse:w:0x05:m

list: $(NAME).lss

tty: $(PORT)
	@echo '  STTY -F$(PORT) $(MODE) $(BAUD)'
	@$(STTY) -F$(PORT) $(MODE) $(BAUD)

cat: $(PORT)
	@$(CAT) $(PORT)

clean:
	rm -f *.elf *.hex *.bin *.map *.lst *.lss *.sym
