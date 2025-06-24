// remove_format.h
// Remove all formatting from a string, modifying it in-place

#ifndef REMOVE_FORMAT_H
#define REMOVE_FORMAT_H

// Global variables.
// Formats calculated number according to if decimal exists in any 
// formula digits.
extern int hasDec; 
extern int offDec;
extern int maxDec;
extern int maxDecLocal;

// Forward declarations.
void remove_format(char* str);
void max_decimals(int* num);
void FormatOutput(const char* expr, double result, char* fin_str);

#endif
