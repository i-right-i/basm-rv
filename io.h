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
#include<stdlib.h>
#include<stdint.h>


FILE* input_file_ptr;
FILE* output_file_ptr;


int init_files(char *input_file,char *output_file)
{


	input_file_ptr = fopen(input_file,"rb");
	if(input_file_ptr== NULL)
	{
		exit(-1);
		//return 0;
	}
	output_file_ptr = fopen(output_file,"wb+");
	if(output_file_ptr == NULL)
	{
		exit(-1);
		//return 0;
	}
	return 1;	
}

void reopen_for_update(char *output_file)
{
	fclose(output_file_ptr);
	output_file_ptr = fopen(output_file,"rb+");
	if(output_file_ptr == NULL)
	{
		exit(-1);
		//return 0;
	}
	
}

int set_code_pos(int pos)
{
	if(fseek(output_file_ptr,pos,SEEK_SET))
	{
		exit(-1);
	}
	return 0;
}
	
int put_code(uint32_t op_code)
{
	if( fwrite(&op_code,sizeof(uint32_t),1,output_file_ptr )!=1 )
	{
		// error
		exit(-1);
	}
	return 1;
}

int get_char(uint8_t* source_char)
{
	if( fread(source_char,sizeof(uint8_t),1,input_file_ptr)!=1 )
	{
		if(feof(input_file_ptr)!=0)
		{
			clearerr(input_file_ptr);
			return 0;	
		}
		// print error msg
		// exit
		printf("\n ERROR: Unexpected file error? \n");
		exit(-1);
	}
	return 1;
}

void close_files()
{
	fclose(input_file_ptr);
	fclose(output_file_ptr);
}

void print_error(char *error,int source_code_number)
{
	printf(error);
	if(source_code_number>=0)
	{
		printf(" - on line %i \n",source_code_number+1);
	}
}

void print_char(uint8_t x)
{
	char p_buff[2];
	p_buff[0]=x;
	p_buff[1]=0;
	printf(p_buff);
}

void print_buffer(uint8_t*buffer,int size)
{
	for(int i=0;i<size;i++)
	{
		print_char(buffer[i]);	
	}
}
