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

#include<stdio.h>
#include<stdlib.h>//
#include<stdint.h>

#include"encoder.h"
#include"io.h"
#include"op_types.h"


#define	TRUE	1
#define FALSE	0

#define OP_CODE_SIZE		4
#define MAX_TOKEN_SIZE 		32
#define MAX_HEX_LENGTH		8
//#define MAX_LINE_LENGTH		128
#define MAX_FULL_LABELS		2600
#define LABEL_SLOT			MAX_TOKEN_SIZE + 5 // the 5 is, one byte header + 4 bytes code position
#define LABEL_BUFFER_MAX	LABEL_SLOT*MAX_FULL_LABELS


#define LINE_END 			10
#define TAB					9   // HT
#define SPACE				32
#define PERIOD				46  // .
#define COMMA				44	// ,
#define MINUS				45	// -
#define COMMENT				35  // #
#define COLON				58  // :
#define AT_SIGN				64  // @
#define DOLLAR_SIGN			36  // $ 
#define UNDER_SCORE			95  // _
#define L_SQUARE_BRACKET	91  // [
#define R_SQUARE_BRACKET	93  // ]
#define GREATER_THAN		62	// >

#define END_OF_FILE			0
#define RECV_CHAR			1


#define FILE_PTR			FILE*
#define TOKEN				uint32_t
#define DATA 				uint32_t

#define CHAR				uint8_t
#define CHAR_PTR			uint8_t*

// Globals
CHAR 		label_buffer[LABEL_BUFFER_MAX];
uint32_t	label_buffer_position; // Todo - Change this to a local var.

CHAR 		const_buffer[LABEL_BUFFER_MAX];
uint32_t	const_buffer_position;

Op_Saves	op_saves[LABEL_BUFFER_MAX * 2];
uint32_t	op_saves_pos = 0;

Op_Name 	op_name[NUMBER_OF_OPS];	// defined in op_types.h
Known_Prams	op_const[NUMBER_OF_OPS];

CHAR 		tmp_token_buffer[MAX_TOKEN_SIZE];
int32_t 	tmp_token_buffer_length;
CHAR 		new_char = SPACE;
uint32_t	source_line_number =0 ;
uint32_t	output_code_position = 0;
//uint32_t	in_white_space;
uint32_t	reached_end_of_file = FALSE;

//
void next_char();			 // %50 - done
void clear_white_space();	 // %100 - done
void skip_line();			 // %100 - done

int compare_buffer(CHAR_PTR a,CHAR_PTR b,int size);			 
void copy_buffer(void *src,void *dst,int size);
//
int check_white_space(CHAR check);
int check_numbers(CHAR check);
int check_upper_case(CHAR check);
int check_lower_case(CHAR check);
int check_hex(CHAR check);
int check_AF(CHAR check);
int check_af(CHAR check);
//
uint32_t check_12bit(int32_t num)
{
	if( (num>SIGNED_12_BIT_MAX) || (num < SIGNED_12_BIT_MIN) )
	{
		print_error("Error Offset out of scope",source_line_number);
	}
	return num;	
}
uint32_t check_20bit(int32_t num)
{
	if( (num>SIGNED_20_BIT_MAX) || (num < SIGNED_20_BIT_MIN) )
	{
		print_error("Error Offset out of scope",source_line_number);
	}
	return num;	
}


void start_parser();
void parser_start_new_line();

void parse_Label();			 // %90 - done -needs error code
void parse_Const();			 // %0
void parse_Data();			 // %0
void parse_OP();			 // %0
	void parse_type_r(uint8_t id);// rd, rs1 ,rs2
	void parse_type_i(uint8_t id);
	void parse_type_b(uint8_t id);
	void parse_type_s(uint8_t id);
	void parse_type_j(uint8_t id);
	void parse_type_u(uint8_t id);

uint8_t get_reg();
void load_name_to_tmp();	// %100
void load_op_name_to_tmp(); // %100
void load_hex_to_tmp();
void find_end_of_line();	// %100
void add_const_name();
void add_const_hex();
int32_t convert_txt_to_hex();
void binary_write_data(uint32_t data);
void add_label();			 // %100
int32_t have_label(int32_t *l_number);
void get_label();			 // %0
int32_t get_const();

uint8_t compare_ops(int a,int b);
uint8_t search_op();

void search_label_buffer();	 // %0 

void save_op(); 			 // %0

void file_finish(); // %0

void parser_start_new_line()
{
	clear_white_space();
	switch(new_char)
	{
		case LINE_END:
			source_line_number++;
			break;
			
		case COMMENT:
			skip_line(); // skip the rest of char, return when reach LINE_END
			source_line_number++;
			break;

		case COLON:
			parse_Label();
			source_line_number++;
			break;

		case AT_SIGN:
			parse_Const();
			source_line_number++;
			break;

		case DOLLAR_SIGN:
			parse_Data();
			source_line_number++;
			break;
		
		case 0:
			//file_finish();
			//printf("FILE END");
			break;
		

		default:
			parse_OP();
			source_line_number++;
			break;
			
	}
}

