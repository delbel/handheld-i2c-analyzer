;data_capture.S

;define pins and ports
#include <avr/io.h>

#define new r16
#define prev r17
#define i r18
#define j r19
#define temp r20
#define zero r21
#define Xl r26
#define Xh r27
#define Zl r30
#define Zh r31

;cancel button is port B, pin 2
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

.global data_capture
data_capture:
  ;push any registers
  push new
  push prev
  push i
  push j
  push temp
  push zero
  push Xl
  push Xh
  push Zl
  push Zh

  ;set up the pointer to the array (Z)
  mov Zl, r24
  mov Zh, r25

  ;set up port A as virtual port 0
  lds temp, PORTCFG_VPCTRLA
  andi temp, ~PORTCFG_VP0MAP_gm
  sts PORTCFG_VPCTRLA, temp

  ;set up 0 register and initialize j
  ldi zero, 0
  mov j, zero

  ;enable cancel interrupt
  ldi temp, CANCEL_BUTTON
  ldi Xl, lo8(PORTB_INT0MASK)
  ldi Xh, hi8(PORTB_INT0MASK)
  st X, temp

  ;point to end of array (X)
  mov Xl, r24
  mov Xh, r25
  ldi temp, lo8(ARRAY_SIZE)
  add Xl, temp
  ldi temp, hi8(ARRAY_SIZE)
  adc Xh, temp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

idle_start:
  ld temp, X                           ;check if filling last byte of array
  cpi temp, 0
  brne done_helper_1

  mov prev, new                        ;copy previous value
  in new, VPORT0_IN                    ;read new value
  cpi prev, 0b11
  brne idle_start                      ;00 -> XX or 01 -> XX or 10 -> XX

  cpi new, 0b01
  breq idle_start_11_01
  cpi new, 0b11
  breq idle_start

  cpi j, 0                             ;11 -> 00 or 11 -> 10
  brne idle_start                      ;don't write abnormal marker twice
  ld temp, Z
  ori temp, ABNORM
  st Z+, temp
  st Z+, zero
  ldi j, 1
  rjmp idle_start

idle_start_11_01:
  ld temp, Z                           ;11 -> 01
  ori temp, START
  st Z+, temp
  mov i, zero
  mov j, zero
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done_helper_1:
  rjmp done

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

address_rw:
  ld temp, X                           ;check if filling last byte of array
  cpi temp, 0
  brne done_helper_1

  mov prev, new                        ;copy previous value
  in new, VPORT0_IN                    ;read new value
  cpi prev, 0b01
  breq address_rw_01
  cpi prev, 0b10
  breq address_rw_10
  cpi prev, 0b11
  breq address_rw_11

  cpi new, 0b01
  breq address_rw_00_01
  cpi new, 0b11
  brne address_rw                      ;00 -> 00 or 00 -> 10

  cpi i, 7
  breq address_rw_00_11_i7

  ld temp, Z                           ;00 -> 11, i=0..6
  lsl temp
  ori temp, 1
  st Z, temp
  inc i
  rjmp address_rw

address_rw_00_11_i7:
  ld temp, Z                           ;00 -> 11, i=7
  lsl temp
  ori temp, 1
  st Z+, temp
  rjmp ack_nack

address_rw_00_01:
  cpi i, 7
  breq address_rw_00_01_i7

  ld temp, Z                           ;00 -> 01, i=0..6
  lsl temp
  st Z, temp
  inc i
  rjmp address_rw

address_rw_00_01_i7:
  ld temp, Z                           ;00 -> 01, i=7
  lsl temp
  st Z+, temp
  rjmp ack_nack

address_rw_01:
  cpi new, 0b11
  brne address_rw                      ;01 -> 00 or 01 -> 01 or 01 -> 10

  ld temp, Z+                          ;01 -> 11
  ld temp, Z
  ori temp, ASTOP
  st Z+, temp
  st Z+, zero
  rjmp idle_start

address_rw_11:
  cpi new, 0b01
  brne address_rw                      ;11 -> 00 or 11 -> 10 or 11 -> 11

  ld temp, Z+                          ;11 -> 01
  ld temp, Z
  ori temp, ASTART
  st Z+, temp
  st Z+, zero
  mov i, zero
  rjmp address_rw

address_rw_10:
  cpi new, 0b01
  breq address_rw_10_01
  cpi new, 0b11
  brne address_rw                      ;10 -> 00 or 10 -> 10

  cpi i, 7
  breq address_rw_00_11_i7

  ld temp, Z                           ;10 -> 11, i=0..6
  lsl temp
  ori temp, 1
  st Z, temp
  inc i
  rjmp address_rw

address_rw_10_11_i7:
  ld temp, Z                           ;10 -> 11, i=7
  lsl temp
  ori temp, 1
  st Z+, temp
  rjmp ack_nack

address_rw_10_01:
  cpi i, 7
  breq address_rw_10_01_i7

  ld temp, Z                           ;10 -> 01, i=0..6
  lsl temp
  st Z, temp
  inc i
  rjmp address_rw

address_rw_10_01_i7:
  ld temp, Z                           ;10 -> 01, i=7
  lsl temp
  st Z+, temp
  rjmp ack_nack

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ack_nack:
  mov prev, new                        ;copy previous value
  in new, VPORT0_IN                    ;read new value
  cpi prev, 0b01
  breq ack_nack_01
  cpi prev, 0b10
  breq ack_nack_10
  cpi prev, 0b11
  breq ack_nack_11

  cpi new, 0b01
  breq ack_nack_00_01
  cpi new, 0b11
  brne ack_nack                        ;00 -> 00 or 00 -> 10

  ldi temp, (NACK<<4) | DATA           ;00 -> 11
  st Z+, temp
  mov i, zero
  rjmp data_m

