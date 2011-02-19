;data_capture.S

;define pins and ports
#include <avr/io.h>
.equ capture_port, 0x0600

.global data_capture

data_capture: 
	;push any registers
	
	;pop any resgisters
	ret

