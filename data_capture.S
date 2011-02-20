;data_capture.S

;define pins and ports
#include <avr/io.h>
.equ capture_port, 0x0600

.global data_capture

data_capture: 
	;push any registers
	push R16
	push R30
	push R31
	lds r30, zlow
	lds r31, zhigh
	LDI R16, 23
	ST Z+, R16
	LDI R16, 56
	ST Z+, R16
	;pop any resgisters
	pop R31
	pop R30
	pop R16
	ret