void clear_white_space()
{
	 
	next_char();

	while( check_white_space(new_char) )
	{
		next_char();
	}

}

void skip_line()
{

	next_char();

	while(new_char!=LINE_END)
	{
		if(reached_end_of_file)
		{
			return;
		}
		next_char();
		//printf(".");
	}
	
}

void next_char()
{

	if(get_char(&new_char) == FALSE)
	{
		reached_end_of_file = TRUE;
		new_char = 0;
	}
	return;
}

void parse_Data()			 // %0
{
	// current new_char is '$' ,this move to next none white space
	clear_white_space(); 
	if(new_char == L_SQUARE_BRACKET) // hex number
	{
		// load hex number
		clear_white_space();
		load_hex_to_tmp();// 

		if(new_char != R_SQUARE_BRACKET) // check for closing bracket ] 
		{
			// error out "Const syntax incorrect on line" source_line_number
			print_error("Const Syntax Incorrect",source_line_number);
			exit(-1);
		}

		next_char();
		find_end_of_line();

		int32_t number = convert_txt_to_hex();
		// write number	
		binary_write_data(number);
	}
	else if(new_char == AT_SIGN ) // get const
	{
		// get const number
		clear_white_space();
		load_name_to_tmp();
		int32_t number = get_const();

		//next_char();
		find_end_of_line();
		// write number
		binary_write_data(number);
	}
	else // error
	{
		print_error("Const Syntax Incorrect",source_line_number);
		exit(-1);	
	}
	return;	
}

void parse_Label()
{
	
	//printf("parse_Label\n"); // Remove

	clear_white_space();
	load_name_to_tmp();
	
	if(new_char != COLON) // check closing Colon 
	{
		// error out "Label syntax incorrect on line" source_line_number
		print_error(" Label Syntax Incorrect",source_line_number);
		exit(-1);

	}

	add_label();
	// DEBUG REMOVE LATER -----------------
	//printf("\n");
	//print_buffer(tmp_token_buffer,tmp_token_buffer_length);
	//printf(" at buffer pointer %i",label_buffer_position);
	//printf("\n");
	
	

	next_char();	
	find_end_of_line(); // ignore comments and find end of line.
	return;
}


void parse_Const()
{
	
	//printf("parse_Const\n"); // Remove

	clear_white_space();
	load_name_to_tmp();

	add_const_name();// 

	if(new_char != L_SQUARE_BRACKET) // check for opening bracket [
	{
		// error out "Const syntax incorrect on line" source_line_number
		print_error("Const Syntax Incorrect",source_line_number);
		exit(-1);

	}
	clear_white_space();
	load_hex_to_tmp();// 

	if(new_char != R_SQUARE_BRACKET) // check for closing bracket ] 
	{
		// error out "Const syntax incorrect on line" source_line_number
		print_error("Const Syntax Incorrect",source_line_number);
		exit(-1);

	}

	

	add_const_hex();

	next_char();
	find_end_of_line();
	return;

}

void parse_OP()
{
	// remove later
	//print_error("OP is not written yet\n",-1);
	//exit(-1);

	//start
	//printf("parse_OP\n"); // Remove


	load_op_name_to_tmp();

	uint8_t op_id = search_op();

	// Parse OP type
	switch(op_const[op_id].op_type)
	{
		case TYPE_R:
			parse_type_r(op_id); // rd, rs1 ,rs2
			break;
		case TYPE_I:
			parse_type_i(op_id);
			break;
		case TYPE_B:
			parse_type_b(op_id);
			break;
		case TYPE_S:
			parse_type_s(op_id);
			break;
		case TYPE_J:
			parse_type_j(op_id);
			break;
		case TYPE_U:
			parse_type_u(op_id);
			break;		
	}
	
	find_end_of_line();
	
}

uint8_t search_op()
{
//CHAR 		tmp_token_buffer[MAX_TOKEN_SIZE];
//int32_t 	tmp_token_buffer_length;
	int i;
	switch(tmp_token_buffer[0])
	{
		case LETTER_A:
			// 1-7
			return compare_ops(1,7);
		case LETTER_B:
			// 8-13		
			return compare_ops(8,13);
		case LETTER_C:
			// 14-19 
			return compare_ops(14,19);
		case LETTER_D:
			// 20-23
			return compare_ops(20,23);
		case LETTER_E:
			// 24-25
			return compare_ops(24,25);
		case LETTER_F:
			// 26-27
			return compare_ops(26,27);
		case LETTER_J:
			// 28-29
			return compare_ops(28,29);
		case LETTER_L:
			// 30-37
			return compare_ops(30,37);
		case LETTER_M:
			// 38-42
			return compare_ops(38,42);
		case LETTER_O:
			// 43-44
			return compare_ops(43,44);
		case LETTER_R:
			//45-48
			return compare_ops(45,48);
		case LETTER_S:
			// 49-70
			return compare_ops(49,70);
		case LETTER_X:
			// 71-72
			return compare_ops(71,72);
		default:
			// error
			print_error("Error OP Name Error ",source_line_number);
			exit(-1);
			
	}
	
}

