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

;subtract two to prevent indexing out of bounds
.equ ARRAY_SIZE, 2560-2

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

  ;set up the pointer to the array (Z)
  mov r30, r24
  mov r31, r25

  ;set up port A as virtual port 0
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

  ;point to end of array (X)
  mov r26, r24
  mov r27, r25
  ldi r19, lo8(ARRAY_SIZE)
  add r26, r19
  ldi r19, hi8(ARRAY_SIZE)
  adc r27, r19

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

idle_start:
  mov r17, r16                         ;copy previous value
  in r16,  VPORT0_IN                   ;read new value
  cpi r17, 0b11
  brne idle_start                      ;00 -> XX or 01 -> XX or 10 -> XX

  cpi r16, 0b01
  breq idle_start_11_01
  cpi r16, 0b11
  breq idle_start

  ld r19, Z                            ;11 -> 00 or 11 -> 10
  ori r19, ABNORM
  st Z+, r19
  st Z+, r20
  rjmp abnormal

idle_start_11_01:
  ld r19, Z                            ;11 -> 01
  ori r19, START
  st Z+, r19
  mov r18, r20
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

abnormal:
  in r16,  VPORT0_IN                   ;read new value
  cpi r16, 0b11
  breq idle_start                      ;XX -> 11
  rjmp abnormal

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done_helper_1:
  rjmp done

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

address_rw:
  ld r19, X                            ;check if filling last byte of array
  cpi r19, 0
  brne done_helper_1

  mov r17, r16                         ;copy previous value
  in r16,  VPORT0_IN                   ;read new value
  cpi r17, 0b01
  breq address_rw_01
  cpi r17, 0b10
  breq address_rw_10
  cpi r17, 0b11
  breq address_rw_11

  cpi r16, 0b01
  breq address_rw_00_01
  cpi r16, 0b11
  brne address_rw                      ;00 -> 00 or 00 -> 10

  cpi r18, 7
  breq address_rw_00_11_i7
  
  ld r19, Z                            ;00 -> 11, i=0..6
  lsl r19
  ori r19, 1
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_00_11_i7:
  ld r19, Z                            ;00 -> 11, i=7
  lsl r19
  ori r19, 1
  st Z+, r19
  rjmp ack_nack

address_rw_00_01:
  cpi r18, 7
  breq address_rw_00_01_i7
  
  ld r19, Z                            ;00 -> 01, i=0..6
  lsl r19
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_00_01_i7:
  ld r19, Z                            ;00 -> 01, i=7
  lsl r19
  st Z+, r19
  rjmp ack_nack

address_rw_01:
  cpi r16, 0b11
  brne address_rw                      ;01 -> 00 or 01 -> 01 or 01 -> 10

  ld r19, Z+                           ;01 -> 11
  ld r19, Z
  ori r19, ASTOP
  st Z+, r19
  st Z+, r20
  rjmp idle_start

address_rw_11:
  cpi r16, 0b01
  brne address_rw                      ;11 -> 00 or 11 -> 10 or 11 -> 11

  ld r19, Z+                           ;11 -> 01
  ld r19, Z
  ori r19, ASTART
  st Z+, r19
  st Z+, r20
  mov r18, r20
  rjmp address_rw

address_rw_10:
  cpi r16, 0b01
  breq address_rw_10_01
  cpi r16, 0b11
  brne address_rw                      ;10 -> 00 or 10 -> 10

  cpi r18, 7
  breq address_rw_00_11_i7
  
  ld r19, Z                            ;10 -> 11, i=0..6
  lsl r19
  ori r19, 1
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_10_11_i7:
  ld r19, Z                            ;10 -> 11, i=7
  lsl r19
  ori r19, 1
  st Z+, r19
  rjmp ack_nack

address_rw_10_01:
  cpi r18, 7
  breq address_rw_10_01_i7
  
  ld r19, Z                            ;10 -> 01, i=0..6
  lsl r19
  st Z, r19
  inc r18
  rjmp address_rw

address_rw_10_01_i7:
  ld r19, Z                            ;10 -> 01, i=7
  lsl r19
  st Z+, r19
  rjmp ack_nack

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ack_nack:
  mov r17, r16                         ;copy previous value
  in r16,  VPORT0_IN                   ;read new value
  cpi r17, 0b01
  breq ack_nack_01
  cpi r17, 0b10
  breq ack_nack_10
  cpi r17, 0b11
  breq ack_nack_11  

  cpi r16, 0b01
  breq ack_nack_00_01
  cpi r16, 0b11
  brne ack_nack                        ;00 -> 00 or 00 -> 10

  ldi r19, (NACK<<4) | DATA            ;00 -> 11
  st Z+, r19
  mov r18, r20
  rjmp data_m

ack_nack_00_01:
  ldi r19, (ACK<<4) | DATA             ;00 -> 01
  st Z+, r19
  mov r18, r20
  rjmp data_m

