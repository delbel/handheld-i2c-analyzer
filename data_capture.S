;data_capture.S

;define pins and ports
#include <avr/io.h>

.global data_capture

.equ START, 1
.equ STOP, 2
.equ DATA, 3
.equ RSTART, 4
.equ ABNORM, 5
.equ ASTOP, 6
.equ ASTART, 7

.equ ACK, 1
.equ NACK, 2

;r16 - new value
;r17 - previous value
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

idle_start:
  in r16,  VPORT0_IN    ;read new value

idle_start_11:
  cpi r16, 0b01         ;check if new value is 0b01
  breq idle_start_11_01
  cpi r16, 0b10         ;check if new value is 0b10
  breq idle_start_11_10
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start_11_11  

idle_start_11_00:       ;11 -> 00 - Abnormal transition
  ld r19, Z
  andi r19, 0b11110000
  ori r19, ABNORM
  st Z+, r19
  ldi r19, 0
  st Z+, r19
  rjmp abnormal

idle_start_11_01:       ;11 -> 01 - Normal start
  ld r19, Z
  andi r19, 0b11110000
  ori r19, START
  st Z+, r19
  ldi r18, 0
  rjmp address_rw

idle_start_11_10:       ;11 -> 10 - Abnormal transition
  ld r19, Z
  andi r19, 0b11110000
  ori r19, ABNORM
  st Z+, r19
  ldi r19, 0
  st Z+, r19
  rjmp abnormal

idle_start_11_11:       ;11 -> 11
  rjmp idle_start

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

abnormal:
  in r16,  VPORT0_IN    ;read new value
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start
  rjmp abnormal

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

address_rw:
  nop

done:
	;pop any resgisters
	pop r31
	pop r30
  pop r19
  pop r18
  pop r17
	pop r16
	ret

