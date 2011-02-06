/*
  i2c-driver.c
  
  This is a driver file to use I2C on the xmega128a3.
  
*/
#include <stdio.h>
#include <stddef.h>
#include <avr/io.h>
#define F_CPU 32000000UL
#include <util/delay.h>
#define SDA 		(1<<0)
#define SCL 		(1<<1)
#define TWI_PORT 	PORTC.PORT
#define TWI_DIR 	PORTC.DIR
#define TWI_PIN 	PORTC.PIN
#define POT_ADDRESS		0b01011110	
#define POT_ADDRESS_READ	(POT_ADDRESS | 0x01)
#define POT_ADDRESS_WRITE	(POT_ADDRESS | 0x00)
#define WIF			6

#define VERSION "0.0.1"

void init_I2C(void){
  //Set up I2C using TWIC (PORTC)
  TWI_DIR |= (SDA | SCL);
  TWIC.MASTER.CTRLA = (1<<3);
  TWIC.MASTER.BAUD = 0x23;
  TWIC.MASTER.STATUS |= 0x01;
}

void set_pot(uint8_t rv){
  //need to spin on status register, WIF bit
  TWIC.MASTER.ADDR = POT_ADDRESS_WRITE;
  while(bit_is_clear(TWIC.MASTER.STATUS, WIF)){};
  
  TWIC.MASTER.DATA = rv;
  while(bit_is_clear(TWIC.MASTER.STATUS, WIF)){};
  
  TWIC.MASTER.CTRLC |= 0x03;
}