ack_nack_01:
  cpi r16, 0b11
  brne ack_nack                        ;01 -> 00 or 01 -> 01 or 01 -> 10

  ld r19, Z                            ;01 -> 11
  ori r19, ASTOP
  st Z+, r19
  st Z+, r20
  rjmp idle_start

ack_nack_10:
  cpi r16, 0b01
  breq ack_nack_10_01
  cpi r16, 0b11
  brne ack_nack                        ;10 -> 00 or 10 -> 10

  ldi r19, (NACK<<4) | DATA            ;10 -> 11
  st Z+, r19
  mov r18, r20
  rjmp data_m

ack_nack_10_01:
  ldi r19, (ACK<<4) | DATA             ;10 -> 01
  st Z+, r19
  mov r18, r20
  rjmp data_m

ack_nack_11:
  cpi r16, 0b01
  brne ack_nack                        ;11 -> 00 or 11 -> 10 or 11 -> 11

  ld r19, Z                            ;11 -> 01
  ori r19, ASTART
  st Z+, r19
  mov r18, r20
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done_helper_2:
  rjmp done

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

data_m:
  ld r19, X                            ;check if filling last byte of array
  cpi r19, 0
  brne done_helper_2

  mov r17, r16                         ;copy previous value
  in r16,  VPORT0_IN                   ;read new value
  cpi r17, 0b01
  breq data_m_01
  cpi r17, 0b10
  breq data_m_10
  cpi r17, 0b11
  breq data_m_11

  cpi r16, 0b01
  breq data_m_00_01
  cpi r16, 0b11
  brne data_m                          ;00 -> 00 or 00 -> 10

  cpi r18, 7
  breq data_m_00_11_i7

  ld r19, Z                            ;00 -> 11, i=0..6
  lsl r19
  ori r19, 1
  st Z, r19
  inc r18
  rjmp data_m

data_m_00_11_i7:
  ld r19, Z                            ;00 -> 11, i=7
  lsl r19
  ori r19, 1
  st Z+, r19
  rjmp ack_nack

data_m_00_01:
  cpi r18, 7
  breq data_m_00_01_i7

  ld r19, Z                            ;00 -> 01, i=0..6
  lsl r19
  st Z, r19
  inc r18
  rjmp data_m

data_m_00_01_i7:
  ld r19, Z                            ;00 -> 01, i=7
  lsl r19
  st Z+, r19
  rjmp ack_nack

data_m_01:
  cpi r16, 0b11
  brne data_m                          ;01 -> 00 or 01 -> 01 or 01 -> 10

  cpi r18, 1
  breq data_m_01_11_i1

  ld r19, Z+                           ;01 -> 11, i=0, i=2..7
  ld r19, Z
  ori r19, ASTOP
  st Z+, r19
  st Z+, r20
  rjmp idle_start

data_m_11:
  cpi r16, 0b01
  brne data_m                          ;11 -> 00 or 11 -> 10 or 11 -> 11

  cpi r18, 1
  breq data_m_11_01_i1

  ld r19, Z+                           ;11 -> 01, i=0, i=2..7
  ld r19, Z
  ori r19, ASTART
  st Z+, r19
  mov r18, r20
  rjmp idle_start

data_m_10:
  cpi r16, 0b01
  breq data_m_10_01
  cpi r16, 0b11
  brne data_m                          ;10 -> 00 or 10 -> 10

  cpi r18, 7
  breq data_m_10_11_i7

  ld r19, Z                            ;10 -> 11, i=0..6
  lsl r19
  ori r19, 1
  st Z, r19
  inc r18
  rjmp data_m

data_m_10_11_i7:
  ld r19, Z                            ;10 -> 11, i=7
  lsl r19
  ori r19, 1
  st Z+, r19
  rjmp ack_nack

data_m_10_01:
  cpi r18, 7
  breq data_m_10_01_i7

  ld r19, Z                            ;10 -> 01, i=0..6
  lsl r19
  st Z, r19
  inc r18
  rjmp data_m

data_m_10_01_i7:
  ld r19, Z                            ;10 -> 01, i=7
  lsl r19
  st Z+, r19
  rjmp ack_nack

data_m_01_11_i1:
  st Z, r20                            ;01 -> 11, i=1
  ld r19, -Z
  andi r19, 0b11110000
  ori r19, STOP
  st Z+, r19
  st Z+, r20
  rjmp idle_start

data_m_11_01_i1:
  st Z, r20                            ;11 -> 01, i=1
  ld r19, -Z
  andi r19, 0b11110000
  ori r19, RSTART
  st Z+, r19
  mov r18, r20
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done:
  ;disable cancel button
  ldi r26, lo8(PORTB_INT0MASK)
  ldi r27, hi8(PORTB_INT0MASK)
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

;cancel button ISR
;replaces PC on stack so that returning from interrupt jumps to "done"
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

