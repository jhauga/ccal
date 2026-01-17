// modules/converter.c
// Unit conversion module for ccal calculator
// Reads JSON rule files from rules/converter/ directory and performs conversions
// Supports flexible unit naming (case-insensitive, multiple aliases per unit)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

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

// Trim whitespace from both ends of a string
void trim_whitespace(char* str) {
    char* start = str;
    char* end;
    
    // Trim leading whitespace
    while (isspace((unsigned char)*start)) start++;
    
    if (*start == 0) {
        *str = '\0';
        return;
    }
    
    // Trim trailing whitespace
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Parse the converter array from JSON (e.g., ["mm", "cm", "m", ...])
int parse_converter_array(const char* line, ConversionRules* rules) {
    const char* p = strchr(line, '[');
    if (!p) return 0;
    p++;
    
    rules->converter_count = 0;
    char buffer[32];
    int buf_idx = 0;
    int in_quotes = 0;
    
    while (*p && rules->converter_count < MAX_UNITS) {
        if (*p == '"') {
            in_quotes = !in_quotes;
            if (!in_quotes && buf_idx > 0) {
                // End of quoted string
                buffer[buf_idx] = '\0';
                trim_whitespace(buffer);
                strcpy(rules->converter_units[rules->converter_count], buffer);
                rules->converter_count++;
                buf_idx = 0;
            }
        } else if (in_quotes) {
            if (buf_idx < 31) {
                buffer[buf_idx++] = *p;
            }
        } else if (*p == ']') {
            break;
        }
        p++;
    }
    
    return rules->converter_count > 0;
}

// Parse a unit's name field from JSON
int parse_unit_name(const char* line, char* name_buffer) {
    const char* p = strstr(line, "\"name\"");
    if (!p) return 0;
    
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    
    // Skip whitespace
    while (*p && isspace(*p)) p++;
    
    if (*p != '"') return 0;
    p++;
    
    // Copy until closing quote
    int idx = 0;
    while (*p && *p != '"' && idx < MAX_NAME_LEN - 1) {
        name_buffer[idx++] = *p++;
    }
    name_buffer[idx] = '\0';
    
    return 1;
}

// Parse a unit's offset values from JSON (the "offset" array for temperature)
int parse_unit_offset_array(const char* line, double* offset_array, int* count) {
    const char* p = strstr(line, "\"offset\"");
    if (!p) return 0;
    
    p = strchr(p, '[');
    if (!p) return 0;
    p++;
    
    *count = 0;
    char buffer[32];
    int buf_idx = 0;
    
    while (*p && *count < MAX_UNITS) {
        if (isdigit(*p) || *p == '.' || *p == '-' || *p == 'e' || *p == 'E') {
            if (buf_idx < 31) {
                buffer[buf_idx++] = *p;
            }
        } else if ((*p == ',' || *p == ']') && buf_idx > 0) {
            buffer[buf_idx] = '\0';
            offset_array[*count] = atof(buffer);
            (*count)++;
            buf_idx = 0;
            if (*p == ']') break;
        }
        p++;
    }
    
    return *count > 0;
}

// Parse a unit's conversion factors from JSON (the "to" array)
int parse_unit_to_array(const char* line, double* to_array, int* count) {
    const char* p = strstr(line, "\"to\"");
    if (!p) return 0;
    
    p = strchr(p, '[');
    if (!p) return 0;
    p++;
    
    *count = 0;
    char buffer[32];
    int buf_idx = 0;
    
    while (*p && *count < MAX_UNITS) {
        if (isdigit(*p) || *p == '.' || *p == '-' || *p == 'e' || *p == 'E') {
            if (buf_idx < 31) {
                buffer[buf_idx++] = *p;
            }
        } else if ((*p == ',' || *p == ']') && buf_idx > 0) {
            buffer[buf_idx] = '\0';
            to_array[*count] = atof(buffer);
            (*count)++;
            buf_idx = 0;
            if (*p == ']') break;
        }
        p++;
    }
    
    return *count > 0;
}

// Load conversion rules from a JSON file
int load_conversion_rules(const char* filepath, ConversionRules* rules) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open rules file: %s\n", filepath);
        return 0;
    }
    
    rules->converter_count = 0;
    rules->unit_count = 0;
    
    char line[MAX_LINE_LEN];
    int in_unit_object = 0;
    int current_unit_has_offset = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        trim_whitespace(line);
        
        // Parse converter array
        if (strstr(line, "\"converter\"") && rules->converter_count == 0) {
            parse_converter_array(line, rules);
        }
        // Detect start of unit object
        else if (strstr(line, "\"name\"")) {
            in_unit_object = 1;
            current_unit_has_offset = 0;
            if (rules->unit_count < MAX_UNITS) {
                parse_unit_name(line, rules->units[rules->unit_count].names);
                rules->units[rules->unit_count].has_offset = 0;
                // Initialize offset array to zeros
                for (int i = 0; i < MAX_UNITS; i++) {
                    rules->units[rules->unit_count].offset[i] = 0.0;
                }
            }
        }
        // Parse conversion factors
        else if (in_unit_object && strstr(line, "\"to\"")) {
            if (rules->unit_count < MAX_UNITS) {
                parse_unit_to_array(line, 
                                   rules->units[rules->unit_count].to,
                                   &rules->units[rules->unit_count].to_count);
                // Don't increment unit_count yet, might have offset array next
            }
        }
        // Parse offset values (optional)
        else if (in_unit_object && strstr(line, "\"offset\"")) {
            if (rules->unit_count < MAX_UNITS) {
                int offset_count;
                if (parse_unit_offset_array(line, 
                                           rules->units[rules->unit_count].offset,
                                           &offset_count)) {
                    rules->units[rules->unit_count].has_offset = 1;
                    current_unit_has_offset = 1;
                }
            }
        }
        // Detect end of unit object (closing brace)
        else if (in_unit_object && strchr(line, '}')) {
            if (rules->units[rules->unit_count].to_count > 0) {
                rules->unit_count++;
            }
            in_unit_object = 0;
        }
    }
    
    fclose(fp);
    return rules->unit_count > 0;
}

