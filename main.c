/*
  main.c
  
  This is the main function of the Handheld I2C Device
*/
#include <stdio.h>
#include <stddef.h>
#include <avr\io.h>
#define F_CPU 32000000UL
#include <util\delay.h>
#include "lcd-driver.c"
#include "i2c-driver.c"
#include "button-driver.c"

#define button0_bm (1<<0)
#define button1_bm (1<<1)
#define button2_bm (1<<2)
#define button3_bm (1<<3)

#define VERSION "0.0.1"

int main(void)
{
  CCP = CCP_IOREG_gc; //Security Signature to modify clock
  // initialize clock source to be 32MHz internal oscillator (no PLL)
  OSC.CTRL = OSC_RC32MEN_bm; //enable internal 32MHz oscillator
  while(!(OSC.STATUS & OSC_RC32MRDY_bm)); //wait for oscillator ready
  CCP = CCP_IOREG_gc; //Security Signature to modify clock 
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc; //select sysclock 32MHz osc

  CLK.PSCTRL = 0x00;
  
  CONTROL_DIR = 0xFF;
  DATA_DIR = 0xFF;
  asm("nop");
  reset_LCD();
  init_LCD();
  
  while(1){};
  return 0;
}