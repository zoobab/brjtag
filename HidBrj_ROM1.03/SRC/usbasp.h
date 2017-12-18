



#define EXTREAME_SPEED 1        // change to '0' if want safe speed


#define U_HWVER_MAJ   0x01      //brjtag AVR MCU firmware ver 1.01 on USBASP hardware
#define U_HWVER_MIN   0x03

#define MAX_TIMEOUT    250

// EJTAG Instruction Registers
#define INSTR_EXTEST     0x00
#define INSTR_IDCODE     0x01
#define INSTR_SAMPLE     0x02
#define INSTR_IMPCODE    0x03
#define INSTR_ADDRESS    0x08
#define INSTR_DATA       0x09
#define INSTR_CONTROL    0x0A
#define INSTR_EJTAGALL   0x0B
#define INSTR_BYPASS     0xFF
#define INSTR_EJTAGBOOT  0x0C
#define INSTR_NORMALBOOT 0x0D

// EJTAG Control Register Bit Masks ---
//byte0
#define TOF             (1UL<< 1 ) //ClkEn,  permit DCLK out
#define TIF             (1UL<< 2 )
#define BRKST           (1UL<< 3 ) //Indicate cpu in debug mode     
#define DINC            (1UL<< 4 ) //Increase Address auto on DMA
#define DLOCK           (1UL<< 5 ) //lock bus for DMA
//byte1
#define DRWN            (1UL<< 9 ) // DMA R or W   
#define DERR            (1UL<< 10) //Indicate a error occur on DMA
#define DSTRT           (1UL<< 11) //DMA xfer start
#define JTAGBRK         (1UL<< 12) //generate a JTAG debug interupt
#define SETDEV          (1UL<< 14) //prob trap
#define PROBEN          (1UL<< 15) //cpu probe is enabled by deug support block
//byte2
#define PRRST           (1UL<< 16) //Reset cpu
#define DMAACC          (1UL<< 17) //Request DMA
#define PRACC           (1UL<< 18) //PraCC interactive ext cpu probe.
#define PRNW            (1UL<< 19) //Indicate cpu R or W activity
#define PERRST          (1UL<< 20)
#define SYNC            (1UL<< 23)
//byte3
#define DNM             (1UL<< 28)
#define ROCC            (1UL<< 31) 

/* define data type for MIPS32 */
#define MIPS_BYTE        0UL 
#define MIPS_HALFWORD    1UL 
#define MIPS_WORD        2UL 
#define MIPS_TRIPLEBYTE  3UL 

#define DMASZ(x)         ((x) << 7)   //make sz flag for DMA xfer


#define UBYTE(x)      ((x) >> 3)       //byte offset
#define UBIT(x)       ((x) &  7)       //bit offset in byte

#define __LE			0		// Little Endian
#define __BE      1   // Big Endian

//---------------------------------------------------------
//       _____             _______            _______
//      | DUT |___________|TAP/MCU|__________|PC/HOST|
//       -----    JTAG     -------     USB    -------
//        BCM               MCU FW            BRJTAG
//---------------------------------------------------------

/*     MCU Processing State Machine      */
#define ST_MCU_IDLE     0x01     //ideal, can accept any cmd
#define ST_MCU_XFER     0x02     //on USB data xfer, not accept any new cmd
#define ST_MCU_XFERR    0x03     //on USB data xfer, not accept any new cmd
#define ST_MCU_BUSY     0x04     //on JTAG process
#define ST_MCU_RDY      0x05     //DMA Read/write complete, data reply buffer ready

//   idle -> xfer dl -> busy -> ready -> xfer ul -> idle


/*      Host <-> MCU  COMMAND ID (rq->bRequest)   */
#define REQ_MCU_HWVER          0x11        //EPIN,get MCU firmware version
#define REQ_MCU_RESET          0x12        //EPIN,reset tap to idle
#define REQ_MCU_SETSPD         0x13        //EPIN,set jtag speed
#define REQ_MCU_GetSTT         0x14        //EPIN,get MCU current state
#define REQ_MCU_CMDSEQ         0x21        //EPOUT,send cmd sequence to mcu
#define REQ_MCU_BITSEQ         0x22        //EPOUT,send bit sequence to mcu
#define REQ_MCU_GetDAT         0x23        //EPIN,get back sequence response data