// Find a unit by name (case-insensitive, checks all aliases)
int find_unit_by_name(const ConversionRules* rules, const char* name) {
    for (int i = 0; i < rules->unit_count; i++) {
        char names_copy[MAX_NAME_LEN];
        strcpy(names_copy, rules->units[i].names);
        
        // Check each comma-separated name
        char* token = strtok(names_copy, ",");
        while (token) {
            trim_whitespace(token);
            
            // Case-insensitive comparison
            if (strcasecmp(token, name) == 0) {
                return i;
            }
            token = strtok(NULL, ",");
        }
    }
    return -1;
}

// Convert a value from one unit to another
double convert_unit(const ConversionRules* rules, double value, 
                   const char* from_unit, const char* to_unit) {
    int from_idx = find_unit_by_name(rules, from_unit);
    if (from_idx < 0) {
        fprintf(stderr, "Error: Unknown unit '%s'\n", from_unit);
        return 0;
    }
    
    // Find the target unit in the converter array
    int to_idx = -1;
    for (int i = 0; i < rules->converter_count; i++) {
        if (strcasecmp(rules->converter_units[i], to_unit) == 0) {
            to_idx = i;
            break;
        }
    }
    
    if (to_idx < 0) {
        fprintf(stderr, "Error: Unknown target unit '%s'\n", to_unit);
        return 0;
    }
    
    if (to_idx >= rules->units[from_idx].to_count) {
        fprintf(stderr, "Error: Conversion not defined\n");
        return 0;
    }
    
    // Apply conversion: result = (value * factor) + offset
    double result = value * rules->units[from_idx].to[to_idx];
    if (rules->units[from_idx].has_offset) {
        result += rules->units[from_idx].offset[to_idx];
    }
    
    return result;
}

// Print available units for a given rule set
void print_available_units(const ConversionRules* rules) {
    printf("Available units:\n");
    for (int i = 0; i < rules->unit_count; i++) {
        printf("  %s\n", rules->units[i].names);
    }
}

// Get the short name (first name in the list) for display
void get_unit_short_name(const ConversionRules* rules, int unit_idx, char* buffer) {
    if (unit_idx < 0 || unit_idx >= rules->unit_count) {
        strcpy(buffer, "unknown");
        return;
    }
    
    char names_copy[MAX_NAME_LEN];
    strcpy(names_copy, rules->units[unit_idx].names);
    
    char* first_name = strtok(names_copy, ",");
    if (first_name) {
        trim_whitespace(first_name);
        strcpy(buffer, first_name);
    } else {
        strcpy(buffer, "unknown");
    }
}

// Convert and display results to all units
void convert_and_display_all(const ConversionRules* rules, double value, 
                             const char* from_unit) {
    int from_idx = find_unit_by_name(rules, from_unit);
    if (from_idx < 0) {
        fprintf(stderr, "Error: Unknown unit '%s'\n", from_unit);
        return;
    }
    
    char from_short[32];
    get_unit_short_name(rules, from_idx, from_short);
    
    printf("%.4f %s =\n", value, from_short);
    
    for (int i = 0; i < rules->converter_count; i++) {
        double result = value * rules->units[from_idx].to[i];
        if (rules->units[from_idx].has_offset) {
            result += rules->units[from_idx].offset[i];
        }
        printf("  %-12s : %.6f\n", rules->converter_units[i], result);
    }
}

