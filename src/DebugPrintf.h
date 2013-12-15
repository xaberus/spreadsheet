#ifndef DEBUGPRINTFSTATE_H
#define DEBUGPRINTFSTATE_H

#include <stdio.h>

/*
	%z - size_t
	%p,%o - Object *
	%s, %f - const char *
	%i - int
	%l - long
	%I - unsigned int
	%L - unsigned long
	//b,B, h,H, w,W, q,Q - 8, 16, 32, 64
	g - double
*/

void debugPrintf(FILE * fp, const char * fmt, ...);

#endif /* DEBUGPRINTFSTATE_H */