//cmd sequence format (total length <= 254 bytes)
//<[cmd id 0][cmd data 0]><[cmd id 1][cmd data 1]><3><4> ... 
//
// Command ID:
#define CMD_TAP_DELAY           0x01        //pause DUT <-> TAP 
//In:  [ 0:id ][1 : us]              out:none
#define CMD_TAP_CLKIO           0x02        //bit-wise clock shift in/out, <tdi,tms>
//In: [0: id][1,2:bit len][3,4,...,x+2][x+3,...,2x+2]    out:[0,..., x-1], x=(bit+7)/8
#define CMD_TAP_DETIR           0x03        //IR shift/scan a instruction and return
//In: [0:id][1:ir bit len][2,3,4,5 ir data]     out:[0,1,2,3]
#define CMD_TAP_SETIR            0x04        //IR shift/scan a instruction
//In: [0:id][1:ir bit len][2,3,4,5 ir data]     out:none
#define CMD_TAP_DR32            0x05        //DR shift/scan a 32b Data
//In: [0:id][1,2,3,4 ir data]        out:[0,1,2,3]
#define CMD_TAP_DMAREAD        0x06        //DMA Read a 32b data
//In: [0:1d][1:type][2,3,4,5:ADDR]  out:[0,1,2,3]
#define CMD_TAP_DMAWRITE        0x07        //DMA write a 32b data
//In: [0:1d][1:type][2,3,4,5:ADDR][6,7,8,9:data]     out:None
#define CMD_TAP_DMABLKRD32        0x08        //DMA Read a data block
//In: [0:1d][1:type][2,3,4,5:ADDR][6:len, mode in type]  out:[len in data mode]
#define CMD_TAP_FLSHBLKWR         0x09        //flash blk write
//In: <cmd head><data 0 seq><data 1 seq>...<data 7 seq>
//<cmd head>: <[0:1d][1:type][2,3,4,5:BASE ADDR][6:len][7,8,9,10:WR Target ADDR]>
//<data x seq>:<0:I1><1:I2><2:I3><3:D><4,5,6,7:data><8:L><9:us>
//I: 0x80~0x8F, flash unlock cmd, I1~I3 for AMD is 0xAA, 0x55, 0xA0.
//         if open bypass mode, only I3 used 0xA0 at datax seq offset 0
//D: 0x00, follow 4 bytes are data, whatever the write type(x8,x16)
//L: 0xFF, means flash program waiting, 
//         waiting time is give in follow us bytes
//for a word program, maximum write step is 5, (I1,I2,I3,D,L)
//for bypass mode, write step is 3, (I3,D,L)
//
//<out>: None
//
//
//type:
#define CMDT_MOD        0x03      //op type mask, word, halfw, byte,tripw
#define CMDT_END       (1<<2)     //endian mask
#define CMDT_DMA       (1<<3)     //1:DMA       2:Pracc
#define CMDT_IRLEN(x)  (((x) >>4)&0x0F)      //ir len mask
                                           //if irlen>16, set this field to 0,
                                           //and send a CMD_TAP_SETIR at very first
//****************************************************************************



/*      Hardware Definition        */

#define LED_PORT_DDR     DDRC
#define LED_PORT_OUTPUT  PORTC
#define LED_Green        0
#define LED_Red          1

#define ledRedOn()       PORTC &= ~(1 << LED_Red)
#define ledRedOff()      PORTC |=  (1 << LED_Red)
#define ledRedToggle()   PORTC ^=  (1 << LED_Red)

#define ledGreenOn()     PORTC &= ~(1 << LED_Green)
#define ledGreenOff()    PORTC |=  (1 << LED_Green)
#define ledGreenToggle() PORTC ^=  (1 << LED_Green)

#define SPI_DIV_2()      ((1 << SPE) | (1 << MSTR) | (1 << DORD))
#define SPI_DIV_8()      ((1 << SPE) | (1 << MSTR) | (1 << DORD) | (1 << SPR0))

#define UTCK      5
#define UTMS      2 
#define UTDI      3
#define UTDO      4

#define TAP_ALL_MASK       (1<<UTCK |1<<UTDI |1<<UTMS |1<<UTDO )
#define TAP_ALL_UMASK      (~ TAP_ALL_MASK)

