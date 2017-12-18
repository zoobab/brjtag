/* Name: main.c
 * Project: hid-custom-rq example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: main.c 692 2008-11-07 15:07:40Z cs $
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
We assume that an LED is connected to port B bit 0. If you connect it to a
different port or bit, change the macros below:
*/


#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */
#include "usbasp.h"       /* The custom request numbers we use */


extern void *memcpy(void *, const void *, size_t);
extern uint8_t cycle_delay;
extern uint8_t spi_speed;
extern uint32_t  gAddr;
extern uint8_t   gInstrlen;
extern uint8_t   gMode;
extern uint8_t st_jtag;

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM char usbHidReportDescriptor[22] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0xc0                           // END_COLLECTION
};
/* The descriptor above is a dummy only, it silences the drivers. The report
 * it describes consists of one byte of undefined data.
 * We don't transfer our data through HID reports, we use custom requests
 * instead.
 */
 

/*
typedef union usbWord{
    unsigned    word;
    uint8_t       bytes[2];
}usbWord_t;

typedef struct usbRequest{
    uint8_t       bmRequestType;       // data[0]
    uint8_t       bRequest;            // data[1]
    usbWord_t   wValue;              // data[2],[3]    [3]<<8 | [2]
    usbWord_t   wIndex;              // data[4],[5]
    usbWord_t   wLength;             // data[6],[7]
}usbRequest_t;
*/

/* ------------------------------------------------------------------------- */

static uint8_t inputDataBuf[254];      // mcu input data buffer
static uint8_t outputDataBuf[140];     // mcu output data buffer
static uint8_t replyBuf[8];            // short output buffer

static uint8_t usbTotalLen=0;         // usb xfer total len 
static uint8_t usbCurrentOffset=0;    // usb xfer cur offset on large data
static uint8_t usbBytesRemaining=0;   // usb xfer remaing bytes on large data

static uint8_t  mcuProcessedBytes;   //processed bytes on in-buffer
static uint8_t  mcuReadBytes;        //jtag scan in bytes to out-buffer
static uint8_t  mcu_cmd;             //latest cmd processing
static uint8_t  mcu_st;              //mcu processing state machine


static void my_post(void);


usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
usbRequest_t    *rq = (void *)data;
uint8_t len = 0;


 if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
        DBG1(0x50, &rq->bRequest, 1);   /* debug output: print our request */
        
        
   switch(rq->bRequest)       //cmd id in bRequest
   {
     case REQ_MCU_HWVER:
     	      if(mcu_st != ST_MCU_XFER)
     	       {
     	        replyBuf[0] = 'B';     
     	        replyBuf[1] = 'r';     
	            replyBuf[2] = U_HWVER_MIN;     
	            replyBuf[3] = U_HWVER_MAJ;
	            len = 4;     
	           }

	  			    break;                          

     case REQ_MCU_RESET:
     	      if(mcu_st != ST_MCU_XFER)
     	       {
              TAP_PIN_INIT();
              my_delayus(1000);       //1ms
              TAP_ST_Reset();
	            replyBuf[0] = 0;
	            len = 1;     
	            mcu_st = ST_MCU_IDLE;
	           }
	      ledRedOff();
				ledGreenOff();
	  			    break;     

     case REQ_MCU_SETSPD:
     	     if((mcu_st != ST_MCU_XFERR) &&(mcu_st != ST_MCU_XFER) && (mcu_st != ST_MCU_BUSY))
     	       {
                  if (rq->wValue.bytes[0])
                        cycle_delay = 1;       //low speed, 1.5MHz
                     else
                      	cycle_delay = 0;       //high speed, 6MHz
                  spi_speed = cycle_delay?SPI_DIV_8():SPI_DIV_2();    	
                  replyBuf[0] = 0;          	       	
                  len = 1;
              }
							break;   
								                          
	   case REQ_MCU_GetSTT:
    	        {
     	         replyBuf[0] = mcu_st;     
     	         len = 1;     
	            }
							break;                        
								   
     case REQ_MCU_GetDAT:
     	    if(mcu_st == ST_MCU_RDY)
     	    	{
     	    		ledGreenOn();
     	    		mcu_st = ST_MCU_XFERR;
     	    		mcu_cmd = REQ_MCU_GetDAT;

         	    if(rq->wLength.word < mcuReadBytes)
         	    	  usbBytesRemaining = rq->wLength.bytes[0];    //xfer not > 255
         	    	else
 	           	    usbBytesRemaining = mcuReadBytes;         		

/*
                  if(usbBytesRemaining ==0)      // all trunks xfer completd
    	              {
    		              mcu_st = ST_MCU_IDLE;
    	              	mcuReadBytes = 0;
    		              mcuProcessedBytes = 0;
    	                }
*/ 
              usbCurrentOffset = 0;
              return USB_NO_MSG;  /* mcu uses usbFunctionRead() to send large data */    	    		
            }
								   break;                        

     case REQ_MCU_CMDSEQ:
     case REQ_MCU_BITSEQ:
     	    if(mcu_st == ST_MCU_IDLE) //we not allow SET cmd override, eg. host must fetch data out before accept new SET cmd
     	    	{
     	    		ledGreenOn();
     	    		mcu_st = ST_MCU_XFER;
     	    		mcu_cmd = rq->bRequest;
         	    usbTotalLen = rq->wLength.bytes[0];
         	    usbBytesRemaining = usbTotalLen;
              usbCurrentOffset = 0;
              return USB_NO_MSG;  /* mcu uses usbFunctionWrite() to obtain data */    	    		
            }
								   break;   
      
      default:
      	           break;
	 }	   
			
	 usbMsgPtr = replyBuf;
   return len;     //complete USBRQ_TYPE_VENDOR 

  }else{
        /* calss requests USBRQ_HID_GET_REPORT and USBRQ_HID_SET_REPORT are
         * not implemented since we never call them. The operating system
         * won't call them either because our descriptor defines no meaning.
         */
  }
   return 0;   /* default for not implemented requests: return no data back to host */
}


