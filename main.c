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

#define START 1
#define STOP 2
#define DATA 3
#define RSTART 4
#define ABNORM 5
#define ASTOP 6
#define ASTART 7

#define ACK 1
#define NACK 2

#define VERSION "0.0.1"

#define CAPTURE_DATA_BYTES 1024
#define MAX_LINE 24

volatile uint8_t logic_level = 0; //0 equals 3.3V, 1 equals 5V

extern uint16_t data_capture(uint16_t);
volatile uint8_t capture_data[CAPTURE_DATA_BYTES];
const uint16_t capture_data_start = (uint16_t)capture_data;
volatile uint16_t capture_data_end;

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
}

void display_analyze(int startByte/*, int endByte*/)
{
  clear_display();
  uint8_t line = 1;
  int i = startByte;
  uint8_t condition;
  while(line <= MAX_LINE /*& if i > endByte*/){
    if(capture_data[i] == 0x00)
	{
	  line++;
	  i++;
	  continue;
	}
	condition = (capture_data[i] & 0x0F);\
	
	if(condition == START){
	  write_string(line, 1, "START");
	}
	else if(condition == STOP){
	  write_string(line, 1, "STOP");
	  line++;
	  i++;
	  continue;
	}
	else if(condition == RSTART){
	  line++;
	  write_string(line, 1, "REPEATED START");
	}
	else if(condition == ABNORM){
	  write_string(line, 1, "ABNORMAL TRANSITION");
	  line++;
	  i++;
	  continue;
	}
	else if(condition == ASTOP){
	  write_string(line, 1, "ABNORMAL STOP CONDITION");
	  line++;
	  i++;
	  continue;
	}
	else if(condition == ASTART){
	  write_string(line, 1, "ABNORMAL START CONDITION");
	}
	else if(condition == DATA){
	  i++;
	  char string2[40];
	  sprintf(string2, "DATA: %X + %s", capture_data[i], (((capture_data[i+1] & 0xF0)>> 4) == ACK)?"ACK":"NACK");
	  write_string(line, 1, string2);
	  i++;
	  line++;
	  continue;
	}
	line++;
	i++;
	char string[40];
	sprintf(string, "ADDR: %X + %s + %s", (capture_data[i] >> 1), ((capture_data[i] & 0x01) == 0x01)?"READ":"WRITE", (((capture_data[i+1] & 0xF0 )>> 4) == ACK)?"ACK":"NACK");
	write_string(line, 1, string);
	i++;
	line++;
  }
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

  //Disable normal button checking
  disable_normal_buttons();
  
  //Begin capture code
  capture_data_end = data_capture(capture_data_start);

  //Re-enable normal button checking
  enable_normal_buttons();
  
  //TEST CODE:
  capture_data[0] = 0x01;
  capture_data[1] = 0xF1;
  capture_data[2] = 0x13;
  capture_data[3] = 0xFF;
  capture_data[4] = 0x22;
  //TEST CODE:
  
  //Analyze and display
  display_analyze(0/*,end value of data*/);
  
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
