Hid-Brjtag v2 is working with STM32F103C8TC. The hardware platform could be modified on ST mini 3in1 devlopment kit.
STM32F.pdf shows the required PINs used by Hid-Brjtag v2, You can make yourself hidbrjtag dongle according to.

File list as below. There are two version for using or unusing external 8MHz HSE OSC.
Even though the cost is lower, the total performance of HSI is about 80% of using HSE.

1.Utility of Bootloader
  stload.exe

2.Bootloader and HIDBrjtag v2 rom for 8MHz HSE
  hidbl_stm32_hse.hex
  hidbrjhse.hex

3.Bootloader and HIDBrjtag v2 rom for onchip RC HSI
  hidbl_stm32_hsi.hex
  hidbrjhsi.hex

The Bootloader will occupy about 8KB flash space from 0x08000000 to 0x0801FFFF
The functional HID-Brjtag v2 Rom should occupy about 15KB flash space from 0x08020000.


Working steps on a nake board:

1.jump BOOT1(Boot0 pin) from 0 to 1, let mcu boot from system memory once power on. This should active internal USART bootloader
2.connect a RS232-TTL board with MCU USART1_TX, USART1_RX and GND pin.
3.power on MCU board, using a ISP tool read mcu info.(www.mcuisp.com)
4.program usb bootloader file hidbl_stm32_hse.hex or hidbl_stm32_hsi.hex on mcu according to your hardware
5.power of mcu board. take off RS232-TTL connections from mcu board.jump BOOT1 to 0
6.short USART1_TX and USART1_RX pin with a short jumper. let usb bootloader wait to program Hid-Brjtag rom
7.power on, led ld2 and ld3 should be off. windows find HID-BRJTAG hardware
8.open a cmd window in ROM folder, and execute cmd: "stload hidbrjhse.hex" or "stload hidbrjhsi.hex"
9 when write complete, no need power off, just take of USART_TX short jumper, and press <enter> key
10.Hid-Brjtag v2 is ready



hugebird
2010.11.13 