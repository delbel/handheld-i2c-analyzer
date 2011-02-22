;data_capture.S

;define pins and ports
#include <avr/io.h>

.global data_capture

.equ CANCEL_BUTTON, 1<<2

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
;r20 - 0

data_capture: 
  ;push any registers
  push r16
  push r17
  push r18
  push r19
  push r20
  push r26
  push r27
  push r30
  push r31

  ;set up the pointer to the array
  mov r30, r24
  mov r31, r25

  ;set up virtual port
  lds    r16, PORTCFG_VPCTRLA
  andi  r16, ~PORTCFG_VP0MAP_gm
  sts    PORTCFG_VPCTRLA, r16

  ;set up 0 register
  ldi r20, 0

  ;enable cancel interrupt
  ldi r19, CANCEL_BUTTON
  ldi r26, lo8(PORTB_INT0MASK)
  ldi r27, hi8(PORTB_INT0MASK)
  st X, r19

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

idle_start:
  mov r17, r16          ;copy previous value
  in r16,  VPORT0_IN    ;read new value
  cpi r17, 0b11         ;check if previous value is 0b11 (should be)
  brne idle_start

idle_start_11:
  cpi r16, 0b01         ;check if new value is 0b01
  breq idle_start_11_01
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start

idle_start_11_00:       ;11 -> 00 - Abnormal transition
idle_start_11_10:       ;11 -> 10 - Abnormal transition
  ld r19, Z
  andi r19, 0b11110000
  ori r19, ABNORM
  st Z+, r19
  st Z+, r20
  rjmp abnormal

idle_start_11_01:       ;11 -> 01 - Normal start
  ld r19, Z
  andi r19, 0b11110000
  ori r19, START
  st Z+, r19
  ldi r18, 0
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

abnormal:
  in r16,  VPORT0_IN    ;read new value
  cpi r16, 0b11         ;check if new value is 0b11
  breq idle_start
  rjmp abnormal

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

address_rw:
  mov r17, r16          ;copy previous value
  in r16,  VPORT0_IN    ;read new value
  cpi r17, 0b00         ;check if previous value is 0b00
  breq address_rw_00
  cpi r17, 0b01         ;check if previous value is 0b01
  breq address_rw_01
  cpi r17, 0b10         ;check if previous value is 0b10
  breq address_rw_10
  rjmp address_rw_11

address_rw_00:
  cpi r16, 0b01         ;check if new value is 0b01
  breq address_rw_00_01
  cpi r16, 0b11         ;check if new value is 0b11
  breq address_rw_00_11  
  rjmp address_rw

address_rw_00_01:       ;00 -> 01
  cpi r18, 7            ;check if i = 7
  breq address_rw_00_01_i7
  
address_rw_00_01_i0to6: ;00 -> 01, i=0..6
  ld r19, Z
  lsl r19
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_00_01_i7:    ;00 -> 01, i=7
  ld r19, Z
  lsl r19
  st Z+, r19
  rjmp ack_nack

address_rw_00_11:       ;00 -> 11
  cpi r18, 7            ;check if i = 7
  breq address_rw_00_11_i7
  
address_rw_00_11_i0to6: ;00 -> 11, i=0..6
  ld r19, Z
  lsl r19
  ori r19, 1
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_00_11_i7:    ;00 -> 11, i=7
  ld r19, Z
  lsl r19
  ori r19, 1
  st Z+, r19
  rjmp ack_nack

address_rw_01:
  cpi r16, 0b11         ;check if new value is 0b11
  brne address_rw

address_rw_01_11:       ;01 -> 11
  ld r19, Z+
  ld r19, Z
  andi r19, 0b11110000
  ori r19, ASTOP
  st Z+, r19
  st Z+, r20
  rjmp idle_start

address_rw_10:
  cpi r16, 0b01         ;check if new value is 0b01
  breq address_rw_10_01
  cpi r16, 0b11         ;check if new value is 0b11
  breq address_rw_10_11
  rjmp address_rw

address_rw_10_01:       ;10 -> 01
  cpi r18, 7            ;check if i = 7
  breq address_rw_10_01_i7
  
address_rw_10_01_i0to6: ;10 -> 01, i=0..6
  ld r19, Z
  lsl r19
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_10_01_i7:    ;10 -> 01, i=7
  ld r19, Z
  lsl r19
  st Z+, r19
  rjmp ack_nack

address_rw_10_11:       ;10 -> 11
  cpi r18, 7            ;check if i = 7
  breq address_rw_00_11_i7
  
address_rw_10_11_i0to6: ;10 -> 11, i=0..6
  ld r19, Z
  lsl r19
  ori r19, 1
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_10_11_i7:    ;10 -> 11, i=7
  ld r19, Z
  lsl r19
  ori r19, 1
  st Z+, r19
  rjmp ack_nack

address_rw_11:
  cpi r16, 0b01         ;check if new value is 0b01
  rjmp address_rw

address_rw_11_01:       ;11 -> 01
  ld r19, Z+
  ld r19, Z
  andi r19, 0b11110000
  ori r19, ASTART
  st Z+, r19
  st Z+, r20
  ldi r18, 0
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ack_nack:
  nop ;TODO

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done:
  ;disable cancel button
  st X, r20

  ;return end of data array
  mov r24, r30
  mov r25, r31

  ;pop any resgisters
  pop r31
  pop r30
  pop r27
  pop r26
  pop r20
  pop r19
  pop r18
  pop r17
  pop r16
  ret

;Cancel button ISR
.global PORTB_INT0_vect
PORTB_INT0_vect:
  pop r19
  pop r19
  pop r19
  ldi r19, lo8(pm(done))
  push r19
  ldi r19, hi8(pm(done))
  push r19
  push r20
  reti

