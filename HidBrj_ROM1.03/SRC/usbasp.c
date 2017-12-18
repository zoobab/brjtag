
#include <avr/io.h>
#include <util/delay_basic.h>
#include "usbasp.h"


uint8_t st_jtag = JTAG_UNK;

#if (EXTREAME_SPEED)
uint8_t cycle_delay = 0;
uint8_t spi_speed = SPI_DIV_2();  // hw spi speed;
#else
uint8_t cycle_delay = 1;
uint8_t spi_speed = SPI_DIV_8();  // hw spi speed;
#endif
uint32_t  gAddr;               //dma rw address
uint8_t   gInstrlen;           //instruction length
uint8_t   gMode;               //dma op mode


static uint8_t ptmp[8];        //dummy


static uint32_t flshcmd[16] = {
	       0x00AA00AA, 0x00550055,0x00A000A0,      // AMD x16
         0xAAAAAAAA, 0x55555555,0xA0A0A0A0,      // AMD x8
 	       0x00AA00AA, 0x00550055,0x00A000A0,      // SST x16
         0x00500050, 0x00400040,                 // Intel x16
         0x50505050, 0x40404040,                 // Intel x8
         0,0,0};
         
static uint32_t flshadr[16] = {
	       0x00000AAA, 0x00000554,0x00000AAA,      // AMD x16
         0x00000AAA, 0x00000555,0xA0A0A0A0,      // AMD x8
 	       0x0000AAAA, 0x00005554,0x0000AAAA,      // SST x16
         0, 0,                                   // Intel x16
         0, 0,                                   // Intel x8
         0,0,0};


void inline my_delayus(uint16_t us)
{
	if(us == 0 ) return;
		else if (us <= 64) _delay_loop_1((us<<2)&0xFF);
	  else if (us <= 128) {_delay_loop_1(0);_delay_loop_1((us<<2)&0xFF);}
			else if (us < 21840U) _delay_loop_2((us<<1) + us);
	  else
	  	while(us > 20000U)
	  	  {
	  	  	 _delay_loop_2(60010U);
	  		  us -=20000U;
	  		}
	  _delay_loop_2((us<<1) + us);
}

/*
// for tck latency
void inline mydelay(uint8_t clk) 
{

while (clk--)
 { __asm__ __volatile__ ("nop"); } 

}
*/

//-----------------------------------------------------------------------------
void TAP_STmove(uint8_t tms,uint8_t len)
{
	uint8_t msk = 1;

	while(len--)
	{
		if(tms & msk)
			{TAP_SFTMS_1();}
		 else
			{TAP_SFTMS_0();}

		msk <<= 1;
	}
}

// Reset TAP to idle                         
void TAP_ST_Reset(void)
{
	 TAP_STmove(0x1F,6);
   st_jtag = JTAG_RTI;
}

//--------------------------------------------------------------------------------
// max len = 8x240 = 1920 bits,  100Khz clk, total 16ms
void TAP_clock_bits(uint8_t* ptdi, uint8_t* ptms, uint8_t* ptdo, uint16_t len)
{
	uint8_t msk=1;
	uint8_t bflag = 0;
	uint16_t i;

//	uint8_t byte_ofs = 0;

	uint8_t bit_ofs = 0;
	
  if(len == 0 || len >= 960) return ;
  if(ptms)                    //tms buffer presented
  	{  bflag = 1;             //bit by bit mode
       st_jtag = JTAG_UNK;    //after bit by bit shift, MCU dn't know jtag state
     }  		

   *(ptdo)=0;
   
	for(i=0; i<len; i++)
	{
//    byte_ofs = UBYTE(i);      // Byte offset
    bit_ofs  = UBIT(i);         // bit offset
    if(i>0 && !bit_ofs) 
    	{ 
    		++ptdo;
    		*(ptdo)=0;              //clean input on new byte
    		++ptdi;
    		++ptms;
    		msk = 1;
    	}
    
    if (bflag)
     {
    	 if( *(ptms) & msk ) 
    	 	   TAP_PIN_SET(UTMS);
    	 	else
           TAP_PIN_CLR(UTMS);
     }
      else
     {  
      	if(i == len-1)              //final bit, need set tms let DR or IR update
      		 TAP_PIN_SET(UTMS);       // on Exit-xR state
      	else
      		 TAP_PIN_CLR(UTMS);	 
     }
      
     if( *(ptdi) & msk )            //set tms/tdi
    	 	   TAP_PIN_SET(UTDI);
    	else
           TAP_PIN_CLR(UTDI);
 
 		 if(TAP_PIN_GET(UTDO) )         //get in tdo at +Ve
			 *(ptdo) |= msk;		    
      

		 TAP_PIN_SET(UTCK);            // tck high, let bit shift
     CLK_LAT();
		 TAP_PIN_CLR(UTCK);            // tck low
			 
		 msk <<= 1;

	 }
	 
   if(!bflag) TAP_ST_ExRtoIDLE();
}		


