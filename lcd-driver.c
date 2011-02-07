#include <avr/io.h>
#include <string.h>

#define A0 		(1<<0)
#define WR		(1<<1)
#define RD		(1<<2)
#define CS 		(1<<3)
#define RESET 	(1<<4)

#define CONTROL_PORT PORTE.OUT
#define CONTROL_DIR PORTE.DIR
#define CONTROL_PIN PORTE.IN

#define DATA_PORT PORTD.OUT
#define DATA_DIR PORTD.DIR
#define DATA_PIN PORTD.IN

void delay(unsigned int n)
{
  unsigned int i,j;
  for (i=0; i<n; i++)
	for (j=0; j<350; j++)
	{;}
}

void data_out(unsigned char i)
{
  CONTROL_PORT &= ~(A0 | CS);
  CONTROL_PORT &= ~(WR);
  asm("nop");
  DATA_PORT = i;
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  CONTROL_PORT |= WR;
  CONTROL_PORT |= CS;
}

void comm_out(unsigned char j)
{
  CONTROL_PORT |= A0;
  CONTROL_PORT &= ~CS;
  CONTROL_PORT &= ~(WR);
  asm("nop");
  DATA_PORT = j;
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  CONTROL_PORT |= WR;
  CONTROL_PORT |= CS;
}

void clear_display()
{
  int n;
  comm_out(0x46);
  data_out(0x00);
  data_out(0x00);
  comm_out(0x42);
  for(n=0;n<1200;n++){
	data_out(0x20);
  }
  comm_out(0x46);
  data_out(0xB0);
  data_out(0x04);
  comm_out(0x42);
  for(n=0;n<9600;n++){
	data_out(0x00);
  }
}

void reset_LCD()
{
	CONTROL_PORT &= ~RESET;
	delay(5);
	CONTROL_PORT |= RESET;
	delay(10);
}

void init_LCD() 
{
  int i;
  CONTROL_PORT |= RD;
  delay(5);
  comm_out(0x40); //Initialize device and display
  delay(5);
  data_out(0x30); 
  data_out(0x87); 
  data_out(0x07); 
  data_out(0x27); 
  data_out(0x2F); 
  data_out(0xEF); 
  data_out(0x28); 
  data_out(0x00); 
  comm_out(0x44); //SCROLL
  data_out(0x00);
  data_out(0x00);
  data_out(0xF0);
  data_out(0x80);
  data_out(0x25);
  data_out(0xF0);
  data_out(0x00);
  data_out(0x4B);
  data_out(0x00);
  data_out(0x00);
  comm_out(0x5A); //HDOT SCR
  data_out(0x00);
  comm_out(0x5B); //OVLAY
  data_out(0x01); 
  comm_out(0x58); //Display off
  data_out(0x54); //Cursor is off
  comm_out(0x46); //Set cursor to first screen block
  data_out(0x00);
  data_out(0x00);
  for(i = 0; i<9600; i++){
    comm_out(0x42); //Write Command
    data_out(0xA0);
  }
  for(i = 0; i<9600 ; i++){
    comm_out(0x42); //Write Command
    data_out(0x00);
  }
  comm_out(0x46); //Set cursor to first screen block
  data_out(0x00);
  data_out(0x00);
  comm_out(0x5D); //Sets cursor form
  data_out(0x04);
  data_out(0x86);
  comm_out(0x59); //Display on
  comm_out(0x4C); //Cursor Direction right
  delay(5);
}

void write_string(uint8_t line_number, uint8_t position, char* string){
  uint16_t address = line_number * 40 + position;
  comm_out(0x46);
  data_out((uint8_t)(address & 0xFF));
  data_out((uint8_t)(address >> 8));
  comm_out(0x42);
  int i;
  for(i = 0; i < strlen(string); i++){
    data_out(string[i]);
  }
}

void invert_string(uint8_t line_number, uint8_t position, uint8_t length, uint8_t invert_flag){
  uint8_t data;
  if(invert_flag){
    data = 0xFF;
  }else{
    data = 0x00;
  }
  uint16_t address = 9600 + (line_number * 320) + position - 40;
  int j;
  for(j = 0; j < length; j++){
    comm_out(0x46);
    data_out((uint8_t)(address & 0xFF));
    data_out((uint8_t)(address >> 8));
    comm_out(0x4F); //set cusor direction to down
    comm_out(0x42);
    int i;
    for(i = 0; i < 9; i++){
      data_out(data);
    }
	address++ ;
  }
  comm_out(0x4C); //reset cursor to right
}