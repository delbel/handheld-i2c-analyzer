;;//data_capture.S

.include "ATxmega128A3def.inc"

;;//define pins and ports
.equ capture_port = 0x0600

data_capture: 
sbr r16, 0x01
sts PORTA_base + PORT_OUTSET_offset, r16