//------------------------------------------------------------------------------
// clock func must be optimized
// for 128KB DMA read, total spend ~25s
//   usb xfer from mcu to pc take 8s
//   Hw spi jtag 3 bytes data take 7s (at fosc/2)
//   other take 10s (instr 4s + 1 byte data 6s), sw jtag clk estimate ~fosc/5

/*
// total sw mode
void TAP_clock_32sw(uint8_t* ptdi, uint8_t* ptdo, uint8_t len)
{

	uint8_t msk=1;
	uint16_t i;

	uint8_t bit_ofs = 0;
	
   *(ptdo)=0;
   TAP_PIN_CLR(UTMS);
   
	for(i=0; i<len; i++)
	{
    bit_ofs  = UBIT(i);       // bit offset
    if(i>0 && !bit_ofs) 
    	{ 
    		++ptdo;
    		*(ptdo)=0;    //clean input on new byte
    		++ptdi;
    		msk = 1;
    	}
    
      	if(i == len-1)       //final bit, need set tms let DR or IR update
      		 TAP_PIN_SET(UTMS);         // on Exit-xR state

      
     if( *(ptdi) & msk )    //set tms/tdi
    	 	   TAP_PIN_SET(UTDI);
    	else
           TAP_PIN_CLR(UTDI);
 
 		 if(TAP_PIN_GET(UTDO) )         //get in tdo at +Ve
			 *(ptdo) |= msk;		    
      

		 TAP_PIN_SET(UTCK);            // tck high, let bit shift
     CLK_LAT();
		 TAP_PIN_CLR(UTCK);            // tck low
			 
		 msk <<= 1;

	 }
	 
   TAP_ST_ExRtoIDLE();

}	
*/


/* 
// method 1 
void TAP_clock_32(uint8_t* ptdi, uint8_t* ptdo, uint8_t len)
{

	uint8_t hwbytes = (len -1)>>3;
	uint8_t swbits = len - (hwbytes <<3);
	
	TAP_PIN_CLR(UTMS);

  if(hwbytes)
  	{
	   uint8_t i;
		 SPCR = spi_speed;  //enable hw spi(SPE), master , LSB first(DORD)
		 SPSR = (1 << SPI2X);                         //spi rate : (spi2x,spr1,spr0)=(1,0,0) fosc/2	

   	for(i = 0; i< hwbytes; i++)
	   {
	
	     SPDR = *(ptdi++);

	     while (!(SPSR & (1 << SPIF))) ;
		
	     *(ptdo++) =  SPDR;	
     }

     SPCR = 0;      // hw spi disable
    }

 {
	uint8_t msk=1 ;
	uint8_t to =0;
	uint8_t ti =*ptdi;    //get final byte tdi;
	
	while(swbits--)
	{

   	if(!swbits)                         //final bit, need set tms let DR or IR update
      		TAP_PIN_SET(UTMS);            // on Exit-xR state

     if(ti & msk )                      //set tdi
    	 	   TAP_PIN_SET(UTDI);
    	else
           TAP_PIN_CLR(UTDI);

    
		 if(TAP_PIN_GET(UTDO) )             //get in tdo, work!
			 to |= msk;		    
	

		 TAP_PIN_SET(UTCK);                 // tck high, let bit shift
     CLK_LAT();
		 TAP_PIN_CLR(UTCK);                 // tck low
		 
		 msk <<= 1;

	 }

	 *(ptdo) = to;
	}
   TAP_ST_ExRtoIDLE();

}	
*/

