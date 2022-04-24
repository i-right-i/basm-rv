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

#define TYPE_R	1
#define TYPE_I	2
#define TYPE_B	3
#define TYPE_S	4
#define TYPE_J	5
#define TYPE_U	6
#define REG_INT 	120  // 'x'
//#define REG_FLOAT 	102  // 'f'
//#define REG_DOUBLE 	100  // 'd'

#define NUMBER_OF_OPS 72

typedef struct Known_Parms
{
	uint8_t		op_type;
	uint8_t		pad[3];// May remove this padding later.
	int32_t		p_known[3];
}Known_Prams;

typedef struct Op_Name
{
	uint8_t		length;
	uint8_t 	name[15];
}Op_Name;

typedef struct Op_Saves
{
	uint8_t		op_id;
	uint8_t		rd;
	uint8_t		rs1;
	uint8_t		rs2;
	uint32_t	label_pos;
	uint32_t	op_pos;
}Op_Saves;

//			
//			fun7			fun3   OP7
// R-type	0000000 rs2 rs1 000 rd 0110011 					ADD 	rd, rs1, rs2
// known[0] == p0 , known[1] == p2 , known[2] == p5 
// rd == p1 , rs1 == p3 , rs2 == p4

// I-type	imm[11:0] rs1 000 rd 0010011 					ADDI 	rd, rs1, imm
// known[0] == p0 , known[1] == p2
// rd == p1 , rs1 == p3 , imm == p4
 

// B-type	imm[12|10:5] rs2 rs1 000 imm[4:1|11] 1100011 	BEQ		rs1, rs2, offset
// known[0] == p0 , known[1] == p2 
// rs1 == p3 , rs2 == p4 , offset  == p5


// S-type	imm[11:5] rs2 rs1 000 imm[4:0] 0100011 			SB 		rs2, rs1, offset	# Maybe swap for consistancy
// known[0] == p0 , known[1] == p2 
// rs1 == p3 , rs2 == p4 , offset  == p5


// J-type	imm[20|10:1|11|19:12] rd 1101111 				JAL 	rd, offset20
// known[0] == p0
// rd == p1 , offset == p2

// U-type	imm[31:12] rd 0010111 							AUIPC	rd, imm20
// known[0] == p0
// rd == p1 , imm == p2

void copy_op_name(Op_Name* op_name,const uint8_t *const_name)
{
	int i = 0;
	while(const_name[i]!=0)
	{
		if(i>14)
		{
			// error
			print_error("Error copy_op_name() ",-1);
			exit(-1);
		}
		op_name->name[i] = 	const_name[i];
		i++;
	}
	op_name->length = i;
}
 
