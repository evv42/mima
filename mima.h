//mima - monochrome image, maybe anything
#ifndef MIMA_H
#define MIMA_H
#ifdef __cplusplus
extern "C" {
#endif

//Returns a decoded image of chan bpp, size *w x *h, from file. Or NULL if it fails.
unsigned char* mima_read(char* filename, int* w, int* h, unsigned char* achan, char wchan);

void mima_write(char* filename, unsigned char* data, unsigned char chan, long unsigned int size, unsigned int width);

#ifdef __cplusplus
}
#endif
#endif //MIMA_H

#ifdef MIMA_IMPLEMENTATION

#include <stdio.h>//fread, fopen, fclose
#include <stdlib.h>//malloc, free
#include <string.h>//memcmp, memset
#include <stdint.h>

//fast variants are not optional in C99
//if using C89, redefine these to a type that can support these at minimum
#define uint40 uint_fast64_t
#define uint18 uint_fast32_t

#define STDOUT 1

static uint18 mima_uint_18(const unsigned char* bytes) {
	uint18 a = bytes[0];
	uint18 b = bytes[1];
	uint18 c = bytes[2];
	return a << 16 | b << 8 | c;
}

static uint40 mima_uint_40(const unsigned char* bytes) {
	uint40 a = bytes[0];
	uint40 b = bytes[1];
	uint40 c = bytes[2];
	uint40 d = bytes[3];
	uint40 e = bytes[4];
	return a << 32 | b << 24 | c << 16 | d << 8 | e;
}

unsigned char* mima_dec(FILE* file, uint18* w, uint40* h, unsigned char* chan){
	//check first if file is a qoi file:
	unsigned char magic[7];
	fread(magic,1,7,file);
	if(memcmp(magic,"MIMA_FF",7))return NULL;//Invalid file type
	
	fread(chan,1,1,file);
	if(chan[0] > 4)return NULL;//Invalid channel number
	unsigned char size_f[5];fread(size_f,1,5,file);
	unsigned char width_f[3];fread(width_f,1,3,file);
	
	//fill return fields and size
	*w = mima_uint_18(width_f);
	uint40 size = mima_uint_40(size_f);
	*h = (size / w[0])/ (chan[0] ? chan[0] : 1);
	
	//allocate buffer
	unsigned char* out = malloc(size);
	if(out == NULL)return NULL;
	
	//decode here
	int ins_val;
	unsigned char ins = 0;
	unsigned char r = 1;
	unsigned char* slide = out;
	unsigned char index[4] = {0x00,0x01,0xEF,0xFF};
	while(r){
		r = fread(&ins,1,1,file);
		switch(ins & 0xC0){
			case 0x40: //DIFF
				ins_val = slide[-1]+((ins & 0x38)>>3)-4;
				slide[0]=ins_val;
				index[(slide[0]%4)-1] = slide[0];
				ins_val = slide[0]+(ins & 0x07)-4;
				slide[1]=ins_val;
				index[(slide[1]%4)-1] = slide[1];
				slide+=2;
				break;
			case 0x00: //VALUE
				ins_val = (ins & 0x3F);
				while(ins_val-- > 0){
					fread(slide,1,1,file);
					index[(slide[0]%4)-1] = slide[0];
					slide++;
				}
				break;
			case 0x80: //INDEX
				ins_val = (ins & 0x30)>>4;
				slide[0] = index[ins_val];
				ins_val = (ins & 0x0C)>>2;
				slide[1] = index[ins_val];
				ins_val = (ins & 0x03);
				slide[2] = index[ins_val];
				slide+=3;
				break;
			case 0xC0: //RUN
				ins_val = (ins & 0x3F)+3;
				memset(slide,slide[-1],ins_val);
				slide+=ins_val;
				break;
		}
		if(slide-out >= size)return out; 
	}
	return out;
	
}

static unsigned char index_of(unsigned char v, unsigned char* index){
	for(unsigned char i=0; i<4; i++)if(index[i] == v)return i+1;
	return 0;
}