/*
//method 2
void TAP_clock_32(uint8_t* ptdi, uint8_t* ptdo, uint8_t len)
{

	uint8_t msk =0;
	uint8_t ti;
	uint8_t to  = (len -1)>>3;
	uint8_t swbits = len - (to <<3);
	
	TAP_PIN_CLR(UTMS);

  if(to)
  	{
		 SPCR = spi_speed;         //enable hw spi(SPE), master , LSB first(DORD)
		 SPSR = (1 << SPI2X);      //spi rate : (spi2x,spr1,spr0)=(1,0,0) fosc/2	

   	while(msk < to){
	
	     SPDR = ptdi[msk];

	     while (!(SPSR & (1 << SPIF))) ;
		
	     ptdo[msk] =  SPDR;	
	     msk++;

     }

     SPCR = 0;      // hw spi disable
     ptdi += msk;
     ptdo += msk;

    }

  msk = 1;
	to =0;
	ti =*ptdi;    //get final byte tdi;
	
	while(swbits--)
	{

   	if(!swbits)                         //final bit, need set tms let DR or IR update
      		TAP_PIN_SET(UTMS);            // on Exit-xR state

     if(ti & msk )                      //set tdi
    	 	   TAP_PIN_SET(UTDI);
    	else
           TAP_PIN_CLR(UTDI);

    
		 if(TAP_PIN_GET(UTDO) )             //get in tdo, work!
			 to |= msk;		    
	

		 TAP_PIN_SET(UTCK);                 // tck high, let bit shift
     CLK_LAT();
		 TAP_PIN_CLR(UTCK);                 // tck low
		 
		 msk <<= 1;

	 }

	 *(ptdo) = to;

   TAP_ST_ExRtoIDLE();

}	
*/


//method 3
void TAP_clock_32(uint8_t* ptdi, uint8_t* ptdo, uint8_t len)
{

	uint8_t msk =1;
	uint8_t ti;
	uint8_t to  = (len -1)>>3;
	uint8_t swbits = len - (to <<3);
	
	TAP_PIN_CLR(UTMS);

  if(to)
  	{
		 SPCR = spi_speed;         //enable hw spi(SPE), master , LSB first(DORD)
//		 SPSR = (1 << SPI2X);      //spi rate : (spi2x,spr1,spr0)=(1,0,0) fosc/2	

   	while(to--)
   	{
	
	     SPDR = *(ptdi++);

	     while (!(SPSR & (1 << SPIF))) ;
		
	     *(ptdo++) =  SPDR;	

     }

     SPCR = 0;      // hw spi disable
    }

	ti =*ptdi;    //get final byte tdi;
	to = 0;
	
	while(swbits--)
	{

   	if(!swbits)                         //final bit, need set tms let DR or IR update
      		TAP_PIN_SET(UTMS);            // on Exit-xR state

     if(ti & msk )                      //set tdi
    	 	   TAP_PIN_SET(UTDI);
    	else
           TAP_PIN_CLR(UTDI);

    
		 if(TAP_PIN_GET(UTDO) )             //get in tdo, work!
			 to |= msk;		    
	

		 TAP_PIN_SET(UTCK);                 // tck high, let bit shift
     CLK_LAT();
		 TAP_PIN_CLR(UTCK);                 // tck low
		 
		 msk <<= 1;

	 }

	 *(ptdo) = to;

   TAP_ST_ExRtoIDLE();

}	


//-------------------------------------------------------------------
void TAP_SetInstr(uint32_t* instr)
{
   static uint32_t in_0 =0xFFFFFFFF;

   if(in_0 == *instr )          //used instruction only 4b
        return;
   	 else
   	 		in_0 = *instr ;

	TAP_ST_ToSIR();
//	TAP_clock_bits(instr, 0, ptmp, (uint16_t)gInstrlen);
  TAP_clock_32((uint8_t *)instr, ptmp, gInstrlen);
}

void TAP_DetInstr(uint8_t* instr, uint8_t* out)
{
  if(st_jtag != JTAG_RTI) { TAP_ST_Reset();}
	TAP_ST_ToSIR();
//	TAP_clock_bits(instr, 0, out, (uint16_t)gInstrlen);
	TAP_clock_32(instr, out, gInstrlen);
}

void TAP_SetData32(uint32_t *ptdi,uint32_t *ptdo)
{
	TAP_ST_ToSDR();
//	TAP_clock_bits((uint8_t *)ptdi, 0, (uint8_t *)ptdo, 32);
	TAP_clock_32((uint8_t *)ptdi, (uint8_t *)ptdo, 32);
}



