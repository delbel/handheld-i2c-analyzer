//slave-main.c

//Code to demo an I2C bus to snoop
//PORTD 0-SCL, 1-SDA

#define F_CPU 16000000
#define TRUE 1
#define FALSE 0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <util/twi.h>
#include <string.h>
#include <stdio.h>
#include "lcd.h"

//				i2c_init
//Initializes I2C bus to run at 100kHz and enables the bus
void i2c_init(void){
//  DDRD |= (1<<PD0) | (1<<PD1);
//  PORTD |= (1<<PD0) | (1<<PD1);
  //TWBR = 0x12;
  TWAR = 0xFE;
  TWCR = (1<<TWEN);
}

//				spi_init
//Initializes the SPI interface for data transfer in master mode, clock=clk/2,
//cycle half phase, low polarity, MSB first, interrupts disabled, poll SPIF bit
//in SPSR to check xmit completion.
void spi_init(void){
  DDRB |= 0x0F;
  SPCR |= (1<<SPE) | (1<<MSTR);
  SPSR |= (1<<SPI2X);
}

int main(){
  spi_init();
  DDRF |= (1<<PF3);  //Set up LCD pin
  DDRD |= (1<<PD2) | (1<<PD3);
  PORTD &= ~((1<<PD2) | (1<<PD3));
  lcd_init();
  i2c_init();
  while(1){
    TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
    while(!(TWCR & (1<<TWINT))){}
    TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
    while(!(TWCR & (1<<TWINT))){}
	  uint8_t data = TWDR;
	  TWCR = (1<<TWINT) | (1<<TWEN);
    char string[20];
	  clear_display();
	  cursor_home();
    sprintf(string,"Recieved: %X",data);
    string2lcd(string);
  }
  return 0;
}
