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

#define CAPTURE_DATA_BYTES 2560
#define MAX_LINE 28

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

void display_analyze(uint16_t startByte, uint16_t endByte)
{
  char string[40];
  uint8_t line = 1;
  uint16_t i = startByte;
  uint8_t condition;
  static uint16_t startByteOld = 0;

  if(startByteOld != startByte)
    clear_display();
  startByteOld = startByte;

  while((line <= MAX_LINE) && (i < endByte-2)){
    condition = (capture_data[i] & 0x0F);
  
    if(condition == START){
      write_string(line, 1, "START");
      line++;
    }
    else if(condition == STOP){
      write_string(line, 1, "STOP");
      line += 2;
      i += 2;
      continue;
    }
    else if(condition == RSTART){
      write_string(line, 1, "REPEATED START");
      line++;
    }
    else if(condition == ABNORM){
      write_string(line, 1, "ABNORMAL TRANSITION");
      line += 2;
      i += 2;
      continue;
    }
    else if(condition == ASTOP){
      write_string(line, 1, "ABNORMAL STOP CONDITION");
      line += 2;
      i += 2;
      continue;
    }
    else if(condition == ASTART){
      write_string(line, 1, "ABNORMAL START CONDITION");
      line++;
    }
    else if(condition == DATA){
      char string2[40];
      sprintf(string2, "DATA: %02X + %s", capture_data[i+1],
        (((capture_data[i+2] & 0xF0)>> 4) == ACK)?"ACK":"NACK");
      write_string(line, 1, string2);
      i += 2;
      line++;
      continue;
    }
    sprintf(string, "ADDR: %02X + %s + %s", (capture_data[i+1] >> 1),
      ((capture_data[i+1] & 0x01) == 0x01)?"READ":"WRITE",
      (((capture_data[i+2] & 0xF0 )>> 4) == ACK)?"ACK":"NACK");
    write_string(line, 1, string);
    i += 2;
    line++;
  }
}

int main(void)
{
  uint16_t analyze_start;
  uint8_t held_count;
  uint8_t scroll_up_last;
  uint8_t scroll_down_last;

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

  while(1){
    //Start Menu code
    clear_display();
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
    pressed_buttons &= ~enter;
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
    pressed_buttons |= cancel;
    
    //Begin capture code
    capture_data_end = data_capture(capture_data_start);
    if (capture_data_end > capture_data_start + CAPTURE_DATA_BYTES - 2)
      capture_data_end = capture_data_start + CAPTURE_DATA_BYTES - 2;

    //Re-enable normal button checking
    enable_normal_buttons();
    while(pressed_buttons & cancel){
      pressed_buttons &= ~cancel;
      _delay_ms(1);
    }

    clear_display();

    analyze_start = 0;
    held_count = 0;
    scroll_up_last = 0;
    scroll_down_last = 0;
    while((pressed_buttons & cancel) == 0){
      //Check scrolling buttons
      if(pressed_buttons & scroll_up){
        pressed_buttons &= ~scroll_up;
        clear_button_states();
        analyze_start -= (held_count > MAX_LINE) ? MAX_LINE : 2;
        if(analyze_start >= UINT16_MAX - MAX_LINE)
          analyze_start = 0;
        if(scroll_up_last && held_count <= MAX_LINE){
          held_count++;
        } else if(!scroll_up_last){
          held_count = 1;
          scroll_up_last = 1;
        }
        scroll_down_last = 0;
      }
      else if(pressed_buttons & scroll_down){
        pressed_buttons &= ~scroll_down;
        clear_button_states();
        analyze_start += (held_count > MAX_LINE) ? MAX_LINE : 2;
        if(analyze_start > capture_data_end - capture_data_start - 2)
          analyze_start = capture_data_end - capture_data_start - 2;
        if(scroll_down_last && held_count <= MAX_LINE){
          held_count++;
        } else if(!scroll_down_last){
          held_count = 1;
          scroll_down_last = 1;
        }
        scroll_up_last = 0;
      } else {
        held_count = 0;
        scroll_up_last = 0;
        scroll_down_last = 0;
      }
      if(held_count > MAX_LINE) held_count = MAX_LINE + 1;

      //Analyze and display
      display_analyze(analyze_start, capture_data_end - capture_data_start);

      _delay_ms(held_count > MAX_LINE ? 250 : 50);
    }
    pressed_buttons &= ~cancel;
  }
  return 0;
}
