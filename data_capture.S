;data_capture.S

;define pins and ports
#include <avr/io.h>

.global data_capture

.equ START, 1
.equ STOP, 2
.equ DATA, 3
.equ RSTART, 4
.equ ABNORMAL, 5
.equ ASTOP, 6
.equ ASTART, 7

.equ ACK, 1
.equ NACK, 2

;r16 - previous value
;r17 - new value
;r18 - i
;r19 - temp

data_capture: 
	;push any registers
	push r16
  push r17
  push r18
  push r19
	push r30
	push r31

  ;set up the pointer to the array
	lds r30, zlow
	lds r31, zhigh

  ;set up virtual port
  lds		r16, PORTCFG_VPCTRLA
  andi	r16, ~PORTCFG_VP0MAP_gm
  sts		PORTCFG_VPCTRLA, r16

idle_start:
  mov r17, r16          ;copy previous value
  in r16,  VPORT0_IN    ;read new value
  cpi r17, 0b01         ;check if previous value is 0b01
  breq idle_start_01
  cpi r17, 0b10         ;check if previous value is 0b10
  breq idle_start_10
  cpi r17, 0b11         ;check if previous value is 0b11
  breq idle_start_11  

idle_start_00:
  cpi r16, 0b00         ;check if new value is 0b01
  breq idle_start_00_01
  cpi r16, 0b10         ;check if new value is 0b10
  breq idle_start_00_10
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start_00_11  

idle_start_00_00:       ;00 -> 00
  rjmp idle_start

idle_start_00_01:       ;00 -> 01
  rjmp idle_start

idle_start_00_10:       ;00 -> 10
  rjmp idle_start

idle_start_00_11:       ;00 -> 11
  rjmp idle_start

idle_start_01:
  cpi r16, 0b01         ;check if new value is 0b01
  breq idle_start_01_01
  cpi r16, 0b10         ;check if new value is 0b10
  breq idle_start_01_10
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start_01_11  

idle_start_01_00:       ;01 -> 00
  rjmp idle_start

idle_start_01_01:       ;01 -> 01
  rjmp idle_start

idle_start_01_10:       ;01 -> 10
  rjmp idle_start

idle_start_01_11:       ;01 -> 11
  rjmp idle_start

idle_start_10:
  cpi r16, 0b01         ;check if new value is 0b01
  breq idle_start_10_01
  cpi r16, 0b10         ;check if new value is 0b10
  breq idle_start_10_10
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start_10_11  

idle_start_10_00:       ;10 -> 00
  rjmp idle_start

idle_start_10_01:       ;10 -> 01
  rjmp idle_start

idle_start_10_10:       ;10 -> 10
  rjmp idle_start

idle_start_10_11:       ;10 -> 11
  rjmp idle_start

idle_start_11:
  cpi r16, 0b01         ;check if new value is 0b01
  breq idle_start_11_01
  cpi r16, 0b10         ;check if new value is 0b10
  breq idle_start_11_10
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start_11_11  

idle_start_11_00:       ;11 -> 00
  rjmp idle_start

idle_start_11_01:       ;11 -> 01 - Normal start
  ld r19, Z
  ori r19, START
  st Z+, r19
  ldi r18, 0
  rjmp done

idle_start_11_10:       ;11 -> 10
  rjmp idle_start

idle_start_11_11:       ;11 -> 11
  rjmp idle_start
  



done:
	;pop any resgisters
	pop r31
	pop r30
  pop r19
  pop r18
  pop r17
	pop r16
	ret

