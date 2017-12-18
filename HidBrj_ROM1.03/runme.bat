@echo off
@echo I will program Brjtag ROM to target USBASP
@echo Please confirm JP "Self Programing" is shorted
pause
call proguasp 0xc0 0x9f .\