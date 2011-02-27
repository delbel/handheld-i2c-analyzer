//atmega-main.c

//Test code to send I2C conditions
//PORTD is the 8 buttons board
//PORTB is the 8 LEDs on the board (also used for SPI)

#define F_CPU 16000000
#define TRUE 1
#define FALSE 0
#define NUM_BUTTONS 8

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <util/twi.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "lcd.h"

//				chk_buttons
//Checks the state of the button number passed to it. It shifts in ones till
//the button is pushed.  Function returns a 1 only once per debounced button
//push so a debounce and toggle function can be implemented at the same time.
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"
//Expects active low pushbuttons on PINA port.  Debounce time is determined by
//timer/counter interrupt delay times 12.
//
uint8_t chk_buttons(uint8_t button){
  static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds present state
  state[button] = (state[button] <<1) | (! bit_is_clear(PIND, button)) |
  0xE000;
  if(state[button] == 0xF000) return 1;
  return 0;
}

//				tcnt0_init
//Initalizes timer/counter0 (TCNT0). TCNT0 is running in async mode with
//external 32kHz crystal.  Runs in normal mode with no prescaling.
//Interrupt occurs at overflow 0xFF
void tcnt0_init(void){
  ASSR |= (1<<AS0); //ext osc TOSC
  TIMSK |= (1<<TOIE0); //enable timer/counter0 overflow interrupt
  TCCR0 |= (1<<CS00); //normal mode, no prescale
}

//				timer/counter0 ISR
ISR(TIMER0_OVR_Vect){

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

//		clock_high, clock_low, data_high, data_low
void clock_high(void){
  PORTA |= 0x01;
  PORTB |= 0x80;
}

void clock_low(void){
  PORTA &= 0xFE;
  PORTB &= 0x7F;
}

void data_high(void){
  PORTA |= 0x02;
  PORTB |= 0x40;
}

void data_low(void){
  PORTA &= 0xFD;
  PORTB &= 0xBF;
}

//	send_start, send_stop
void send_start(void){
  data_high();
  _delay_ms(1);
  clock_high();
  _delay_ms(1);
  data_low();
  _delay_ms(50);
  clock_low();
}

void send_stop(void){
  data_low();
  _delay_ms(50);
  clock_high();
  _delay_ms(50);
  data_high();
}

void send_bit(uint8_t bit){
  if(bit == 0){
    data_low();
  }
  else{
    data_high();
  }
  _delay_ms(1);
  clock_high();
  _delay_ms(50);
  clock_low();
  _delay_ms(50);
}

void send_byte(uint8_t byte){
  int i;
  for(i = 7; i >= 0; i--){
    send_bit((byte & (1<<i))>>i);
  }
}

//				send_condition
//Sends conditions out on the I2C bus to the other mega128 board.
//Output conditions on PORTA Pin 0 SCL, Pin 1 SDA
//The following table dictates which condition is sent
//Number		Condition
//0				Normal transaction
//1				Normal transaction with repeated start
//2				Send transaction and then another without stop condition
//3				Send data without a start
//4				Send start in the middle of data byte and send normal transaction
//5				Send stop in middle of data byte transfer
//6				Send data and stop without ACK or NACK
void send_condition(uint8_t condition){
  if(condition == 7){
    uint16_t i;
	for(i=0; i<=255; i++){
	  cursor_home();
	  char string[20];
	  sprintf(string, "Sending byte:%i   ", i);
	  string2lcd(string);
	  home_line2();
	  string2lcd("                                ");
	  send_start();
	  //8 bits = 7 address + R/W
	  send_byte(0b11000011);
	  send_bit(0);
	  //8 bits = data
	  send_byte((i & 0xFF));
	  send_bit(0);
	  send_stop();
	}
  }
  else if(condition == 0){ //Normal Transaction
    cursor_home();
    string2lcd("Normal Tran                     ");
	home_line2();
	string2lcd("                                ");
    send_start();
	//8 bits = 7 address + R/W
	send_byte(0b01010101);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b11001100);
	send_bit(0);
	send_stop();
  }
  else if(condition == 1){ //Normal Transaction with repeated start
    cursor_home();
    string2lcd("Normal Tran w/                  ");
	home_line2();
	string2lcd("Repeated Start                  ");
    send_start();
	//8 bits = 7 address + R/W
	send_byte(0b01010101);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b11001100);
	send_bit(0); //ACK
	send_start();
	//8 bits = 7 address + R/W
	send_byte(0b10101010);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b00110011);
	send_bit(0); //ACK
	send_stop();
  }
  else if(condition == 2){ //Send transaction and another transaction with no stop or repeated start bit
    cursor_home();
	string2lcd("2 tran w/ no                    ");
	home_line2();
	string2lcd("stop or r-start                 ");
    send_start();
	//8 bits = 7 address + R/W
	send_byte(0b01010101);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b11001100);
	send_bit(0); //ACK
	//8 bits = 7 address + R/W
	send_byte(0b10101010);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b00110011);
	send_bit(0); //ACK
  }
  else if(condition == 3){ //Send data without a start
    cursor_home();
	string2lcd("No start                        ");
	home_line2();
	string2lcd("                                ");
    //8 bits = 7 address + R/W
	send_byte(0b01010101);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b11001100);
	send_bit(0); //ACK
	send_stop();
  }
  else if(condition == 4){ //Send start in middle of transaction and then finish transaction
    cursor_home();
	string2lcd("Abnormal Start                  ");
	home_line2();
	string2lcd("middle of tran                  ");
    send_start();
	//8 bits = 7 address + R/W
    send_byte(0b10101010);
	send_bit(0); //ACK
	//4 bits = data, then interrupted
	send_bit(1);
	send_bit(1);
	send_bit(1);
	send_bit(1);
	send_start();
	//8 bits = 7 address + R/W
	send_byte(0b01010101);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b11110000);
	send_bit(0); //ACK
	send_stop();
  }
  else if(condition == 5){ //Stop condition in middle of data byte transfer
    cursor_home();
	string2lcd("Stop in middle                  ");
	home_line2();
	string2lcd("of tran                         ");
    send_start();
	//8 bits = 7 address + R/W
	send_byte(0b10101010);
	send_bit(0); //ACK
	//4 bits = data, then interrupted
	send_bit(1);
	send_bit(1);
	send_bit(0);
	send_bit(0);
	send_stop();
  }
  else if(condition == 6){ //Send data and stop without ack or nack
    cursor_home();
	string2lcd("No ACK or NACK                 ");
	home_line2();
	string2lcd("                               ");
    send_start();
	//8 bits = 7 address + R/W
	send_byte(0b10101010);
	send_bit(0); //ACK
	//8 bits = data
	send_byte(0b00110011);
	send_stop();
  }
}

//				scan_buttons
//Scans the buttons to see if they have changed state.
void scan_buttons(void){
  uint8_t i;
  for(i = 0; i<NUM_BUTTONS; i++){
    uint8_t value = chk_buttons(i);
	if(value == 1){
	  send_condition(i);
	}
  }
}

int main()
{
  spi_init();
  DDRF |= (1<<PF3);  //Set up LCD pin
  lcd_init();
  cursor_home();
  DDRD = 0x00; //input for buttons
  DDRB |= 0xF0; //high LEDs output
  DDRA = 0x03; //output for SDA and SCL
  sei();
  
  while(1){
    _delay_ms(1);
	scan_buttons();
  }
}