void init_instructions(Op_Name* op_name,Known_Prams* parms)
{
	int i = 1;
	// ADD 	rd, rs1, rs2
	// ID:
	i = 1;
	copy_op_name(&op_name[i],"ADD");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	// ADDI rd, rs1, imm12
	// ID:
	i = 2;
	copy_op_name(&op_name[i],"ADDI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;	// op
	parms[i].p_known[1] = 0b000;			// fun3
	// ADDIW rd, rs1, imm12
	// ID:
	i = 3;
	copy_op_name(&op_name[i],"ADDIW");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0011011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	//ADDW rd, rs1, rs2
	// ID:
	i = 4;
	copy_op_name(&op_name[i],"ADDW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//AND rd, rs1, rs2
	// ID:
	i = 5;
	copy_op_name(&op_name[i],"AND");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b111;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//ANDI rd, rs1, imm12
	// ID:
	i = 6;
	copy_op_name(&op_name[i],"ANDI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b111;			// fun3
	//AUIPC rd, imm20
	// ID:
	i = 7;
	copy_op_name(&op_name[i],"AUIPC");
	parms[i].op_type = 	TYPE_U;
	parms[i].p_known[0] = 0b0010111;		// op
	//BEQ rs1, rs2, offset7
	// ID:
	i = 8;
	copy_op_name(&op_name[i],"BEQ");
	parms[i].op_type = 	TYPE_B;
	parms[i].p_known[0] = 0b1100011;		// op
	parms[i].p_known[1] = 0b000;			// fun3	
	//BGE rs1, rs2, offset7
	// ID:
	i = 9;
	copy_op_name(&op_name[i],"BGE");
	parms[i].op_type = 	TYPE_B;
	parms[i].p_known[0] = 0b1100011;		// op
	parms[i].p_known[1] = 0b101;			// fun3	
	//BGEU rs1, rs2, offset7
	// ID:
	i = 10;
	copy_op_name(&op_name[i],"BGEU");
	parms[i].op_type = 	TYPE_B;
	parms[i].p_known[0] = 0b1100011;		// op
	parms[i].p_known[1] = 0b111;			// fun3	
	//BLT rs1, rs2, offset7
	// ID:
	i = 11;
	copy_op_name(&op_name[i],"BLT");
	parms[i].op_type = 	TYPE_B;
	parms[i].p_known[0] = 0b1100011;		// op
	parms[i].p_known[1] = 0b100;			// fun3	
	//BLTU rs1, rs2, offset7
	// ID:
	i = 12;
	copy_op_name(&op_name[i],"BLTU");
	parms[i].op_type = 	TYPE_B;
	parms[i].p_known[0] = 0b1100011;		// op
	parms[i].p_known[1] = 0b110;			// fun3	
	//BNE rs1, rs2, offset7
	// ID:
	i = 13;
	copy_op_name(&op_name[i],"BNE");
	parms[i].op_type = 	TYPE_B;
	parms[i].p_known[0] = 0b1100011;		// op
	parms[i].p_known[1] = 0b001;			// fun3	
	//CSRRC rd, rs1, imm12
	// ID:
	i = 14;
	copy_op_name(&op_name[i],"CSRRC");	// TODO need to work on syntax on these
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1110011;		// op
	parms[i].p_known[1] = 0b011;			// fun3
	//CSRRCI rd, rs1, imm12
	// ID:
	i = 15;
	copy_op_name(&op_name[i],"CSRRCI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1110011;		// op
	parms[i].p_known[1] = 0b111;			// fun3
	//CSRRS rd, rs1, imm12
	// ID:
	i = 16;
	copy_op_name(&op_name[i],"CSRRS");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1110011;		// op
	parms[i].p_known[1] = 0b010;			// fun3
	//CSRRSI rd, rs1, imm12
	// ID:
	i = 17;
	copy_op_name(&op_name[i],"CSRRSI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1110011;		// op
	parms[i].p_known[1] = 0b110;			// fun3
	//CSRRW rd, rs1, imm12
	// ID:
	i = 18;
	copy_op_name(&op_name[i],"CSRRW");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1110011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	//CSRRWI rd, rs1, imm12
	// ID:
	i = 19;
	copy_op_name(&op_name[i],"CSRRWI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1110011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	//DIV rd, rs1, rs2
	// ID:
	i = 20;
	copy_op_name(&op_name[i],"DIV");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b100;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//DIVU rd, rs1, rs2
	// ID:
	i = 21;
	copy_op_name(&op_name[i],"DIVU");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//DIVUW rd, rs1, rs2
	// ID:
	i = 22;
	copy_op_name(&op_name[i],"DIVUW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//DIVW rd, rs1, rs2
	// ID:
	i = 23;
	copy_op_name(&op_name[i],"DIVW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b100;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7

	// ---- Special
	//EBREAK
	// ID:
	i = 24;
	
	//ECALL
	// ID:
	i = 25;
	
	//FENCE
	// ID:
	i = 26;

	//FENCE.I
	// ID:
	i = 27;
	
	// ---- Special
	
	//JAL rd, offset20
	// ID:
	i = 28;
	copy_op_name(&op_name[i],"JAL");
	parms[i].op_type = 	TYPE_J;
	parms[i].p_known[0] = 0b1101111;		// op	
	//JALR rd, rs1, imm12
	// ID:
	i = 29;
	copy_op_name(&op_name[i],"JALR");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b1100111;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	//LB rd, rs1, imm12
	// ID:
	i = 30;
	copy_op_name(&op_name[i],"LB");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	//LBU rd, rs1, imm12
	// ID:
	i = 31;
	copy_op_name(&op_name[i],"LBU");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b100;			// fun3
	//LD rd, rs1, imm12
	// ID:
	i = 32;
	copy_op_name(&op_name[i],"LD");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b011;			// fun3
	//LH rd, rs1, imm12
	// ID:
	i = 33;
	copy_op_name(&op_name[i],"LH");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	//LHU rd, rs1, imm12
	// ID:
	i = 34;
	copy_op_name(&op_name[i],"LHU");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	//LUI rd, imm20
	// ID:
	i = 35;
	copy_op_name(&op_name[i],"LUI");
	parms[i].op_type = 	TYPE_U;
	parms[i].p_known[0] = 0b0110111;		// op
	//LW rd, rs1, imm12
	// ID:
	i = 36;
	copy_op_name(&op_name[i],"LW");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b010;			// fun3
	//LWU rd, rs1, imm12
	// ID:
	i = 37;
	copy_op_name(&op_name[i],"LWU");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0000011;		// op
	parms[i].p_known[1] = 0b110;			// fun3
	//MUL rd, rs1, rs2
	// ID:
	i = 38;
	copy_op_name(&op_name[i],"MUL");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//MULH rd, rs1, rs2
	// ID:
	i = 39;
	copy_op_name(&op_name[i],"MULH");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//MULHSU rd, rs1, rs2
	// ID:
	i = 40;
	copy_op_name(&op_name[i],"MULHSU");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;    	// op
	parms[i].p_known[1] = 0b010;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//MULHU rd, rs1, rs2
	// ID:
	i = 41;
	copy_op_name(&op_name[i],"MULHU");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b011;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//MULW rd, rs1, rs2
	// ID:
	i = 42;
	copy_op_name(&op_name[i],"MULW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//OR rd, rs1, rs2
	// ID:
	i = 43;
	copy_op_name(&op_name[i],"OR");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b110;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//ORI rd, rs1, imm12
	// ID:
	i = 44;
	copy_op_name(&op_name[i],"ORI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b110;			// fun3
	//REM rd, rs1, rs2
	// ID:
	i = 45;
	copy_op_name(&op_name[i],"REM");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b110;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//REMU rd, rs1, rs2
	// ID:
	i = 46;
	copy_op_name(&op_name[i],"REMU");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b111;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//REMUW rd, rs1, rs2
	// ID:
	i = 47;
	copy_op_name(&op_name[i],"REMUW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b111;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//REMW rd, rs1, rs2
	// ID:
	i = 48;
	copy_op_name(&op_name[i],"REMW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b110;			// fun3
	parms[i].p_known[2] = 0b0000001;		// fun7
	//SB rs2, rs1, offset7
	// ID:
	i = 49;
	copy_op_name(&op_name[i],"SB");
	parms[i].op_type = 	TYPE_S;
	parms[i].p_known[0] = 0b0100011;		// op
	parms[i].p_known[1] = 0b000;			// fun3		
	//SD rs2, rs1, offset7
	// ID:
	i = 50;
	copy_op_name(&op_name[i],"SD");
	parms[i].op_type = 	TYPE_S;
	parms[i].p_known[0] = 0b0100011;		// op
	parms[i].p_known[1] = 0b011;			// fun3		
	//SH rs2, rs1, offset7
	// ID:
	i = 51;
	copy_op_name(&op_name[i],"SH");
	parms[i].op_type = 	TYPE_S;
	parms[i].p_known[0] = 0b0100011;		// op
	parms[i].p_known[1] = 0b001;			// fun3		
	//SLL rd, rs1, rs2
	// ID:
	i = 52;
	copy_op_name(&op_name[i],"SLL");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//SLLI rd, rs1, imm12
	// ID:
	i = 53;
	copy_op_name(&op_name[i],"SLLI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	//SLLIW rd, rs1, imm12
	// ID:
	i = 54;
	copy_op_name(&op_name[i],"SLLIW");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0011011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	//SLLW rd, rs1, rs2
	// ID:
	i = 55;
	copy_op_name(&op_name[i],"SLLW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b001;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//SLT rd, rs1, rs2
	// ID:
	i = 56;
	copy_op_name(&op_name[i],"SLT");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b010;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//SLTI rd, rs1, imm12
	// ID:
	i = 57;
	copy_op_name(&op_name[i],"SLTI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b010;			// fun3
	//SLTIU rd, rs1, imm12
	// ID:
	i = 58;
	copy_op_name(&op_name[i],"SLTIU");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b011;			// fun3
	//SLTU rd, rs1, rs2
	// ID:
	i = 59;
	copy_op_name(&op_name[i],"SLTU");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b011;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//SRA rd, rs1, rs2
	// ID:
	i = 60;
	copy_op_name(&op_name[i],"SRA");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	parms[i].p_known[2] = 0b0100000;		// fun7
	//SRAI rd, rs1, imm12
	// ID:
	i = 61;
	copy_op_name(&op_name[i],"SRAI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	//SRAIW rd, rs1, imm12
	// ID:
	i = 62;
	copy_op_name(&op_name[i],"SRAIW");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0011011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	//SRAW rd, rs1, rs2
	// ID:
	i = 63;
	copy_op_name(&op_name[i],"SRAW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	parms[i].p_known[2] = 0b0100000;		// fun7
	//SRL rd, rs1, rs2
	// ID:
	i = 64;
	copy_op_name(&op_name[i],"SRL");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//SRLI rd, rs1, imm12
	// ID:
	i = 65;
	copy_op_name(&op_name[i],"SRLI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	//SRLIW rd, rs1, imm12
	// ID:
	i = 66;
	copy_op_name(&op_name[i],"SRLIW");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0011011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	//SRLW rd, rs1, rs2
	// ID:
	i = 67;
	copy_op_name(&op_name[i],"SRLW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b101;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//SUB rd, rs1, rs2
	// ID:
	i = 68;
	copy_op_name(&op_name[i],"SUB");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	parms[i].p_known[2] = 0b0100000;		// fun7
	//SUBW rd, rs1, rs2
	// ID:
	i = 69;
	copy_op_name(&op_name[i],"SUBW");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0111011;		// op
	parms[i].p_known[1] = 0b000;			// fun3
	parms[i].p_known[2] = 0b0100000;		// fun7
	//SW rs2, rs1, offset7
	// ID:
	i = 70;
	copy_op_name(&op_name[i],"SW");
	parms[i].op_type = 	TYPE_S;
	parms[i].p_known[0] = 0b0100011;		// op
	parms[i].p_known[1] = 0b010;			// fun3		
	//XOR rd, rs1, rs2
	// ID:
	i = 71;
	copy_op_name(&op_name[i],"XOR");
	parms[i].op_type = 	TYPE_R;
	parms[i].p_known[0] = 0b0110011;		// op
	parms[i].p_known[1] = 0b100;			// fun3
	parms[i].p_known[2] = 0b0000000;		// fun7
	//XORI rd, rs1, imm12	
	// ID:
	i = 72;
	copy_op_name(&op_name[i],"XORI");
	parms[i].op_type = 	TYPE_I;
	parms[i].p_known[0] = 0b0010011;		// op
	parms[i].p_known[1] = 0b100;			// fun3		
}

#define		LETTER_A	65
#define		LETTER_B	66
#define		LETTER_C	67
#define		LETTER_D	68
#define		LETTER_E	69
#define		LETTER_F	70
#define		LETTER_J	74
#define		LETTER_L	76
#define		LETTER_M	77
#define		LETTER_O	79
#define		LETTER_R	82
#define		LETTER_S	83
#define		LETTER_X	88
//#define		LETTER_
//#define		LETTER_
//#define		LETTER_
//#define		LETTER_
//#define		LETTER_
