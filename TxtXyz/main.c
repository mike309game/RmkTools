#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <locale.h>
#include <zlib.h>
#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, const char** argv) {
	#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); //set console to utf8 
	#endif
	setlocale(LC_ALL, "en_US.UTF-8");
	for(int i = 1; i < argc; i++) {
		FILE* fp = fopen(argv[i], "r");
		if(!fp) {
			printf("Can't open %s\n", argv[i]);
			continue;
		}
		int fsize;
		unsigned short w, h;
		fsize = 0, w = 0, h = 0;
		uint8_t* data, *compressed;
		//uint8_t* compressed;
		
		fseek(fp, 0, SEEK_END);
		fsize = ftell(fp);
		fseek(fp, 4, SEEK_SET);
		
		fread(&w, 2, 1, fp);
		fread(&h, 2, 1, fp);
		
		compressed = 	(uint8_t*)malloc(fsize - 8);
		data = 			(uint8_t*)malloc(768 + (w*h));
		fread(compressed, fsize-8, 1, fp);
		fclose(fp);
		unsigned long unused = 768 + (w*h);
		uncompress(data, &unused, compressed, fsize-8);
		free(compressed);
		
		uint8_t* src = data+768;
		uint8_t(*palette)[3] = (uint8_t(*)[3])data;
		printf("%d %d\n", w, h);
		for(int j = 0; j < 256; j++) {
			uint8_t* colour = palette[j];
			printf("%02X %02X %02X,%s", colour[0], colour[1], colour[2], (((j+1)%16) == 0) ? "\n" : "");
		}
		for(int j = 0; j < (w*h); j++) {
			printf("%02X %s", src[j], (((j+1)%w) == 0) ? "\n" : "");
		}
		free(data);
	}
	return 0;
}