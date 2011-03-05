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

volatile uint8_t new_flag = 0;
volatile uint8_t data = 0;

//				i2c_init
//Initializes I2C bus to run at 100kHz and enables the bus
/*void i2c_init(void){
  //DDRD |= (1<<PD0) | (1<<PD1);
  //PORTD |= (1<<PD0) | (1<<PD1);
  //TWBR = 0x12;
  TWAR = 0xFE;
  TWCR = (1<<TWEN) | (1<<TWEA);
}*/
void i2c_init(void){
  DDRD |= (1<<PD0) | (1<<PD1);
  //DDRD &= ~(1<<PD1);
  TWAR = 0xFE;
  TWCR = (1<<TWEN) | (1<<TWEA) | (1<<TWIE);
}

ISR(TWI_vect)
{
  //uint8_t status = TWSR & 0xF8;
  if((TWSR & 0xF8) == 0x60){
    TWCR = (1<<TWEA) | (1<<TWINT);
  }
  else if((TWSR & 0xF8) == 0x80){
    new_flag = TRUE;
    data = TWDR;
    TWCR = (1<<TWEA) | (1<<TWINT);
  }
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

/*int main(){
  spi_init();
  DDRF |= (1<<PF3);  //Set up LCD pin
  lcd_init();
  i2c_init();
  while(1){
    while(!(TWCR & (1<<TWINT))){}
    TWCR |= (1<<TWINT) | (1<<TWEA);
    while(!(TWCR & (1<<TWINT))){}
	TWCR = (1<<TWEN) | (1<<TWEA); //This may or may not do anything
	uint8_t data = TWDR;
    char string[20];
	clear_display();
	cursor_home();
    sprintf(string,"Recieved: %X",data);
    string2lcd(string);
  }
  return 0;
}*/

int main(){
  spi_init();
  DDRF |= (1<<PF3); //Set up LCD pin
  DDRD |= (1<<PD2) | (1<<PD3);
  PORTD &= ~((1<<PD2) | (1<<PD3));
  lcd_init();
  i2c_init();
  sei();
  while(1)
  {
    //if(new_flag == TRUE){
	  new_flag = FALSE;
	  char string[20];
	  clear_display();
	  cursor_home();
	  sprintf(string, "Recieved: %X", TWDR);
	  string2lcd(string);
	//}
	_delay_ms(100);
  }
  return 0;
}