uint8_t compare_ops(int a,int b) // TODO add global OP Name struct
{
	for(int i = a ; i<=b ;i++)
	{
		if(op_name[i].length == tmp_token_buffer_length)
		{
			if( compare_buffer(op_name[i].name,tmp_token_buffer,tmp_token_buffer_length) )
			{
				return i;
			}
		}
	}
	// error
	print_error("Error OP not Found ",source_line_number);
	exit(-1);
}

// ---------------- Labels Management -------------------------
void add_flagged_label()  // this function is for labels that have not been reached yet
{
	label_buffer_position = 0;
	while(TRUE)
	{
		
		// check if search reached last entry
		if(label_buffer[label_buffer_position] == 0) 
		{
			// not enough memory left  in buffer
			if( (label_buffer_position + tmp_token_buffer_length + 5)> LABEL_BUFFER_MAX )
			{
				//error out
				print_error("Reached Max ammount of Labels ",source_line_number);
				exit(-1);
			}
			// add label, start by anding label size to the header byte
			label_buffer[label_buffer_position] = tmp_token_buffer_length ;
			// store flag code in label address
			uint32_t tmp = 0xffffffff;
			copy_buffer(&tmp,&label_buffer[label_buffer_position+1],OP_CODE_SIZE);
			// store label 
			copy_buffer(tmp_token_buffer,&label_buffer[label_buffer_position+5],tmp_token_buffer_length);

			// zero length , clear
			tmp_token_buffer_length = 0	;		
			return;
   
		}
		// next_label
		label_buffer_position = label_buffer_position + label_buffer[label_buffer_position] + 5;
	}
}

int32_t have_label(int32_t *l_number)
{
	//
	uint32_t tmp;
	label_buffer_position = 0;
	while(label_buffer[label_buffer_position]!=0)
	{
		// compare const name length
		if(tmp_token_buffer_length==label_buffer[label_buffer_position])
		{
			// compare if const names are equal
			if(compare_buffer(&label_buffer[label_buffer_position+5],tmp_token_buffer,tmp_token_buffer_length))
			{
				// copy out number
				copy_buffer(&label_buffer[label_buffer_position+1],&tmp,OP_CODE_SIZE);
				if(tmp == 0xffffffff )
				{
					// get location to be filled by label
					*l_number = label_buffer_position+1;
					return FALSE;
				}
				else
				{
					*l_number = tmp - (output_code_position);
					return TRUE;
				}
				
			}
		}
		// next_label
		label_buffer_position = label_buffer_position + label_buffer[label_buffer_position] + 5;	
		
	}
	// Add flagged label
	if( (label_buffer_position + tmp_token_buffer_length + 5)> LABEL_BUFFER_MAX ) // size check
	{
		//error out
		print_error("Reached Max ammount of Labels ",source_line_number);
		exit(-1);
	}
	// add label, start by anding label size to the header byte
	label_buffer[label_buffer_position] = tmp_token_buffer_length ;

	// store flag
	tmp = 0xffffffff;
	copy_buffer(&tmp,&label_buffer[label_buffer_position+1],OP_CODE_SIZE);
	// store label 
	copy_buffer(tmp_token_buffer,&label_buffer[label_buffer_position+5],tmp_token_buffer_length);

	// zero length , clear
	*l_number = (uint32_t) (label_buffer_position+1);
	tmp_token_buffer_length = 0	;
	return FALSE;			
	
}

void add_label()
{
	// 	0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28
	//  4  .  .  .  .  .  .  .  .  4  .  .  .  .  .  .  .  .  4  .  .  .  .  .  .  .  .
	// output_code_position
	label_buffer_position = 0;
	while(TRUE)
	{
		//printf("add_label\n");
		// check if search reached last entry
		if(label_buffer[label_buffer_position] == 0) 
		{
			// not enough memory left  in buffer
			if( (label_buffer_position + tmp_token_buffer_length + 5)> LABEL_BUFFER_MAX )
			{
				//error out
				print_error("Reached Max ammount of Labels ",source_line_number);
				exit(-1);
			}
			// add label, start by anding label size to the header byte
			label_buffer[label_buffer_position] = tmp_token_buffer_length ;
			//printf("-- buffer_pointer[%i]=%i --",label_buffer_position,tmp_token_buffer_length);
			// store code position to go with label
			copy_buffer(&output_code_position,&label_buffer[label_buffer_position+1],OP_CODE_SIZE);
			// store label 
			copy_buffer(tmp_token_buffer,&label_buffer[label_buffer_position+5],tmp_token_buffer_length);

			// zero length , clear
			//tmp_token_buffer_length = 0	;		
			return;
   
		}
		else
		{
			// check if new label equal size of stored label
			if(tmp_token_buffer_length==label_buffer[label_buffer_position])
			{
				// compare if new label = old label
				if(compare_buffer(&label_buffer[label_buffer_position+5],tmp_token_buffer,tmp_token_buffer_length))
				{

					// check if old label position is flagged
					uint32_t tmp; // get value of label
					copy_buffer(&label_buffer[label_buffer_position+1],&tmp,OP_CODE_SIZE);
					if(tmp != 0xffffffff )// if not falgged error out
					{
						//error out
						print_error("Duplicate Label used ",source_line_number);
						exit(-1);
					}
					else
					{
						// replace flag with code position to go with label
						copy_buffer(&output_code_position,&label_buffer[label_buffer_position+1],OP_CODE_SIZE);
						return;
					}
				}
			}
			// next_label
			label_buffer_position = label_buffer_position + label_buffer[label_buffer_position] + 5;	
		}
	}// while
	
	
}

