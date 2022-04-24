/*
    basm_rv - A very simple Risc-V RV64 assembler for building flat binaries, done in a single pass.
    Copyright (C) 2022  Ben Olmstead <myos.sos.os.ben@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ENCODER_H_
#define ENCODER_H_

#include<stdint.h>

#define SIGNED_20_BIT_MAX	1048575
#define SIGNED_12_BIT_MAX	4095
#define SIGNED_20_BIT_MIN	-1048576
#define SIGNED_12_BIT_MIN	-4096


// add bits to clear overflow for shift add
#define KEEP3	0b111
#define KEEP5	0b11111
#define KEEP7	0b1111111
#define KEEP12  0b111111111111
#define KEEP13  0b1111111111111
#define KEEP20	0b11111111111111111111	// Probably not needed due to left shift at the end
#define KEEP21	0b111111111111111111111	// Probably not needed due to left shift at the end
#define ANDCLEAR(SRC,KEEP_BITS) ( (SRC) = (SRC)&(KEEP_BITS)) // 



// J_Type defs
#define JBIT20	0b10000000000000000000
#define JBIT11	0b10000000000
#define JBIT_20_11 (JBIT20 & JBIT11)
#define JAND11	0b11111111111
//

// S_Type defs
#define S_AND5	0b11111
//

// B_Type defs
#define B_AND4	0b1111
#define B_AND6	0b111111
#define BBIT11 	0b10000000000
#define BBIT12 	0b100000000000


// may change these all to uint32_t
#define OP7		uint8_t
#define REG5	uint8_t
#define FUN3	uint8_t
#define FUN7	uint8_t
#define IMM12	uint16_t
#define OFF5	uint8_t
#define OFF7	uint8_t
#define OFF12	uint16_t
#define OFF20	uint32_t
#define IMM20	uint32_t


#define CODE32	uint32_t

CODE32 R_Type(FUN7 p5,REG5 p4,REG5 p3,FUN3 p2,REG5 p1, OP7 p0);
CODE32 I_Type(IMM12 p4, REG5 p3,FUN3 p2,REG5 p1, OP7 p0);
CODE32 B_Type(OFF12 p5,REG5 p4,REG5 p3,FUN3 p2,OP7 p0); 
CODE32 J_Type(OFF20 p2,REG5 p1, OP7 p0);
CODE32 S_Type(IMM12 p5,REG5 p4,REG5 p3,FUN3 p2,OP7 p0); 
CODE32 U_Type(IMM20 p2,REG5 p1, OP7 p0);



CODE32 R_Type(FUN7 p5,REG5 p4,REG5 p3,FUN3 p2,REG5 p1, OP7 p0)
{
	ANDCLEAR(p0,KEEP7);
	ANDCLEAR(p1,KEEP5);
	ANDCLEAR(p2,KEEP3);
	ANDCLEAR(p3,KEEP5);
	ANDCLEAR(p4,KEEP5);
	// ANDCLEAR not needed for last parameter, overflow gets shifted out.

	CODE32 code =0;
	code = ( ((p5)<<25) + ((p4)<<20) + ((p3)<<15) + ((p2)<<12) + ((p1)<<7) + ((p0)<<0) );
	return code;
}

CODE32 I_Type(IMM12 p4, REG5 p3,FUN3 p2,REG5 p1, OP7 p0)
{
	ANDCLEAR(p0,KEEP7);
	ANDCLEAR(p1,KEEP5);
	ANDCLEAR(p2,KEEP3);
	ANDCLEAR(p3,KEEP5);
	// ANDCLEAR not needed for last parameter, overflow gets shifted out.

	CODE32 code =0;
	code = ( ((p4)<<20) + ((p3)<<15) + ((p2)<<12) + ((p1)<<7) + ((p0)<<0) );
	return code;	
}

// p5 and p1 are combined in p5
CODE32 B_Type(OFF12 p5,REG5 p4,REG5 p3,FUN3 p2,OP7 p0)
{
	ANDCLEAR(p0,KEEP7);
	ANDCLEAR(p2,KEEP3);
	ANDCLEAR(p3,KEEP5);
	ANDCLEAR(p4,KEEP5);


	p5 = ((p5)>>1);
	// save bit 12 and 11 

	uint32_t bit11 	= 	(p5 & BBIT11) 	&& 1;
	uint32_t bit12 	= 	(p5 & BBIT12) 	&& 1;
	
	// seperate p5 into two perameters.
	OFF12 p1 = ((p5 & B_AND4)<<1)+ bit11 ;
	p5 =( ((bit12)<<7)  + ( ((p5)>>4 )& B_AND6) );

	
	CODE32 code =0;
	code = ( ((p5)<<25) + ((p4)<<20) + ((p3)<<15) + ((p2)<<12) + ((p1)<<7) + ((p0)<<0) );
	return code;
}

CODE32 J_Type(OFF20 p2,REG5 p1, OP7 p0)
{
	ANDCLEAR(p0,KEEP7);
	ANDCLEAR(p1,KEEP5);

	p2 = ((p2)>>1);
	// swap bits 20 and 11
	uint32_t  bit20 = (p2 & JBIT20) && 1 ;
	uint32_t  bit11  = (p2 & JBIT11) && 1  ;
	if(bit20 != bit11)// if not equal bit flip them.
	{
		p2 = p2 ^ JBIT_20_11;
	}
	// swap lower 11 bits and upper 9 bits
	p2 = ( (p2 & JAND11) << 9) + ( (p2) >> 11); 
	
	
	CODE32 code =0;
	code = (  ((p2)<<12) + ((p1)<<7) + ((p0)<<0) );
	return code;
}

// p5 and p1 are in p5.
CODE32 S_Type(IMM12 p5,REG5 p4,REG5 p3,FUN3 p2,OP7 p0)
{
	ANDCLEAR(p0,KEEP7);
	ANDCLEAR(p2,KEEP3);
	ANDCLEAR(p3,KEEP5);
	ANDCLEAR(p4,KEEP5);


	// seperate p5 into two perameters.
	IMM12 p1 = p5 & S_AND5;
	p5 = (p5)>>5;
	
	CODE32 code =0;
	code = ( ((p5)<<25) + ((p4)<<20) + ((p3)<<15) + ((p2)<<12) + ((p1)<<7) + ((p0)<<0) );
	return code;
}

CODE32 U_Type(IMM20 p2,REG5 p1, OP7 p0)
{
	ANDCLEAR(p0,KEEP7);
	ANDCLEAR(p1,KEEP5);


	CODE32 code =0;
	code = (  ((p2)<<12) + ((p1)<<7) + ((p0)<<0) );
	return code;	
}


#endif
