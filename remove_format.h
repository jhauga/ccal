// remove_format.h
// Remove all formatting from a string, modifying it in-place

#ifndef REMOVE_FORMAT_H
#define REMOVE_FORMAT_H

// Global variables.
// Formats calculated number according to if decimal exists in any 
// formula digits.
extern int hasDec; 

// Forward declarations.
void remove_format(char* str);

#endif