void load_name_to_tmp()
{
	tmp_token_buffer_length = 0;
	//clear_white_space();
	while( check_numbers(new_char) || check_upper_case(new_char) || check_lower_case(new_char) )
	{
		tmp_token_buffer[tmp_token_buffer_length] = new_char;
		tmp_token_buffer_length++; // this work for zero indexed.
		if(tmp_token_buffer_length>MAX_TOKEN_SIZE)
		{
			// error out "Name to long in line" source_line_number
			print_error(" Name is too long",source_line_number);
			exit(-1);
		}
		next_char();
	}
	if(tmp_token_buffer_length==0)
	{
		// error out "Name is blank" source_line_number
		print_error("Error Blank Name",source_line_number);
		exit(-1);		
	}
	if( check_white_space(new_char) )
	{
		clear_white_space();
	}
}

void load_op_name_to_tmp()
{
	tmp_token_buffer_length = 0;
	//clear_white_space();
	while( (new_char==PERIOD) || check_upper_case(new_char) || check_lower_case(new_char) )
	{
		if( check_lower_case(new_char) )
		{
			// convert to Upper case.
			tmp_token_buffer[tmp_token_buffer_length] = new_char - 32 ;
		}
		else
		{
			tmp_token_buffer[tmp_token_buffer_length] = new_char;
		}
		tmp_token_buffer_length++; // this work for zero indexed.
		if(tmp_token_buffer_length>MAX_TOKEN_SIZE)
		{
			// error out "Name to long in line" source_line_number
			print_error("Error name to large",source_line_number);
			exit(-1);
		}
		next_char();
	}
	if(tmp_token_buffer_length==0)
	{
		// error out "Name is blank" source_line_number
		print_error("Blank Name Check file end",source_line_number);
		exit(-1);		
	}
	if( check_white_space(new_char) )
	{
		clear_white_space();
	}
}


void load_hex_to_tmp()
{
	int negative = 0;
	tmp_token_buffer_length = 0;
	//clear_white_space();

	if(new_char == MINUS ) // handle negative number
	{
		tmp_token_buffer[tmp_token_buffer_length] = MINUS;
		tmp_token_buffer_length++; // this work for zero indexed.
		negative = 1;	
		next_char();
	}
	
	while( check_hex(new_char) )
	{
		tmp_token_buffer[tmp_token_buffer_length] = new_char;
		tmp_token_buffer_length++; // this work for zero indexed.
		if(tmp_token_buffer_length> (MAX_HEX_LENGTH+negative) )
		{
			// error out "Name to long in line" source_line_number
			print_error(" Number is too long",source_line_number);
			exit(-1);
		}
		next_char();
	}
	if(tmp_token_buffer_length<(negative+1))// check if there was a number
	{
		print_error(" No number was entered",source_line_number);
		exit(-1);
	}
	if( check_white_space(new_char) )
	{
		clear_white_space();
	}
	
	return;
}

void find_end_of_line()
{
	if(new_char==LINE_END)
	{
		return;
	}
	if( check_white_space(new_char) )
		{
			clear_white_space();
		}
	//clear_white_space();
	if(new_char == COMMENT)
	{
		skip_line(); // skip the rest of char, return when reach LINE_END
		return;
	}
	if(new_char!=LINE_END) // error on some other charector
	{
		// error out
		//printf(" new_char = %i",new_char);
		print_error("Syntax Error expected Comment or end of line.. ",source_line_number);
		exit(-1);

	}
}

void add_const_name()
{

	
	const_buffer_position = 0;
	while(TRUE)
	{
		
		// check if search reached last entry
		if(const_buffer[const_buffer_position] == 0) 
		{
			// not enough memory left  in buffer
			if( (const_buffer_position + tmp_token_buffer_length + 5)> LABEL_BUFFER_MAX )
			{
				//error out
				print_error("Reached Max ammount of Consts ",source_line_number);
				exit(-1);
			}
			// add label, start by adding const name size to the header byte
			const_buffer[const_buffer_position] = tmp_token_buffer_length ;

			
			// store const_name
			copy_buffer(tmp_token_buffer,&const_buffer[const_buffer_position+5],tmp_token_buffer_length);

			// zero length , clear
			tmp_token_buffer_length = 0	;		
			return;
   
		}
		else
		{
			// check if new const equal size of stored label
			if(tmp_token_buffer_length==const_buffer[const_buffer_position])
			{
				// compare if new label = old label
				if(compare_buffer(&const_buffer[const_buffer_position+5],tmp_token_buffer,tmp_token_buffer_length))
				{
					//error out
					print_error("Duplicate Const used ",source_line_number);
					exit(-1);
				}
			}
			// next_label
			const_buffer_position = const_buffer_position + const_buffer[const_buffer_position] + 5;	
		}
	}
}

