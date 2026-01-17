// modules/converter.h
// Header file for unit conversion module
// Declares functions and structures for JSON-based unit conversion system

#ifndef CONVERTER_H
#define CONVERTER_H

// Maximum sizes for parsing JSON conversion rules
#define MAX_UNITS 32
#define MAX_NAME_LEN 64
#define MAX_LINE_LEN 512

// Structure to hold conversion data for a single unit
typedef struct {
    char names[MAX_NAME_LEN];     // Comma-separated unit names (e.g., "in,inch")
    double to[MAX_UNITS];         // Conversion factors to other units
    double offset[MAX_UNITS];     // Offset values for conversions (for temperature)
    int to_count;                 // Number of conversion factors
    int has_offset;               // Flag indicating if offset array is present
} ConversionUnit;

// Structure to hold all conversion rules for a category (e.g., length)
typedef struct {
    char converter_units[MAX_UNITS][16];  // Unit abbreviations in order
    int converter_count;                   // Number of units in converter array
    ConversionUnit units[MAX_UNITS];      // Array of conversion units
    int unit_count;                        // Number of units defined
} ConversionRules;

// Function declarations
void trim_whitespace(char* str);
int parse_converter_array(const char* line, ConversionRules* rules);
int parse_unit_name(const char* line, char* name_buffer);
int parse_unit_to_array(const char* line, double* to_array, int* count);
int load_conversion_rules(const char* filepath, ConversionRules* rules);
int find_unit_by_name(const ConversionRules* rules, const char* name);
double convert_unit(const ConversionRules* rules, double value, const char* from_unit, const char* to_unit);
void print_available_units(const ConversionRules* rules);
void get_unit_short_name(const ConversionRules* rules, int unit_idx, char* buffer);
void convert_and_display_all(const ConversionRules* rules, double value, const char* from_unit);

#endif // CONVERTER_H
