/*
  main.c
  
  This is the main function of the Handheld I2C Device
*/

//Hardware Setup:
//PORTA 0: Input A
//PORTA 1: Input B
//PORTA 2-3: GND for Input
//PORTB 0-3: Button input - Red is 0
//PORTC 0: SDA for Digital Pot
//PORTC 1: SCL for Digital Pot
//PORTC 2-3: Ground for I2C line
//PORTC 6: VDD for Level shifters
//PORTC 7: GRD for Level shifters
//PORTD : Data out for LCD screen
//PORTE 0-4: Control out for LCD screen
#include <stdio.h>
#include <stddef.h>
#include <avr/io.h>
#define F_CPU 32000000UL
#include <util/delay.h>
#include <string.h>
#include "lcd-driver.c"
#include "i2c-driver.c"
#include "button-driver.c"

#define scroll_up (1<<0) //scroll up
#define enter (1<<1) //enter
#define cancel (1<<2) //cancel/back button
#define scroll_down (1<<3) //scroll down

#define VERSION "0.0.1"

#define CAPTURE_DATA_BYTES 1024

extern void data_capture(void);
volatile uint8_t logic_level = 0; //0 equals 3.3V, 1 equals 5V

volatile uint8_t zlow;
volatile uint8_t zhigh;
volatile uint8_t capture_data[CAPTURE_DATA_BYTES];
const uint16_t capture_data_start = (uint16_t)capture_data;

void init_clock(void)
{
  CCP = CCP_IOREG_gc; //Security Signature to modify clock
  // initialize clock source to be 32MHz internal oscillator (no PLL)
  OSC.CTRL = OSC_RC32MEN_bm; //enable internal 32MHz oscillator
  while(!(OSC.STATUS & OSC_RC32MRDY_bm)); //wait for oscillator ready
  CCP = CCP_IOREG_gc; //Security Signature to modify clock 
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc; //select sysclock 32MHz osc
  CLK.PSCTRL = 0x00; // no division on peripheral clock 
}

void change_logic_level(void)
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

void init_capture(void)
{
  uint16_t i;

  // Zero out data array
  for (i=0; i < CAPTURE_DATA_BYTES; i++){
    capture_data[i] = 0;
  }

  // Set Z to point to the start of the array
  
  zlow = (uint8_t)capture_data_start;
  zhigh = (uint8_t)(capture_data_start >> 8);
}

int main(void)
{
  // Prevent the FTDI pins from screwing things up
  PORTF.DIR |= 0b1111<<2;
  PORTF.OUT |= 0b1111<<2;

  init_clock();

  _delay_ms(1);
  
  CONTROL_DIR = 0xFF;
  DATA_DIR = 0xFF;
  
  PMIC.CTRL = 0x05; //low and high interrupts enabled
  
  reset_LCD();
  init_LCD();
  
  init_I2C();
  
  sei();
  init_buttons();
  enable_normal_buttons();

  //Start Menu code
  write_string(4, 1, "Select Logic Level and Press Enter to");
  write_string(5, 1, "Start Capturing Data:");
  write_string(7, 5, "3.3 Volt Logic");
  write_string(9, 5, "5   Volt Logic" );
  invert_string(7, 4, 15, 1);  //highlight 3.3V selection

  while((pressed_buttons & enter) == 0){
    if(pressed_buttons & scroll_up){
	    pressed_buttons &= ~scroll_up;
      change_logic_level();
    }
    else if(pressed_buttons & scroll_down){
	    pressed_buttons &= ~scroll_down;
      change_logic_level();
    }
  }
  //End of Start Menu
  
  //update the I2C pot to correct reference voltage
  if(logic_level){
    set_pot(0x40);
  }else{
    set_pot(0x33);
  }
  
  //write out message to display during capturing
  clear_display();
  write_string(4, 1, "Device is capturing data...");
  write_string(6, 1, "Press cancel to stop capturing");
  write_string(7, 1, "data and analyze captured data.");
  write_string(9, 1, "Device will stop automatically");
  write_string(10, 1, "when 256 single byte transactions");
  write_string(11, 1, "are captured.");
  
  //Initialization for capture code
  init_capture();
  //Begin capture code
  data_capture();
  
  //Analyze and display
  
  //TEST CODE:
  char h1[3];
  char h2[3];
  sprintf(h1, "%i", capture_data[0]);
  sprintf(h2, "%i", capture_data[1]);
  
  write_string(13, 1, h1);
  write_string(14, 1, h2);
  
  PORTA.DIR = 0xFC;
  PORTA.OUT = 0x00;
  while(1){
    char string[3];
	sprintf(string, "%i", PORTA.IN);
    write_string(2, 1, string);
    _delay_ms(250);
  }
  
  //TEST CODE:
  
  while(1){};
  return 0;
}