int32_t get_const()
{
	//
	int32_t number;
	const_buffer_position = 0;
	while(const_buffer[const_buffer_position]!=0)
	{
		// compare const name length
		if(tmp_token_buffer_length==const_buffer[const_buffer_position])
		{
			// compare if const names are equal
			if(compare_buffer(&const_buffer[const_buffer_position+5],tmp_token_buffer,tmp_token_buffer_length))
			{
				// TODO  copy out number
				copy_buffer(&const_buffer[const_buffer_position+1],&number,OP_CODE_SIZE);
				// return
				return number;
			}
		}
		// next_label
		const_buffer_position = const_buffer_position + const_buffer[const_buffer_position] + 5;	
		
	}
	// error out
	print_error("Const need to be defined before used ",source_line_number);
	exit(-1);
}

void add_const_hex()
{

	int32_t number = convert_txt_to_hex();
	int test = number;
	//printf("added const %i \n ",test);	
	// store hex number 
	copy_buffer(&number,&const_buffer[const_buffer_position+1],OP_CODE_SIZE);
	
}

int32_t convert_txt_to_hex()
{
	int32_t single_digit = 0;
	int32_t hex_place = 1;
	int32_t number = 0;
	// iterate hex string from right to left, (tmp_token_buffer_length-1) for zero index
	for(int i = (tmp_token_buffer_length-1);i>=0;i-- )
	{

		if( check_numbers(tmp_token_buffer[i]) )
		{
			single_digit = tmp_token_buffer[i] - 48;	// zero is 48 so minus 48.
		}
		if(  check_AF(tmp_token_buffer[i]) )
		{
			single_digit = tmp_token_buffer[i] - 55; // upper A is 65 so minus 55 leave us with digit 10
		}
		if(  check_af(tmp_token_buffer[i]) )
		{
			single_digit = tmp_token_buffer[i] - 87; // lower is 97 so minus 87 leave us with digit 10
		}

		if(tmp_token_buffer[i]==MINUS)
		{
			number = number * (-1); // flip sign.
			// function should exit after minus due to early check make sure minus is left most value	
		}
		else
		{
			number = number + (single_digit*hex_place);
			hex_place = hex_place * 16; // increase hex place by one hex power
		}
		
	}
	return number;
}

/*
void next_label()		 // %0 
{
	
	label_buffer_position = label_buffer_position + label_buffer[label_buffer_position] + 5;
		
}
*/

void get_label();			 // %0
void save_op();


//


// ----------------- Range Checks -------------------

int check_white_space(CHAR check)
{
	return ( (check == SPACE)||(check == TAB) );
}

int check_numbers(CHAR check) // numbers = 48 - 57
{
	if( (check > 47) && (check < 58) )
	{
		return TRUE;
	}
	return FALSE;
}

int check_upper_case(CHAR check) // uppers = 65 - 90
{
	if( (check > 64) && (check < 91) )
	{
		return TRUE;
	}
	return FALSE;
}

int check_lower_case(CHAR check) // lowers = 97 - 122
{
	if( (check > 96) && (check < 123) )
	{
		return TRUE;
	}
	return FALSE;
}

int check_hex(CHAR check)
{
	if(check_numbers(check))  			// 0 - 9
	{
		return TRUE;
	}
	if( (check > 96) && (check < 103) )	// a - b
	{
		return TRUE;
	}
	if( (check > 64) && (check < 71) )	// A - B
	{
		return TRUE;
	}
	return FALSE;
}

int check_AF(CHAR check) // uppers = 65 - 70
{
	if( (check > 64) && (check < 71) )
	{
		return TRUE;
	}
	return FALSE;
}

int check_af(CHAR check) // lowers = 97 - 102
{
	if( (check > 96) && (check < 103) )
	{
		return TRUE;
	}
	return FALSE;
}

// ---------------------------------------------------

