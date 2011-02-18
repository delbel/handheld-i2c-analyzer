/*
  main.c
  
  This is the main function of the Handheld I2C Device
*/
#include <stdio.h>
#include <stddef.h>
#include <avr/io.h>
#define F_CPU 32000000UL
#include <util/delay.h>
#include "lcd-driver.c"
#include "i2c-driver.c"
#include "button-driver.c"

#define btn0_bm (1<<0) 
#define btn1_bm (1<<1)
#define btn2_bm (1<<2) //cancel/back button
#define btn3_bm (1<<3)

#define VERSION "0.0.1"

volatile uint8_t logic_level = 0; //0 equals 3.3V, 1 equals 5V

void init_clock()
{
  CCP = CCP_IOREG_gc; //Security Signature to modify clock
  // initialize clock source to be 32MHz internal oscillator (no PLL)
  OSC.CTRL = OSC_RC32MEN_bm; //enable internal 32MHz oscillator
  while(!(OSC.STATUS & OSC_RC32MRDY_bm)); //wait for oscillator ready
  CCP = CCP_IOREG_gc; //Security Signature to modify clock 
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc; //select sysclock 32MHz osc
  CLK.PSCTRL = 0x00; // no division on peripheral clock 
}

void change_logic_level()
{
	logic_level ^= 1;
  if(logic_level){
    invert_string(9, 4, 15, 1); //highlight 5V selection
    invert_string(7, 4, 15, 0); //unselect 3.3V
  }
  else{
    invert_string(7, 4, 15, 1); //highlight 3.3V selection
    invert_string(9, 4, 15, 0); //unselect 5V
  }
}

int main(void)
{
  // Pull up FTDI pins
  PORTF.PIN2CTRL = PORT_OPC_PULLUP_gc;
  PORTF.PIN3CTRL = PORT_OPC_PULLUP_gc;
  PORTF.PIN4CTRL = PORT_OPC_PULLUP_gc;
  PORTF.PIN5CTRL = PORT_OPC_PULLUP_gc;

  init_clock();

  _delay_ms(5);
  
  CONTROL_DIR = 0xFF;
  DATA_DIR = 0xFF;
  
  PMIC.CTRL = 0x05; //low and high interrupts enabled
  sei();
  init_buttons();
  enable_normal_buttons();
  
  reset_LCD();
  init_LCD();
  
  //Start Menu code
  write_string(4, 1, "Select Logic Level and Press Enter to");
  write_string(5, 1, "Start Capturing Data:");
  write_string(7, 5, "3.3 Volt Logic");
  write_string(9, 5, "5   Volt Logic" );
  invert_string(7, 4, 15, 1);  //highlight 3.3V selection

  while(1/*(pressed_buttons & btn3_bm) == 0*/){
    if(pressed_buttons & btn0_bm){
	    pressed_buttons &= ~btn0_bm;
      change_logic_level();
    }
    else if(pressed_buttons & btn1_bm){
	    pressed_buttons &= ~btn1_bm;
      change_logic_level();
    }
  }
  //End of Start Menu
  
  while(1){};
  return 0;
}
