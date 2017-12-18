brjtag.a   staic link obj£¬ no need install lib-usb 0.1.12 and ftdid2xx lib
brjtag     dynamic link obj, require install lib-usb and ftdid2xx lib


usb supporting requires libusb legacy version 0.1.12
download D2xx for linux version 0.4.16 from http://www.ftdichip.com/Drivers/D2XX.htm
in driver package, libusb 0.1.12 is included.

step 1, compile and install libusb.
in libusb folder
./configure
./make
./make install
all libusb lib can found in /usr/local/lib

step 2, install d2xx lib
copy d2xxinstall shell scripts in un-taggz-ed d2xx driver folder and run.
./d2xxinstall
the scripts will copy ftdid2xx dynamic lib to /usr/local/lib, and create two soft links for link-time and run-time
check d2xx driver folder readme.dat
modify /etc/fstab

if using a kernel 2.6.xx
running occur: libftd2xx.so.0 cannot restore segment prot after reloc: Permission denied
try
chcon -t texrel_shlib_t /usr/local/lib/libftd2xx.*