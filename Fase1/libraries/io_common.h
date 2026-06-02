#ifndef IO_COMMON_H
#define IO_COMMON_H

#include "my_stdarg.h"

int alpha2int(const char *s);
char *int2alpha(int num, char *buffer);
char *float2alpha(float num, char *buffer);
float alpha2float(const char *s);

#endif // IO_COMMON_H