// Count JSON files in rules/converter directory
int count_rule_files(char* single_rule_name) {
    #ifdef _WIN32
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile("rules/converter/*.json", &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    int count = 0;
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            count++;
            if (count == 1 && single_rule_name) {
                // Extract filename without extension
                char* dot = strrchr(find_data.cFileName, '.');
                if (dot) {
                    size_t len = dot - find_data.cFileName;
                    strncpy(single_rule_name, find_data.cFileName, len);
                    single_rule_name[len] = '\0';
                }
            }
        }
    } while (FindNextFile(hFind, &find_data) != 0);
    
    FindClose(hFind);
    return count;
    #else
    DIR* dir = opendir("rules/converter");
    if (!dir) return 0;
    
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char* ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".json") == 0) {
                count++;
                if (count == 1 && single_rule_name) {
                    size_t len = ext - entry->d_name;
                    strncpy(single_rule_name, entry->d_name, len);
                    single_rule_name[len] = '\0';
                }
            }
        }
    }
    closedir(dir);
    return count;
    #endif
}

// Example usage demonstration (can be removed when integrating with main)
#ifdef CONVERTER_STANDALONE
int main(int argc, char* argv[]) {
    // Count available rule files
    char single_rule[64] = {0};
    int rule_count = count_rule_files(single_rule);
    
    // Determine if we're using single-rule or multi-rule syntax
    int arg_offset = 0;
    char filepath[256];
    
    if (rule_count == 0) {
        fprintf(stderr, "Error: No rule files found in rules/converter/\n");
        return 1;
    } else if (rule_count == 1) {
        // Single rule file - rule name is optional
        // Try to detect if first arg is the rule name or a value
        if (argc < 3) {
            printf("Usage: %s [<rule>] <value> <from_unit> <to_unit>\n", argv[0]);
            printf("   or: %s [<rule>] <value> <from_unit> --all\n", argv[0]);
            printf("\nExamples:\n");
            printf("  %s 10 inch cm\n", argv[0]);
            printf("  %s %s 10 inch cm\n", argv[0], single_rule);
            printf("  %s 5 ft --all\n", argv[0]);
            printf("\nUsing rule: %s\n", single_rule);
            return 1;
        }
        
        // Check if first argument matches the single rule name
        if (argc >= 4 && strcasecmp(argv[1], single_rule) == 0) {
            // Rule name was provided: converter length 1 in cm
            arg_offset = 2;
        } else {
            // Rule name omitted: converter 1 in cm
            arg_offset = 1;
        }
        snprintf(filepath, sizeof(filepath), "rules/converter/%s.json", single_rule);
    } else {
        // Multiple rule files - rule name is required
        if (argc < 4) {
            printf("Usage: %s <rule> <value> <from_unit> <to_unit>\n", argv[0]);
            printf("   or: %s <rule> <value> <from_unit> --all\n", argv[0]);
            printf("\nExamples:\n");
            printf("  %s length 10 inch cm\n", argv[0]);
            printf("  %s temperature 32 F C\n", argv[0]);
            printf("  %s length 5 ft --all\n", argv[0]);
            printf("\nAvailable rules:\n");
            printf("  length, temperature\n");
            return 1;
        }
        arg_offset = 2;  // argv[1] = rule, argv[2] = value, argv[3] = from_unit, argv[4] = to_unit
        const char* rule_name = argv[1];
        snprintf(filepath, sizeof(filepath), "rules/converter/%s.json", rule_name);
    }
    
    ConversionRules rules;
    if (!load_conversion_rules(filepath, &rules)) {
        fprintf(stderr, "Failed to load conversion rules from: %s\n", filepath);
        if (rule_count > 1) {
            fprintf(stderr, "Available rules: length, temperature\n");
        }
        return 1;
    }
    
    double value = atof(argv[arg_offset]);
    const char* from_unit = argv[arg_offset + 1];
    
    if (argc >= arg_offset + 3 && strcmp(argv[arg_offset + 2], "--all") == 0) {
        convert_and_display_all(&rules, value, from_unit);
    } else if (argc >= arg_offset + 3) {
        const char* to_unit = argv[arg_offset + 2];
        double result = convert_unit(&rules, value, from_unit, to_unit);
        
        char from_short[32], to_short[32];
        int from_idx = find_unit_by_name(&rules, from_unit);
        get_unit_short_name(&rules, from_idx, from_short);
        
        printf("%.4f %s = %.6f %s\n", value, from_short, result, to_unit);
    } else {
        fprintf(stderr, "Error: Missing target unit or --all flag\n");
        return 1;
    }
    
    return 0;
}
#endif
