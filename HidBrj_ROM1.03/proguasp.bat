
@ECHO TARGET=atmega8
@ECHO HFUSE=0xc0
@ECHO LFUSE=0x9f



:flash
%3avrdude -c usbasp -p atmega8 -u -U flash:w:bootload.hex


:fuse
%3avrdude -c usbasp -p atmega8 -u -U hfuse:w:%1:m -U lfuse:w:%2:m

:lock
rem %3avrdude -c usbasp -p atmega8 -u -U lock:w:0x2f:m