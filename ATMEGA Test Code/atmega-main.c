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

//				scan_buttons
//Scans the buttons to see if they have changed state.
void scan_buttons(void){
  uint8_t i;
  for(i = 0; i<NUM_BUTTONS; i++){
    uint8_t value = chk_buttons(i);
	if((i == 0) & value){
	  PORTB |= 0xF0;
	}
	else if((i == 1) & value){
	  PORTB &= 0x0F;
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
  sei();
  
  while(1){
    _delay_ms(1);
	scan_buttons();
  }
}