ack_nack_00_01:
  ldi temp, (ACK<<4) | DATA            ;00 -> 01
  st Z+, temp
  mov i, zero
  rjmp data_m

ack_nack_01:
  cpi new, 0b11
  brne ack_nack                        ;01 -> 00 or 01 -> 01 or 01 -> 10

  ld temp, Z                           ;01 -> 11
  ori temp, ASTOP
  st Z+, temp
  st Z+, zero
  rjmp idle_start

ack_nack_10:
  cpi new, 0b01
  breq ack_nack_10_01
  cpi new, 0b11
  brne ack_nack                        ;10 -> 00 or 10 -> 10

  ldi temp, (NACK<<4) | DATA           ;10 -> 11
  st Z+, temp
  mov i, zero
  rjmp data_m

ack_nack_10_01:
  ldi temp, (ACK<<4) | DATA            ;10 -> 01
  st Z+, temp
  mov i, zero
  rjmp data_m

ack_nack_11:
  cpi new, 0b01
  brne ack_nack                        ;11 -> 00 or 11 -> 10 or 11 -> 11

  ld temp, Z                           ;11 -> 01
  ori temp, ASTART
  st Z+, temp
  mov i, zero
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done_helper_2:
  rjmp done

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

data_m:
  ld temp, X                           ;check if filling last byte of array
  cpi temp, 0
  brne done_helper_2

  mov prev, new                        ;copy previous value
  in new, VPORT0_IN                    ;read new value
  cpi prev, 0b01
  breq data_m_01
  cpi prev, 0b10
  breq data_m_10
  cpi prev, 0b11
  breq data_m_11

  cpi new, 0b01
  breq data_m_00_01
  cpi new, 0b11
  brne data_m                          ;00 -> 00 or 00 -> 10

  cpi i, 7
  breq data_m_00_11_i7

  ld temp, Z                           ;00 -> 11, i=0..6
  lsl temp
  ori temp, 1
  st Z, temp
  inc i
  rjmp data_m

data_m_00_11_i7:
  ld temp, Z                           ;00 -> 11, i=7
  lsl temp
  ori temp, 1
  st Z+, temp
  rjmp ack_nack

data_m_00_01:
  cpi i, 7
  breq data_m_00_01_i7

  ld temp, Z                           ;00 -> 01, i=0..6
  lsl temp
  st Z, temp
  inc i
  rjmp data_m

data_m_00_01_i7:
  ld temp, Z                           ;00 -> 01, i=7
  lsl temp
  st Z+, temp
  rjmp ack_nack

data_m_01:
  cpi new, 0b11
  brne data_m                          ;01 -> 00 or 01 -> 01 or 01 -> 10

  cpi i, 1
  breq data_m_01_11_i1

  ld temp, Z+                          ;01 -> 11, i=0, i=2..7
  ld temp, Z
  ori temp, ASTOP
  st Z+, temp
  st Z+, zero
  rjmp idle_start

data_m_11:
  cpi new, 0b01
  brne data_m                          ;11 -> 00 or 11 -> 10 or 11 -> 11

  cpi i, 1
  breq data_m_11_01_i1

  ld temp, Z+                          ;11 -> 01, i=0, i=2..7
  ld temp, Z
  ori temp, ASTART
  st Z+, temp
  mov i, zero
  rjmp idle_start

data_m_10:
  cpi new, 0b01
  breq data_m_10_01
  cpi new, 0b11
  brne data_m                          ;10 -> 00 or 10 -> 10

  cpi i, 7
  breq data_m_10_11_i7

  ld temp, Z                           ;10 -> 11, i=0..6
  lsl temp
  ori temp, 1
  st Z, temp
  inc i
  rjmp data_m

data_m_10_11_i7:
  ld temp, Z                           ;10 -> 11, i=7
  lsl temp
  ori temp, 1
  st Z+, temp
  rjmp ack_nack

data_m_10_01:
  cpi i, 7
  breq data_m_10_01_i7

  ld temp, Z                           ;10 -> 01, i=0..6
  lsl temp
  st Z, temp
  inc i
  rjmp data_m

data_m_10_01_i7:
  ld temp, Z                           ;10 -> 01, i=7
  lsl temp
  st Z+, temp
  rjmp ack_nack

data_m_01_11_i1:
  st Z, zero                           ;01 -> 11, i=1
  ld temp, -Z
  andi temp, 0b11110000
  ori temp, STOP
  st Z+, temp
  st Z+, zero
  rjmp idle_start

data_m_11_01_i1:
  st Z, zero                           ;11 -> 01, i=1
  ld temp, -Z
  andi temp, 0b11110000
  ori temp, RSTART
  st Z+, temp
  mov i, zero
  rjmp address_rw

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

done:
  ;disable cancel button
  ldi Xl, lo8(PORTB_INT0MASK)
  ldi Xh, hi8(PORTB_INT0MASK)
  st X, zero

  ;return end of data array
  mov r24, Zl
  mov r25, Zh

  ;pop any resgisters
  pop Zh
  pop Zl
  pop Xh
  pop Xl
  pop zero
  pop temp
  pop j
  pop i
  pop prev
  pop new
  ret

;cancel button ISR
;replaces PC on stack so that returning from interrupt jumps to "done"
.global PORTB_INT0_vect
PORTB_INT0_vect:
  pop temp
  pop temp
  pop temp
  ldi temp, lo8(pm(done))
  push temp
  ldi temp, hi8(pm(done))
  push temp
  push zero
  reti

