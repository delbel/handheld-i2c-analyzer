//master-main.c

//Code to demo an I2C bus to snoop
//PORTD 0-SCL, 1-SDA

#define F_CPU 16000000
#define TRUE 1
#define FALSE 0
#define SLAVE_ADDRESS 0xFE
#define SLAVE_WRITE (SLAVE_ADDRESS | TW_WRITE)
#define SLAVE_READ (SLAVE_ADDRESS | TW_READ)

//define the codes for actions to occur
#define TWCR_START 0xA4 //send start condition
#define TWCR_STOP 0x94 //sent stop condition
#define TWCR_RACK 0xC4 //recieve byte and return ack to slave
#define TWCR_RNACK 0x84 //recieve byte and return nack to slave
#define TWCR_SEND 0x84 //pokes the TWINT flag in TWCR and TWEN

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <util/twi.h>
#include <string.h>
#include <stdio.h>
#include "lcd.h"

volatile uint8_t data;
//				i2c_init
//Initializes I2C bus to run at 100kHz and enables the bus
void i2c_init(void){
  //DDRD |= (1<<PD0) | (1<<PD1);
  //PORTD |= (1<<PD0) | (1<<PD1);
  TWBR = 0x12;
  TWCR |= (1<<TWEN) | (1<<TWIE);
}

//				i2c_send
//Sends a byte over I2C to the slave mega128
/*void i2c_send(uint8_t byte){
  
  //Send start bit
  TWCR |= TWCR_START;
  while(!(TWCR & (1<<TWINT))){}
  if(!(TW_STATUS == TW_START)){
    string2lcd("no start"); 
	return;
  }
  
  //Send the slave write address
  TWDR = SLAVE_WRITE;
  TWCR |= TWCR_SEND;
  while(!(TWCR & (1<<TWINT))){}
  if(TW_STATUS != TW_MT_SLA_ACK){
    string2lcd("bad address"); 
	return;
  }

  //Send the first data byte
  TWDR = byte;
  TWCR |= TWCR_SEND;
  while(!(TWCR & (1<<TWINT))){}
  if(TW_STATUS != TW_MT_DATA_ACK){
    string2lcd("bad data"); 
	return;
  }
  
  //Send the stop bit
  TWCR |= TWCR_STOP;
}*/
void i2c_send(uint8_t byte){
  data = byte;
  TWCR = TWCR_START;
}

ISR(TWI_vect)
{
  uint8_t status = (TWSR & 0xF8);
  if((status == 0x08) || (status == 0x10)){
    TWDR = SLAVE_WRITE;
    TWCR |= TWCR_SEND;
  }
  else if(status == 0x18){
    TWDR = data;
    TWCR |= TWCR_SEND;
  }
  else if(status == 0x20){
    //TWCR |= TWCR_START;
  }
  else if(status == 0x28){
    TWCR |= TWCR_STOP;
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

int main(){
  spi_init();
  DDRF |= (1<<PF3);  //Set up LCD pin
  DDRD |= (1<<PD2) | (1<<PD3);
  PORTD &= ~((1<<PD2) | (1<<PD3));
  lcd_init();
  i2c_init();
  uint8_t i = 0;
  sei();
  while(1){
    char string[20];
	sprintf(string, " Sent: %X", i);
	i2c_send(i);
	clear_display();
	cursor_home();
	string2lcd(string);
	i++;
	_delay_ms(10000);
  }
  return 0;
}