//This function gives the exact length of the resulting file using the given value_length
//Used to optimize the file size.
static uint40 count_bytes(unsigned char* slide, uint40 size, unsigned char value_length){
	uint40 r = 0;
	unsigned char* end = slide + size - 1;
	unsigned char index[4] = {0x00,0x01,0xEF,0xFF};
	r+=16;//header
	r+=2;//first value
	slide += 1;
	while(slide < end){
		if(slide < end-3 && slide[-1] == slide[0] && slide[0] == slide[1] && slide[1] == slide[2]){
			int count = 0;
			while(slide < end && slide[-1] == slide[0] && count<65){
				count++;slide++;
			}
			r++;
		}else if(slide < end-3 && index_of(slide[0],index) && index_of(slide[1],index) && index_of(slide[2],index)){
			r++;
			slide += 3;
		}else if(slide < end-2 && ((slide[0]-slide[-1])<4 && (slide[0]-slide[-1])>=-4) && ((slide[1]-slide[0])<4 && (slide[1]-slide[0])>=-4)){
			r++;
			index[(slide[0]%4)-1] = slide[0];
			index[(slide[1]%4)-1] = slide[1];
			slide += 2;
		}else{
			if(slide + value_length > end)value_length=1;
			r+= 1 + value_length;
			for(int i=0; i<value_length; i++)index[(slide[i]%4)-1] = slide[i];
			slide += value_length;
		}
	}
	r++;//end byte
	//printf("%d : %d kB\n",value_length,r);
	return r;

}

static void write_diff(FILE* file, unsigned char f, unsigned char s){
	f = (f & 0x07)<<3;
	s = s & 0x07;
	unsigned char dc = 0x40 | f | s;
	fwrite(&dc,1,1,file);
}

static void write_run(FILE* file, unsigned char count){
	unsigned char rc = 0xC0 | count;
	fwrite(&rc,1,1,file);
}

static void write_index(FILE* file, unsigned char f, unsigned char s, unsigned char t){
	unsigned char ic = 0x80 | f<<4 | s<<2 | t;
	fwrite(&ic,1,1,file);
}

static void write_values(FILE* file, unsigned char n, unsigned char* values, unsigned char* index){
	unsigned char qty = (n & 0x3F);
	fwrite(&qty,1,1,file);
	while(n-- > 0){
		fwrite(values,1,1,file);
		index[(values[0]%4)-1] = values[0];
		values++;
	}
}

void mima_enc(FILE* file, unsigned char* data, unsigned char chan, uint40 size, uint18 width){
	if(chan == 0)width=1;
	fwrite("MIMA_FF",1,7,file);
	fwrite(&chan,1,1,file);
	unsigned char size_f[5] = {(size & 0xFF00000000)>>32,(size & 0x00FF000000)>>24,(size & 0x0000FF0000)>>16\
		,(size & 0x000000FF00)>>8,(size & 0x00000000FF)};
	fwrite(size_f,1,5,file);
	unsigned char width_f[3] = {(width & 0x0000FF0000)>>16, (width & 0x000000FF00)>>8,(width & 0x00000000FF)};
	fwrite(width_f,1,3,file);
	
	unsigned char value_length = 63;
	unsigned char value_length_c;
	uint40 min = 0xFFFFFFFFFF;
	for(value_length_c=63; value_length_c >= 1; value_length_c-=1){
		uint40 candidate = count_bytes(data,size,value_length_c);
		if(candidate < min){
			value_length = value_length_c;
			min=candidate;
		}
	}
	
	//printf("selected :%d\n",value_length);

	unsigned char* end = data + size - 1;
	unsigned char index[4] = {0x00,0x01,0xEF,0xFF};
	unsigned char* slide = data;
	write_values(file,1,slide,index);
	slide += 1;
	while(slide < end){
		if(slide < end-3 && slide[-1] == slide[0] && slide[0] == slide[1] && slide[1] == slide[2]){
			int count = 0;
			while(slide < end && slide[-1] == slide[0] && count<66){
				count++;slide++;
			}
			write_run(file,count-3);
		}else if(slide < end-3 && index_of(slide[0],index) && index_of(slide[1],index) && index_of(slide[2],index)){
			write_index(file, index_of(slide[0],index)-1, index_of(slide[1],index)-1, index_of(slide[2],index)-1);
			slide += 3;
		}else if(slide < end-2 && ((slide[0]-slide[-1])<4 && (slide[0]-slide[-1])>=-4) && ((slide[1]-slide[0])<4 && (slide[1]-slide[0])>=-4)){
			write_diff(file, slide[0]-slide[-1]+4, slide[1]-slide[0]+4);
			index[(slide[0]%4)-1] = slide[0];
			index[(slide[1]%4)-1] = slide[1];
			slide += 2;
		}else{
			if(slide + value_length > end)value_length=1;
			write_values(file,value_length,slide,index);
			slide += value_length;
		}
	}
	char emark = 0;
	fwrite(&emark,1,1,file);
}