#define TAP_OUT_MASK       (1<<UTDI |1<<UTMS)
#define TAP_OUT_UMASK      (~ TAP_OUT_MASK)

#define TAP_PIN_MSK(sig,x)       ((x?1:0)<<sig)
#define TAP_OPIN_VAL(tms,tdi)    (TAP_PIN_MSK(UTMS,tms)|TAP_PIN_MSK(UTDI,tdi))

#define TAP_OUT_SETVAL(tms,tdi)  PORTB = (PORTB & TAP_OUT_UMASK) | TAP_OPIN_VAL(tms,tdi)

#define TAP_PIN_SET(x)     PORTB |=  (1 << (x))
#define TAP_PIN_CLR(x)     PORTB &= ~(1 << (x))
#define TAP_PIN_TOG(x)     PORTB ^=  (1 << (x))
#define TAP_PIN_GET(x)     ( PINB & (1 << (x)) )

#define TAP_PIN_OUTPUT(x)  DDRB |=  (1 << (x)) 
#define TAP_PIN_INPUT(x)   DDRB &= ~(1 << (x))

#define TAP_DDR_INIT()  DDRB  = (DDRB & TAP_ALL_UMASK) | TAP_OUT_MASK | (1<< UTCK)
#define TAP_PIN_INIT()   \
            PORTB = (PORTB & TAP_ALL_UMASK) | 1<<UTMS;                  \
            DDRB  = (DDRB & TAP_ALL_UMASK) | TAP_OUT_MASK | (1<< UTCK); \
            SPSR = (1 << SPI2X);

#define TAP_PIN_CLOSE()   DDRB &= TAP_ALL_UMASK)

                         

/*             
// idle to Shift DR
#define TAP_ST_ToSDR()  TAP_STmove(0x01,3)
// idle to Shift IR
#define TAP_ST_ToSIR()  TAP_STmove(0x03,4)
//Exit-xR to idle
#define TAP_ST_ExRtoIDLE()  TAP_STmove(0x01,2)
*/

#if (EXTREAME_SPEED)
#define CLK_LAT()

#else
#define CLK_LAT()   \
         if(cycle_delay) _delay_loop_1(cycle_delay)
#endif

#define TAP_SFTMS_0()    \
			TAP_PIN_CLR(UTMS); \
  		TAP_PIN_SET(UTCK); \
      CLK_LAT();         \
		  TAP_PIN_CLR(UTCK)
			
		
#define TAP_SFTMS_1()    \
			TAP_PIN_SET(UTMS); \
  		TAP_PIN_SET(UTCK); \
  		CLK_LAT();         \
		  TAP_PIN_CLR(UTCK)


#define TAP_ST_ToSDR()   \
          TAP_SFTMS_1(); \
          TAP_SFTMS_0(); \
          TAP_SFTMS_0()
         
#define TAP_ST_ToSIR()   \
          TAP_SFTMS_1(); \
          TAP_SFTMS_1(); \
          TAP_SFTMS_0(); \
          TAP_SFTMS_0()
         
#define TAP_ST_ExRtoIDLE()  \
          TAP_SFTMS_1();    \
          TAP_SFTMS_0()         


// TAP state machine
#define  JTAG_UNK  0
#define  JTAG_TLR  1 
#define  JTAG_RTI  2
#define  JTAG_PDR  3
#define  JTAG_PIR  4
#define  JTAG_SDR  5
#define  JTAG_SIR  6

void inline my_delayus(uint16_t us);
//void inline mydelay(uint8_t clk);
void TAP_clock_bits(uint8_t* ptdi, uint8_t* ptms, uint8_t* ptdo, uint16_t len);
void TAP_DetInstr(uint8_t* instr, uint8_t* out);
void TAP_SetData32(uint32_t *ptdi,uint32_t *ptdo);
void TAP_SetInstr(uint32_t* instr);
void TAP_ST_Reset(void);
void TAP_STmove(uint8_t tms,uint8_t len);
void TAP_DMA_WR(uint32_t* odata);
void TAP_DMA_RD(uint32_t * idata);
void TAP_DMA_BLKRD(uint32_t * idata, uint8_t len);
void TAP_clock_32(uint8_t* ptdi, uint8_t* ptdo, uint8_t len);
uint8_t TAP_FLSH_BLKWR(uint8_t * pin, uint8_t len);