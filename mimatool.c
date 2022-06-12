#include <stdio.h>
#define MIMA_IMPLEMENTATION
#include "mima.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string.h>

int read_file(char* in, char* out){
	FILE* f = fopen(out,"wb");
	void* data;

	if (!f){
		printf("Invalid file output.\n");
		return 3;
	}
	int x,y;
	unsigned char n;
	data = mima_read(in, &x, &y, &n, 0);
	
	if(n == 1 || n == 3){
		fwrite("P",1,1,f);
		if(n==1){fwrite("5\n",1,2,f);}
		else{fwrite("6\n",1,2,f);}
		char sizes[64] = {0};
		snprintf(sizes,64,"%d %d\n",x,y);
		fwrite(sizes,1,strlen(sizes),f);
		fwrite("255\n",1,4,f);
	}

	if(n==0)n++;
	fwrite(data,1,x*y*n,f);
	fclose(f);
	return 0;
}

int write_file(char* in, char* out){
	FILE* f = fopen(in,"rb");
	int size;
	void* data;

	if (!f){
		printf("Invalid file.\n");
		return 3;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if (size <= 0) {
		printf("Invalid file.\n");
		return 3;
	}
	fseek(f, 0, SEEK_SET);

	data = malloc(size);
	if (!data) {
		printf("Cannot allocate that much memory.\n");
		return 3;
	}
	fread(data, 1, size, f);
	fclose(f);
	
	mima_write(out, data, 0, size, 1);
	return 0;
}

int write_image(char* in, char* out, char mono){
	int x,y,n;
	void* data = stbi_load(in, &x, &y, &n, mono);
	if(!data){
		printf("Error while reading input.\n");
		return 2;
	}
	if(mono){
		mima_write(out, data, 1, x*y, x);
	}else{
		mima_write(out, data, n, x*y*n, x);
	}
	return 0;
}

int print_help(char* name){
	printf("Compresses to MIMA.\n");
	printf("Usage:\n");
	printf("%s -h : Show this\n\n",name);
	printf("%s -i in out or \n%s -image in out : Decodes and writes an image.\n\n",name,name);
	printf("%s -m in out or \n%s -mono in out : Decodes and writes a monochrome image.\n\n",name,name);
	printf("%s -f in out or \n%s -file in out : Writes a file\n\n",name,name);
	printf("%s -d in out or \n%s -decode in out : Decodes a file/Decodes an image to PNM\n\n",name,name);
	return 1;
}

int main(int argc, char** argv){
	//really basic switch handling here
	if(argc<4 || argv[1][0] != '-')return print_help(argv[0]);
	switch(argv[1][1]){
		case 'h': return print_help(argv[0]);
		case 'i': return write_image(argv[2],argv[3],0);
		case 'm': return write_image(argv[2],argv[3],1);
		case 'f': return write_file(argv[2],argv[3]);
		case 'd': return read_file(argv[2],argv[3]);
	}
	return print_help(argv[0]);
}
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
