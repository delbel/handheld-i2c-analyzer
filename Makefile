PRG            = main.c
DEPS           = i2c-driver.c \
                 lcd-driver.c \
                 button-driver.c

################################################################################

PRG            := $(shell echo $(PRG) | sed 's/\.c//g')
OBJ            := $(PRG).o $(shell echo $(DEPS) | sed 's/\.c/.o/g')

MCU_TARGET     = atxmega128a3
OPTIMIZE       = -O3    # options are 1, 2, 3, s
#OPTIMIZE       = -Os    # options are 1, 2, 3, s

DEFS           =
LIBS           = 

CC             = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,-Map,$(PRG).map

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

all: $(PRG).elf lst text eeprom

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(PRG).c $(LIBS)

clean: 
	rm -rf *.o $(PRG).elf $(PRG).bin $(PRG).hex $(PRG).srec  
	rm -rf *.lst *.map $(PRG)_eeprom.* 

all_clean: 
	rm -rf *.o *.elf *.bin *.hex *.srec  
	rm -rf *.lst *.map *_eeprom.* 

program: $(PRG).hex
ifeq ($(XMEGA_PORT),)
	avrdude -p x128a3 -c avr911 -P COM6 -b 57600 -e -U flash:w:$(PRG).hex 
else
	avrdude -p x128a3 -c avr911 -P $(XMEGA_PORT) -b 57600 -e \
		-U flash:w:$(PRG).hex
endif

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@

