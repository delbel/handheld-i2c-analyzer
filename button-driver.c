/*
  button-driver.c
  
  This is the driver for the buttons.  It has the code to setup the interrupt, debounce the buttons,
  and handle the interrupt for the cancel button.
  
*/
#include <stdio.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define BUTTON_PORT_DIR 	PORTB.DIR
#define BUTTON_PORT_OUT 	PORTB.OUT
#define BUTTON_PORT_IN  	PORTB.IN
#define CANCEL_BUTTON 		(1<<2)
#define CANCEL_CONTROL_PIN 		PORTB.PIN2CTRL 

#define VERSION "0.0.1"

volatile uint8_t pressed_buttons;

/*
* Debounces the buttons and returns 1 when a rising edge is caught and debounced, 0 if button is not pressed
* or is being held down.
*/
uint8_t chk_buttons(uint8_t button){
  static uint16_t state[8] = {0};
  state[button] = (state[button] <<1) | (!bit_is_clear(BUTTON_PORT_IN, button)) | 0xE000;
  if(state[button] == 0xF000) return 1;
  return 0;
}

//ISR for timer/counter
ISR(TCC1_OVF_vect)
{
  int i;
  for(i=0;i<4;i++)
  {
    if(chk_buttons(i))
	  {
	    pressed_buttons = (pressed_buttons | (1<<i));
	  }
  }
}

//ISR(PORTB_INT0_vect)
//{
//  //place value to be checked in register
//}

void init_buttons(void)
{
  BUTTON_PORT_DIR = 0x00;
  CANCEL_CONTROL_PIN = 0x02;
  //setup timer/counter interrupt for button debouncing
  TCC1.CTRLA = 0x07; //use clk/1024
  //top set to 32
  TCC1.PERH = 0x00;
  TCC1.PERL = 0x20;
  
  //setup external interrupt for cancel button
  PORTB.INTCTRL = 0x03; //high level interrupt
}

void enable_normal_buttons(void)
{
  //enable overflow vector
  TCC1.INTCTRLA = 0x01; //low interrupt priority
}

void disable_normal_buttons(void)
{
  //disable overflow vector
  TCC1.INTCTRLA = 0x00; //interrupt off
}

//void enable_cancel_interrupt(void)
//{
//  PORTB.INT0MASK = CANCEL_BUTTON; //enable interrupt
//}

//void disable_cancel_interrupt(void)
//{
//  PORTB.INT0MASK = 0x00; //disable interrupt
//}