//------------------------------------------------------------------------

void TAP_DMA_RD(uint32_t * idata)
{
  uint32_t i1;
  uint32_t d1, d2;

  uint8_t timeout = MAX_TIMEOUT;
   
   if(st_jtag != JTAG_RTI) { TAP_ST_Reset();}
  
       i1 = INSTR_ADDRESS;
       TAP_SetInstr(&i1);
       TAP_SetData32(&gAddr,&d2);

       i1 = INSTR_CONTROL;
       TAP_SetInstr(&i1);
       d1 = DMAACC | DRWN | DMASZ(gMode) | DSTRT | PROBEN | PRACC;
       TAP_SetData32(&d1,&d2);

       d1 = DMAACC | PROBEN | PRACC;
       TAP_SetData32(&d1,&d2);

       while (d2  & DSTRT)
            { 
//            	my_delayus(1);
            	TAP_SetData32(&d1,&d2);
            	if(!(timeout--)) break;
            }
       	
	     i1 = INSTR_DATA;
       TAP_SetInstr(&i1);
       TAP_SetData32(&d1, idata);

/*
       i1 = INSTR_CONTROL;
       TAP_SetInstr(&i1);
       d1 = PROBEN | PRACC;
       TAP_SetData32(&d1,&d2);
       if (d2 & DERR)
        { 
         *idata = 0xFFFFFFFFUL;
        }
*/

}

//----------------------------------------------------------------------
void TAP_DMA_WR(uint32_t* odata)
{
  uint32_t i1;
  uint32_t d1, d2;
  uint8_t timeout = MAX_TIMEOUT;
  
    if(st_jtag != JTAG_RTI) { TAP_ST_Reset();}
    	
       i1 = INSTR_ADDRESS;
       TAP_SetInstr(&i1);
       TAP_SetData32(&gAddr,&d2);       
       
 	     i1 = INSTR_DATA;
       TAP_SetInstr(&i1);
       TAP_SetData32(odata,&d2);

       i1 = INSTR_CONTROL;
       TAP_SetInstr(&i1);
       d1 = DMAACC | DMASZ(gMode) | DSTRT | PROBEN | PRACC;
       TAP_SetData32(&d1,&d2);

       d1 = DMAACC | PROBEN | PRACC;
       TAP_SetData32(&d1,&d2);
       while (d2  & DSTRT)
            { 
//            	my_delayus(1);
            	TAP_SetData32(&d1,&d2);
            	if(!(timeout--)) break;
            }
}


//------------------------------------------------------------------------
void TAP_DMA_BLKRD(uint32_t * idata, uint8_t len)
{
  uint32_t * ptr = idata; 

  while (len-- >0)
  {
  	
  	TAP_DMA_RD( ptr );
	
    gAddr += 4UL;
    ptr ++;
      	
  }

}


//------------------------------------------------------------------------
uint8_t TAP_FLSH_BLKWR(uint8_t * pin, uint8_t len)
{
  uint32_t data, addr_bk,addr; 
  uint8_t k;
  uint8_t II;
  uint8_t adr_step;
  uint8_t adr_ofs = 0;
  uint8_t total=4;            //skip target addr

  addr_bk = gAddr;            //get base addr;
  addr = *(uint32_t *)(pin);  //get target addr
  
  adr_step = (gMode == MIPS_HALFWORD)?2:1;

 while(len--)
 {
   
  for(k=0; k<5; k++)
  {
    II = *(pin+total);
  	if(II == 0xFF)             //delay
  		{
  			my_delayus((uint16_t)(*(pin+total+1)));
  			total += 2;
  			adr_ofs += adr_step;        //count no prog empty data(0xFFFF) no need prog
  			break;
  		}
  		
  	else if(II == 0)	      //data
  		{
  			data = *(uint32_t *)(pin+total+1);
        gAddr = addr + adr_ofs;
        total += 5;
      }
      
    else if (II&0x80)         // I cmd 
      {
      	data = flshcmd[II&0xF];
      	if(flshadr[II&0xF])
      	    gAddr = addr_bk + flshadr[II&0xF];     //amd, offset from base
      	  else
      	  	gAddr = addr + adr_ofs;        //intel
      	total++;
      }
    else
    	continue;

    TAP_DMA_WR(&data);	
	 }
	 

 } // outer loop
  
  return total;

}




//*************************************************************/