void mima_write(char* filename, unsigned char* data, unsigned char chan, long unsigned int size, unsigned int width){
	FILE* file;
	uint40 s = size;
	uint18 w = width;

	file = fopen(filename,"wb");
	if(file == NULL)return;//Invalid file
	
	mima_enc(file, data, chan, s, w);
	
	fclose(file);
}

static unsigned char* convert_image(unsigned char* in, int npixels, char ichan, char ochan){
	unsigned char* out = malloc(npixels * ochan);
	if(out == NULL)return NULL;
	
	unsigned char* inslide = in; unsigned char* outslide = out; unsigned char rgba[4];
	for(int i = 0; i<npixels; i++){
		//upscale in
		switch(ichan){
			case 1:
				rgba[0] = inslide[0];
				rgba[1] = inslide[0];
				rgba[2] = inslide[0];
				rgba[3] = 0xFF;break;
			case 2:
				rgba[0] = inslide[0];
				rgba[1] = inslide[0];
				rgba[2] = inslide[0];
				rgba[3] = inslide[1];break;
			case 3:
				rgba[0] = inslide[0];
				rgba[1] = inslide[1];
				rgba[2] = inslide[2];
				rgba[3] = 0xFF;break;
			case 4:
				rgba[0] = inslide[0];
				rgba[1] = inslide[1];
				rgba[2] = inslide[2];
				rgba[3] = inslide[3];break;
			default: return NULL;
		}
		//downscale out
		switch(ochan){
			case 1:
				outslide[0] = rgba[0];
				break;
			case 2:
				outslide[0] = rgba[0];
				outslide[1] = rgba[3];
				break;
			case 3:
				outslide[0] = rgba[0];
				outslide[1] = rgba[1];
				outslide[2] = rgba[2];
				break;
			case 4:
				outslide[0] = rgba[0];
				outslide[1] = rgba[1];
				outslide[2] = rgba[2];
				outslide[3] = rgba[3];
				break;
			default: return NULL;
		}
		inslide += ichan;
		outslide += ochan;
	}
	free(in);
	return out;
}

unsigned char* mima_read(char* filename, int* w, int* h, unsigned char* achan, char wchan){
	FILE* file;
	uint18 we;
	uint40 he;
	//check if file is a valid file:
	file = fopen(filename,"rb");
	if(file == NULL)return NULL;//Invalid file
	
	unsigned char* r = mima_dec(file, &we, &he, achan);
	if(wchan != 0 && achan == 0){
		if(r != NULL)free(r);
		r = NULL;
	}
	if(wchan != 0 && r != NULL && *achan != wchan)r = convert_image(r, we * he, *achan, wchan);
	*w = we;
	*h = he;
	
	fclose(file);
	
	return r;
}

#endif //MIMA_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2022 evv42
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