/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uint8_t   usbFunctionRead(uint8_t *data, uint8_t len)
{
    if(len > usbBytesRemaining)
        len = usbBytesRemaining;
    
    memcpy(data, outputDataBuf+usbCurrentOffset, len);  // copy mcu out-buffer to usb buffer
    usbCurrentOffset += len;
    usbBytesRemaining -= len;
    if(usbBytesRemaining ==0)      // all trunks xfer completed
    	{
     	 	ledGreenOff();
    		mcu_st = ST_MCU_IDLE;
//    		mcuReadBytes = 0;
//    		mcuProcessedBytes = 0;
    	}
    return len;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uint8_t   usbFunctionWrite(uint8_t *data, uint8_t len)
{
  uint8_t flag;
  
  flag = (usbBytesRemaining==0);
  
  if (!flag)                  // still have trunk need xfer
  	{
    if(len > usbBytesRemaining)
        len = usbBytesRemaining;
    memcpy(inputDataBuf+usbCurrentOffset,data,len);    //copy usb data to mcu in-buffer
    usbCurrentOffset += len;
    usbBytesRemaining -= len;
    flag = (usbBytesRemaining==0);          // test xfer completed again
   }
   
   if (flag)                  //all trunks xfer completed
   	{
   	 mcu_st = ST_MCU_BUSY ;
     mcuReadBytes = 0;        //reset on-processing index
     mcuProcessedBytes = 0;
     ledRedOn();              //jtag start
     ledGreenOff();
   	}

    return flag; /* return 1 if this was the last chunk */
}

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

int main(void)
{
uint8_t   i;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    DBG1(0x00, 0, 0);       /* debug output: main starts */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    odDebugInit();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        my_delayus(1000);
    }
    usbDeviceConnect();
    ledGreenOn();   /* make the LED bit an output */
    sei();
    DBG1(0x01, 0, 0);       /* debug output: main loop starts */

    mcu_st = ST_MCU_IDLE;
    for(;;){                /* main event loop */
#if 0   /* this is a bit too aggressive for a debug output */
        DBG2(0x02, 0, 0);   /* debug output: main loop iterates */
#endif
        wdt_reset();
        usbPoll();
        my_post();

    }
    return 0;
}