int compare_buffer(CHAR_PTR a,CHAR_PTR b,int size)
{
	for(int i=0;i < size; i++)
	{
		if(a[i]==b[i])
		{
			continue;
		}
		else
		{
			return FALSE;
		}
	}
	return TRUE;	
}		 
void copy_buffer(void *src,void *dst,int size)
{
	for(int i = 0; i<size ;i++)
	{
		((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
	}
}

void zero_buffer(void *buffer,int size)
{
	for(int i =0;i<size;i++)
	{
		((uint8_t*)buffer)[i] = 0;
	}
	return;
}


// -------- main() --------------------------------------

int main( int argc, char *argv[] ) 
{

	if(argc != 3 )
	{
		// print help msg
		print_error("\n basm_riscv [source_file_name] [binary_file_name]",-1);
		exit(-1);
	}

	char *input_file = argv[1];
	char *output_file = argv[2];
	
	
	if( !init_files(input_file,output_file) )
	{
		// May move this error to init_files to give info on which file errored. 
		print_error("\n Error opening file",-1);
		exit(-1);	
	}
	init_instructions(op_name,op_const);
	start_parser();
	reopen_for_update(output_file);
	file_finish();
	close_files();
	print_error("\n Assembled with no Errors",-1);
	return 0;	
		
}

void start_parser()
{
	zero_buffer(label_buffer,LABEL_BUFFER_MAX);
	while(!reached_end_of_file)
	{
		parser_start_new_line();	
	}
	// load label values into opcodes
}

/*
void parse_Const()			 // %0
{
	
}
*/
int32_t get_label_offset_check(uint32_t num)
{
	uint32_t label_addr;
	copy_buffer(&label_buffer[op_saves[num].label_pos], &label_addr,OP_CODE_SIZE);
	if( label_addr == 0xffffffff)
	{
		// error
		print_error("Error OP code used with Missing Label ",op_saves[num].op_pos);
		exit(-1);
	}
	return  label_addr - op_saves[num].op_pos;	
}


void file_finish()
{

	int32_t imm_cal;
	for(uint32_t i=0;i<op_saves_pos;i++)
	{
		// Get the type of op to encode.
		switch(op_const[op_saves[i].op_id].op_type )
		{
			case TYPE_R:
				// error
				exit(-1);
			
			case TYPE_I:
				// gets and set offset for label
				imm_cal = get_label_offset_check(i);
				// set position in file
				set_code_pos(op_saves[i].op_pos);
				// write
				binary_write_data( I_Type(	check_12bit(imm_cal) ,
											op_saves[i].rs1	,
											op_const[op_saves[i].op_id].p_known[1] ,
											op_saves[i].rd	,
											op_const[op_saves[i].op_id].p_known[0]) );
				break;
				
			case TYPE_B:
				// gets and set offset for label
				imm_cal = get_label_offset_check(i);
				// set position in file
				set_code_pos(op_saves[i].op_pos);
				// write
				binary_write_data( B_Type(	check_12bit(imm_cal) ,
											op_saves[i].rs2	,
											op_saves[i].rs1	,
											op_const[op_saves[i].op_id].p_known[1] ,
											op_const[op_saves[i].op_id].p_known[0]) );
				break;
				
			case TYPE_S:
				// gets and set offset for label
				imm_cal = get_label_offset_check(i);
				// set position in file
				set_code_pos(op_saves[i].op_pos);
				// write
				binary_write_data( S_Type(	check_12bit(imm_cal) ,
											op_saves[i].rs1	,
											op_saves[i].rs2	,
											op_const[op_saves[i].op_id].p_known[1] ,
											op_const[op_saves[i].op_id].p_known[0]) );
				break;			
			case TYPE_J:
				// gets and set offset for label
				imm_cal = get_label_offset_check(i);
				// set position in file
				set_code_pos(op_saves[i].op_pos);
				// write
				binary_write_data( J_Type(	check_20bit(imm_cal) ,
											op_saves[i].rd	,
											op_const[op_saves[i].op_id].p_known[0]) );
				break;
			case TYPE_U:
				// gets and set offset for label
				imm_cal = get_label_offset_check(i);
				// set position in file
				set_code_pos(op_saves[i].op_pos);
				// write
				binary_write_data( U_Type(	check_20bit(imm_cal) ,
											op_saves[i].rd	,
											op_const[op_saves[i].op_id].p_known[0]) );
				break;
		}
	}
	
}


void binary_write_data(uint32_t data)// TODO write to file instead of printf
{
	//printf("Store value %08x  at location %08X \n ",data,output_code_position);
	put_code(data);
	output_code_position += 4; // 4 is opcode size
}


void parse_type_r(uint8_t id) // rd, rs1 ,rs2
{
	uint8_t rd;
	uint8_t rs1;
	uint8_t rs2;
	
	rd = get_reg();
	if(new_char != COMMA)
	{
		// error out
		print_error("Error Expected Comma  here ",source_line_number);
		exit(-1);
	}
	//printf("first reg");
	clear_white_space();
	rs1 = get_reg();
	if(new_char != COMMA)
		{
			// error out
			print_error("Error Expected Comma  here ",source_line_number);
			exit(-1);

		}
	clear_white_space();
	rs2 = get_reg();
		
	// finish line
	//next_char();
	find_end_of_line();

	// TODO replace with write function
	//printf(" id=%i, rd=%i, rs1=%i, rs2=%i ",id,rd,rs1,rs2);
	binary_write_data( R_Type(op_const[id].p_known[2], rs2 ,rs1,op_const[id].p_known[1],rd, op_const[id].p_known[0]) );	
	
}



// rd, rs1, imm12
void parse_type_i(uint8_t id)
{
	uint8_t		rd;
	uint8_t 	rs1;
	int32_t 	imm12;

	rd = get_reg();
	if(new_char != COMMA)
	{
		// error out
		print_error("Error Expected Comma  here ",source_line_number);
		exit(-1);
	}
	//printf("first reg");
	clear_white_space();
	rs1 = get_reg();
	if(new_char != COMMA)
		{
			// error out
			print_error("Error Expected Comma  here ",source_line_number);
			exit(-1);

		}	

	clear_white_space();
	if( (check_hex(new_char) ) || (new_char == MINUS) )
	{
		load_hex_to_tmp();	
		imm12 = convert_txt_to_hex();
	}
	else if(new_char == AT_SIGN)
	{
		clear_white_space();
		load_name_to_tmp();
		imm12 = get_const();
			
		//next_char();
		
	}
	else if(new_char == GREATER_THAN)
	{
		clear_white_space();
		load_name_to_tmp();

		if(!have_label(&imm12) )
		{
			// store op
			op_saves[op_saves_pos].op_id 		= id;
			op_saves[op_saves_pos].rd 			= rd;
			op_saves[op_saves_pos].rs1 			= rs1;
			op_saves[op_saves_pos].label_pos 	= imm12;
			op_saves[op_saves_pos].op_pos		= output_code_position;
			op_saves_pos++;
			binary_write_data(0xffffffff);
				
			find_end_of_line();
			return;	
		}
		find_end_of_line();	
		binary_write_data( I_Type(check_12bit(imm12), rs1, op_const[id].p_known[1] ,rd, op_const[id].p_known[0] ) );
		return;		
	}
	else
	{
		// error out
		print_error("Error Bad Sytax ",source_line_number);
		exit(-1);
	}
	find_end_of_line();	
	binary_write_data( I_Type(imm12, rs1, op_const[id].p_known[1] ,rd, op_const[id].p_known[0] ) );
	
}
// rs1, rs2, offset12
void parse_type_b(uint8_t id)
{
	uint8_t		rs1;
	uint8_t 	rs2;
	int32_t 	imm12;

	rs1 = get_reg();
	if(new_char != COMMA)
	{
		// error out
		print_error("Error Expected Comma  here ",source_line_number);
		exit(-1);
	}
	//printf("first reg");
	clear_white_space();
	rs2 = get_reg();
	if(new_char != COMMA)
		{
			// error out
			print_error("Error Expected Comma  here ",source_line_number);
			exit(-1);

		}	

	clear_white_space();
	if( (check_hex(new_char) ) || (new_char == MINUS) )
	{
		load_hex_to_tmp();	
		imm12 = convert_txt_to_hex();
	}
	else if(new_char == AT_SIGN)
	{
		clear_white_space();
		load_name_to_tmp();
		imm12 = get_const();
			
		//next_char();
		
	}
	else if(new_char == GREATER_THAN)
	{
		clear_white_space();
		load_name_to_tmp();

		if(!have_label(&imm12) )
		{
			// store op
			op_saves[op_saves_pos].op_id 		= id;
			op_saves[op_saves_pos].rs1 			= rs1;
			op_saves[op_saves_pos].rs2 			= rs2;
			op_saves[op_saves_pos].label_pos 	= imm12;
			op_saves[op_saves_pos].op_pos		= output_code_position;
			op_saves_pos++;
			binary_write_data(0xffffffff);
				
			find_end_of_line();
			return;	
		}
		find_end_of_line();	
		binary_write_data( B_Type(check_12bit(imm12), rs2, rs1, op_const[id].p_known[1] , op_const[id].p_known[0] ) );
		return;
	}
	else
	{
		// error out
		print_error("Error Bad Sytax ",source_line_number);
		exit(-1);
	}

	find_end_of_line();	
	binary_write_data( B_Type(imm12, rs2, rs1, op_const[id].p_known[1] , op_const[id].p_known[0] ) );
}
// rs2, rs1, offset12
void parse_type_s(uint8_t id)
{
	uint8_t		rs1;
	uint8_t 	rs2;
	int32_t 	imm12;

	rs1 = get_reg();
	if(new_char != COMMA)
	{
		// error out
		print_error("Error Expected Comma  here ",source_line_number);
		exit(-1);
	}
	//printf("first reg");
	clear_white_space();
	rs2 = get_reg();
	if(new_char != COMMA)
		{
			// error out
			print_error("Error Expected Comma  here ",source_line_number);
			exit(-1);

		}	

	clear_white_space();
	if( (check_hex(new_char) ) || (new_char == MINUS) )
	{
		load_hex_to_tmp();	
		imm12 = convert_txt_to_hex();
	}
	else if(new_char == AT_SIGN)
	{
		clear_white_space();
		load_name_to_tmp();
		imm12 = get_const();
			
		//next_char();
		
	}
	else if(new_char == GREATER_THAN)
	{
		clear_white_space();
		load_name_to_tmp();

		if(!have_label(&imm12) )
		{
			// store op
			op_saves[op_saves_pos].op_id 		= id;
			op_saves[op_saves_pos].rs1 			= rs1;
			op_saves[op_saves_pos].rs2 			= rs2;
			op_saves[op_saves_pos].label_pos 	= imm12;
			op_saves[op_saves_pos].op_pos		= output_code_position;
			op_saves_pos++;
			binary_write_data(0xffffffff);
				
			find_end_of_line();
			return;	
		}
		find_end_of_line();
			// rs1 and rs2 are flipped for b_types	
		binary_write_data( S_Type(check_12bit(imm12), rs1, rs2, op_const[id].p_known[1] , op_const[id].p_known[0] ) );
		return;
	}
	else
	{
		// error out
		print_error("Error Bad Sytax ",source_line_number);
		exit(-1);
	}

	find_end_of_line();
	// rs1 and rs2 are flipped for b_types	
	binary_write_data( S_Type(imm12, rs1, rs2, op_const[id].p_known[1] , op_const[id].p_known[0] ) );	
}
// rd, offset20
void parse_type_j(uint8_t id)
{
	uint8_t		rd;
	int32_t 	imm20;

	rd = get_reg();
	if(new_char != COMMA)
	{
		// error out
		print_error("Error Expected Comma  here ",source_line_number);
		exit(-1);
	}
	
	clear_white_space();
	if( (check_hex(new_char) ) || (new_char == MINUS) )
	{
		load_hex_to_tmp();	
		imm20 = convert_txt_to_hex();
	}
	else if(new_char == AT_SIGN)
	{
		clear_white_space();
		load_name_to_tmp();
		imm20 = get_const();
			
		//next_char();
		
	}
	else if(new_char == GREATER_THAN)
	{
		clear_white_space();
		load_name_to_tmp();

		if(!have_label(&imm20) )
		{
			// store op
			op_saves[op_saves_pos].op_id 		= id;
			op_saves[op_saves_pos].rd			= rd;
			op_saves[op_saves_pos].label_pos 	= imm20;
			op_saves[op_saves_pos].op_pos		= output_code_position;
			op_saves_pos++;
			binary_write_data(0xffffffff);		
			find_end_of_line();
			return;	
		}
		find_end_of_line();	
		binary_write_data( J_Type(check_20bit(imm20), rd , op_const[id].p_known[0] ) );	
		return;
		
	}
	else
	{
		// error out
		print_error("Error Bad Sytax ",source_line_number);
		exit(-1);
	}

	find_end_of_line();	
	binary_write_data( J_Type(imm20, rd , op_const[id].p_known[0] ) );	
}
// rd, imm20
void parse_type_u(uint8_t id)
{
	uint8_t		rd;
	int32_t 	imm20;

	rd = get_reg();
	if(new_char != COMMA)
	{
		// error out
		print_error("Error Expected Comma  here ",source_line_number);
		exit(-1);
	}
	
	clear_white_space();
	if( (check_hex(new_char) ) || (new_char == MINUS) )
	{
		load_hex_to_tmp();	
		imm20 = convert_txt_to_hex();
	}
	else if(new_char == AT_SIGN)
	{
		clear_white_space();
		load_name_to_tmp();
		imm20 = get_const();
			
		//next_char();
		
	}
	else if(new_char == GREATER_THAN)
	{
		clear_white_space();
		load_name_to_tmp();

		if(!have_label(&imm20) )
		{
			// store op
			op_saves[op_saves_pos].op_id 		= id;
			op_saves[op_saves_pos].rd			= rd;
			op_saves[op_saves_pos].label_pos 	= imm20;
			op_saves[op_saves_pos].op_pos		= output_code_position;
			op_saves_pos++;
			binary_write_data(0xffffffff);
				
			find_end_of_line();
			return;	
		}
			find_end_of_line();	
			binary_write_data( U_Type(check_20bit(imm20), rd , op_const[id].p_known[0] ) );	
			return;

		
	}
	else
	{
		// error out
		print_error("Error Bad Sytax ",source_line_number);
		exit(-1);
	}

	find_end_of_line();	
	binary_write_data( U_Type(imm20, rd , op_const[id].p_known[0] ) );	
}


uint8_t get_reg()
{
	//printf(" %i ",new_char);
	if( !(check_upper_case(new_char)||check_lower_case(new_char)) )
	{
		// error out
		print_error("Error with Syntax no Letter to Start Reg ",source_line_number);
		exit(-1);
	}

	clear_white_space();
	load_name_to_tmp();

	//printf(" %i %i ",tmp_token_buffer[0],tmp_token_buffer[1]);
	
	if( (tmp_token_buffer_length == 1) && ( check_numbers(tmp_token_buffer[0]) ) )
	{
		return (tmp_token_buffer[0] - 48);		
	}
	else if( (tmp_token_buffer_length == 2) && ( check_numbers(tmp_token_buffer[0]) ) && ( check_numbers(tmp_token_buffer[1]) ) )
	{	
		uint8_t tmp = ( (tmp_token_buffer[0] - 48)*10) + (tmp_token_buffer[1] - 48);
		if(tmp>31)
		{
			//error out
			print_error("Error Reg to Big ",source_line_number);
			exit(-1);
		}
		return tmp;
	}
	else
	{
		// error out
		print_error("Error with Reg Syntax ",source_line_number);
		exit(-1);
	}
	
}
