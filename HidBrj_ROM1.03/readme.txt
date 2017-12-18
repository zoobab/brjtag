prepare two usbasp dongles,
one is as for target hid-brjtag which should be programmed in hid-brjtag firmware
another usbasp is working as isp, programing hid-brjtag rom on target.

program bootloader
1.connect two usbasp with 10pin pass through cable
2.short "self programing" JP on target 
2.connect isp to pc , power leds on both usbasp should be on
3.open a dos command window, enter this folder, and run "runme.bat"
4.once bootloader program successfully, two leds on target should start blinking.

program hid-brjtag rom
5.disconnect isp from pc. disconnect target and isp
6.take off  "self programing" jumper on target
6.in a dos command window,type "bootloadhid brj_m812.hex" .But do NOT press <enter> key.
7.conncet target to pc, wait 4-6 s , press <enter> key to execute command in step 6.
8. brjtag rom will be programed on target. 


remark: on step 6, you can short "sck opt" JP, then the bootloader will not exit,
and you have enough time to do succeed steps. other avr isp can replace the 2nd usbasp,
but have to modify bat file by youself.If you have a bootloader in target, step 1-4 can
be bypassed.


currently, this rom is tested on standard usbasp hardware, Mega 8, with 12Mhz OSC.

/cable:x

enjoy!

hugebird
2010.9
(1.9m)