// It is my time to processing input data
void my_post(void)
{

   uint16_t bitlen1 = 0;
   uint8_t  bitlen = 0;
   uint8_t  bytelen = 0;
   uint8_t  cmdlen = 0;
   uint8_t  datalen = 0;
   uint8_t  irlen = 0;
   uint8_t*  cmdptr = inputDataBuf;
   uint8_t seqcmd;

while(mcu_st == ST_MCU_BUSY)
    {
 
           cmdptr = inputDataBuf + mcuProcessedBytes; 
           seqcmd = *cmdptr;          	  		
           switch(seqcmd)
           {
            case CMD_TAP_DELAY:
            	  //In:  [ 0:id ][1 : us]
                cmdlen = 2;
                uint8_t tm = *(cmdptr+1);
                my_delayus((uint16_t)tm);
                datalen = 0;
                break;
            	
            case CMD_TAP_CLKIO:
       	  	    //In: [0: id][1,2:bit len][3,4,...,x+2][x+3,...,2x+2] 
       	  	    bitlen = *(cmdptr + 1);
       	  	    bitlen1 = *(cmdptr + 2)*8 + bitlen;
       	        bytelen = ((bitlen1 +7) >>3);
                if(bitlen>960)
                	{ cmdlen = 0;
                		datalen = 0;
                		mcuProcessedBytes = usbTotalLen;
                		break;
                	}
       	        TAP_clock_bits(cmdptr+3, cmdptr+3+bytelen, outputDataBuf+mcuReadBytes, bitlen1);   
                cmdlen = 3+bytelen*2;
                datalen = bytelen;
                break;               
                                	
            case CMD_TAP_DETIR:
                //In: [0:id][1:ir bit len][2,3,4,5 ir data]
       	  	    gInstrlen = *(cmdptr + 1);
       	        bytelen = ((gInstrlen +7) >>3); 
                TAP_DetInstr((cmdptr+2), outputDataBuf+mcuReadBytes);  
                cmdlen = 2+bytelen;
                datalen = bytelen;
                break;     
            	
            case CMD_TAP_SETIR:
            	  //In: [0:id][1:ir bit len][2,3,4,5 ir data]	
       	  	    gInstrlen = *(cmdptr + 1);
       	        bytelen = ((gInstrlen +7) >>3); 
                if(st_jtag != JTAG_RTI) { TAP_ST_Reset();}
                TAP_SetInstr((uint32_t *)(cmdptr+2));   
                cmdlen = 2+bytelen;
                datalen = 0;
                break;       
            		
            case CMD_TAP_DR32:
                //In: [0:id][1,2,3,4 ir data]
                if(st_jtag != JTAG_RTI) { TAP_ST_Reset();}
                TAP_SetData32((uint32_t *)(cmdptr+1), (uint32_t *)(outputDataBuf+mcuReadBytes));   
                cmdlen = 5;
                datalen = 4;
                break;    
            	
            case CMD_TAP_DMAREAD:
                // [0:1d][1:type][2,3,4,5:ADDR]
         	  	  gMode = (*(cmdptr + 1)) & CMDT_MOD;
         	  	  irlen = CMDT_IRLEN(*(cmdptr + 1));
         	  	  if(irlen) gInstrlen = irlen;
         	  	  gAddr = *(uint32_t *)(cmdptr + 2);
         	  	  TAP_DMA_RD((uint32_t*)(outputDataBuf+mcuReadBytes));
                cmdlen = 6;
                datalen = 4;
                break;                  	
            	
            case CMD_TAP_DMAWRITE:	               	
                // [0:1d][1:type][2,3,4,5:ADDR][6,7,8,9:data]
         	  	  gMode = (*(cmdptr + 1)) & CMDT_MOD;
         	  	  irlen = CMDT_IRLEN(*(cmdptr + 1));
         	  	  if(irlen) gInstrlen = irlen;
         	  	  gAddr = *(uint32_t *)(cmdptr + 2);
         	  	  TAP_DMA_WR((uint32_t*)(cmdptr + 6));
                cmdlen = 10;
                datalen = 0;
                break;                  	
           	
            case CMD_TAP_DMABLKRD32:
                // [0:1d][1:type][2,3,4,5:ADDR][6:len]
         	  	  gMode = MIPS_WORD;
         	  	  irlen = CMDT_IRLEN(*(cmdptr + 1));
         	  	  if(irlen) gInstrlen = irlen;
         	  	  gAddr = *(uint32_t *)(cmdptr + 2);
         	  	  datalen = *(cmdptr + 6);
         	  	  if (datalen >32) datalen = 32;     //not > 16 DWORD per cmd
        	  	  TAP_DMA_BLKRD((uint32_t*)(outputDataBuf+mcuReadBytes),datalen );
         	  	  datalen <<= 2;
                cmdlen = 7;
                break;      

            case CMD_TAP_FLSHBLKWR:
                // <[0:1d][1:type][2,3,4,5:ADDR][6:len]><7,8,9,10 target addr> <data seq>
         	  	  gMode = (*(cmdptr + 1)) & CMDT_MOD;
         	  	  irlen = CMDT_IRLEN(*(cmdptr + 1));
         	  	  if(irlen) gInstrlen = irlen;
         	  	  gAddr = *(uint32_t *)(cmdptr + 2);
         	  	  datalen = *(cmdptr + 6);
         	  	  if (datalen >24) datalen = 24;     //not > 24 WORD per cmd
        	  	  cmdlen = 7 + TAP_FLSH_BLKWR(cmdptr+7,datalen );
         	  	  datalen =0;       //no read;
                break;      

           	default:
           		   cmdlen = 1;     // not recognized cmd, go to next
           		   datalen = 0;
                 break;
           }

       	   mcuProcessedBytes += cmdlen;
       	   mcuReadBytes += datalen;


           if (mcuProcessedBytes >= usbTotalLen )
           {
           	 	    if (mcuReadBytes)
           	 	    	 mcu_st = ST_MCU_RDY;
           	 	     else
           	 	     	 mcu_st = ST_MCU_IDLE;
           }

     }//while
	
	ledRedOff();  //jtag op complete

}

/* ------------------------------------------------------------------------- */
