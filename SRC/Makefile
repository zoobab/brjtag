
CFLAGS = -Wall -O2 -I. -I./lnx

#gcc link to *.so & *.a libs
#app running needs *.so.0 libs
#for dynamic make, *.so libs placed in /usr/local/lib
#for static make, *.a libs placed  app-dir/libs
#so need pre-install libusb and ftd2xx libs
#add lib curses for getch()    -lcurses

DL_PATH = -Wl,-rpath /usr/local/lib
L_LIBS = -L/usr/local/lib -lftd2xx -lusb
S_LIBS = -ldl -lpthread lnx/libftd2xx.a lnx/libusb.a

OBJS = brjtag.o busbasp.o ftdixx.o j-link.o libusb.o stmhid.o


all: brjtag brjtag.a

brjtag: $(OBJS)
	gcc $(CFLAGS) -o brjtag $(OBJS) $(DL_PATH) $(L_LIBS) 

brjtag.a: $(OBJS)
	gcc $(CFLAGS) -o brjtag.a $(OBJS) $(DL_PATH) $(S_LIBS)
clean:
	rm -rf *.o *~ brjtag brjtag.a
