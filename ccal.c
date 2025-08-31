// ccal.c
// Simple recursive descent parser for arithmetic expressions.
// Supports numbers, + - * x / operators, unary minus, and grouping with (), [], {}.
// Works as engine for calc_gui.c, and as a standalone command line tool.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "remove_format.h"
#include "help.h"

// GLOBAL ELEMENTS:
//////////////////////////////////////////////////////////////////////////////

// Global variables.
int hasDec = 0; // don't round if no decimal
int maxDec = 0; // maximum number of meaningful decimals
int offDec = 0; // if 1 turn off decimal formatting always

// Global pointer to current position in expression string
static const char* expr_ptr;

// Skip whitespace characters in the expression
void skip_spaces() {
    while (*expr_ptr == ' ') expr_ptr++;
}

// Forward declarations
double parse_expr(int* error);
double parse_term(int* error);
double parse_factor(int* error);

// Global Functions.
// Remove formatiing characters.
void remove_format(char* s) {
    char* d = s;
    while (*s) {
        if (*s != ',' && *s != '$') *d++ = *s; // mind paste values - remove formatting
        if (*s == '.' && hasDec == 0) hasDec = 1;
        s++;
    }
    *d = '\0';
}

// Get the max number of decimals for formatting output.
void max_decimals(int* num) {
    skip_spaces();

    const char* start = expr_ptr;
    char* end;

    strtod(expr_ptr, &end); // parse the number to move expr_ptr.

    if (end == start) return; // not a number

    const char* dot = strchr(start, '.');
    if (offDec == 1) {
        hasDec = 0;
        return; // no decimal formatting
    }
    else {
        if (dot && dot < end) {
            hasDec = 1;
            int count = 0;
            const char* p = dot + 1;
            while (p < end && isdigit(*p)) {
                count++;
                p++;
            }

            // trim trailing zeros
            if (count > 2) {
                const char* t = end - 1;
                while (count > 2 && *t == '0') {
                    count--;
                    t--;
                }
            }

            if (count > *num)
                *num = count;
        }
    }

    expr_ptr = end;
}

// Format the final output.
void FormatOutput(const char* expr, double result, char* fin_str) {
    expr_ptr = expr;
    int maxDecLocal = 0;
    max_decimals(&maxDecLocal);

    if (hasDec) {
        maxDec = maxDecLocal;
    }

    if (!hasDec || offDec == 1) {
        snprintf(fin_str, 64, "%.16g", result);
    }
    else if (maxDec <= 2) {
        snprintf(fin_str, 64, "%.2f", result);
    }
    else {
        snprintf(fin_str, 64, "%.*f", maxDec, result);
    }
}

// Calculate power of call.
double power_of(double base, int expo) {
    double mathOut = base;
    for (int j = 1; j < expo; j++) {
        base = mathOut * base;
    }
    return base;
}

/*****************************************************************************
*  GUI APPLICATION USEAGE:                                                   *
*****************************************************************************/

// GUI APPLICATION - SUPPORT FUNCTIONS:
//////////////////////////////////////////////////////////////////////////////

// Shift elements in parse_term function for handling gui and -q, --quote.
double shift_parse(int* error) {
    expr_ptr++;
    return parse_factor(error);
}

// Parse a number from the expression.
double parse_number(int* error) {
    skip_spaces();

    char* end;
    const char* start = expr_ptr;
    double val = strtod(expr_ptr, &end);

    if (end == start) {
        *error = 1;
        return 0;
    }

    // count number of decimal places
    const char* dot = strchr(start, '.');
    if (dot && dot < end) {
        hasDec = 1;
        int count = 0;
        const char* p = dot + 1;
        while (p < end) {
            if (*p < '0' || *p > '9') break;
            count++;
            p++;
        }

        // check trailing zeros
        const char* t = end - 1;
        while (count > 2 && *t == '0') {
            count--;
            t--;
        }

        if (count > maxDec)
            maxDec = count;
    }

    expr_ptr = end;
    return val;
}

// Parse expressions with parentheses or brackets: (), [], {}.
double parse_paren(int* error) {
    skip_spaces();
    if (*expr_ptr == '(' || *expr_ptr == '[' || *expr_ptr == '{') {
        char open = *expr_ptr++;         // remember opening bracket and move forward
        double val = parse_expr(error);  // recursively parse inner expression
        skip_spaces();
        char close = *expr_ptr;
        // check matching closing bracket
        if ((open == '(' && close != ')') ||
            (open == '[' && close != ']') ||
            (open == '{' && close != '}')) {
            *error = 1;
            return 0;
        }
        expr_ptr++; // skip closing bracket
        return val;
    }
    // no bracket found, parse a number instead
    return parse_number(error);
}

// Parse factors, handle unary minus.
double parse_factor(int* error) {
    skip_spaces();
    if (*expr_ptr == '-') {
        expr_ptr++;  // skip '-'
        return -parse_factor(error);  // unary minus
    }
    return parse_paren(error);
}

// Parse terms: factors connected by * or / (or x).
double parse_term(int* error) {
    double left = parse_factor(error);
    while (1) {
        skip_spaces();
        if (*expr_ptr == 'x' || *expr_ptr == 'X' || *expr_ptr == '*') {
            double right = shift_parse(error);
            left *= right;
        }
        else if (*expr_ptr == '/') {
            double right = shift_parse(error);
            if (right == 0) {
                *error = 1;  // division by zero error
                return 0;
            }
            left /= right;
        }
        else if (*expr_ptr == 'p' || *expr_ptr == 'P' || *expr_ptr == '^') {
            double right = shift_parse(error);
            // power_of does not set to 1 or -1 when exponent is 0
            if (left < 0)
                left = right == 0 ? -1 : power_of(left, right);
            else
                left = right == 0 ? 1 : power_of(left, right);
        }
        else {
            break;
        }
    }
    return left;
}

// Parse expressions: terms connected by + or -.
double parse_expr(int* error) {
    double left = parse_term(error);
    while (1) {
        skip_spaces();
        if (*expr_ptr == '+') {
            expr_ptr++;
            double right = parse_term(error);
            left += right;
        }
        else if (*expr_ptr == '-') {
            expr_ptr++;
            double right = parse_term(error);
            left -= right;
        }
        else {
            if (*expr_ptr == '*' || *expr_ptr == 'x' || *expr_ptr == 'X' ||
                *expr_ptr == '/' || *expr_ptr == 'p' || *expr_ptr == 'P' ||
                *expr_ptr == '^') {
                hasDec = 0;
                maxDec = 0;
                offDec = 1;
            }
            break;
        }
    }
    return left;
}

// GUI APPLICATION - MAIN FUNCTION:
//////////////////////////////////////////////////////////////////////////////

// Evaluates an expression string and returns result. If error occurs,
// *error is set to 1.
double evaluate_expr_string(const char* expr, int* error) {
    *error = 0;
    expr_ptr = expr;  // initialize global pointer to start of expression
    double result = parse_expr(error);
    skip_spaces();
    // if there are leftover characters after parsing, error
    if (*expr_ptr != '\0') {
        *error = 1;
        return 0;
    }
    return result;
}

/*****************************************************************************
*  COMMAND LINE TOOL USEAGE:                                                 *
*****************************************************************************/

// COMMAND LINE TOOL - SUPPORT FUNCTIONS:
//////////////////////////////////////////////////////////////////////////////

// Check if parameter passed is operator.
int is_operator(const char* s) {
    return (strcmp(s, "+") == 0 || strcmp(s, "-") == 0 ||
            strcmp(s, "x") == 0 || strcmp(s, "X") == 0 ||
            strcmp(s, "p") == 0 || strcmp(s, "P") == 0 ||
            strcmp(s, "/") == 0);
}

// Handle opening nest characters.
int is_open_paren(const char* s) {
    return (strcmp(s, "(") == 0 || strcmp(s, "[") == 0 || strcmp(s, "{") == 0);
}

// Handle closing nest characters.
int is_close_paren(const char* s) {
    return (strcmp(s, ")") == 0 || strcmp(s, "]") == 0 || strcmp(s, "}") == 0);
}

// Ensure nest characters match.
int paren_match(const char* open, const char* close) {
    return (strcmp(open, "(") == 0 && strcmp(close, ")") == 0) ||
           (strcmp(open, "[") == 0 && strcmp(close, "]") == 0) ||
           (strcmp(open, "{") == 0 && strcmp(close, "}") == 0);
}

// Forward declaration.
double parse_expr_eval(int* i, char* argv[], int argc, int* error);

// Variation of parse_term for command line usage.
double parse_term_eval(int* i, char* argv[], int argc, int* error) {
    if (*i >= argc) {
        *error = 1;
        return 0;
    }

    char* tok = argv[*i];
    if (is_open_paren(tok)) {
        const char* open = tok;
        (*i)++;
        double val = parse_expr_eval(i, argv, argc, error);
        if (*error || *i >= argc      ||
            !is_close_paren(argv[*i]) || !paren_match(open, argv[*i])) {
            *error = 1;
            return 0;
        }
        (*i)++;
        return val;
    }
    else {
        double val = atof(tok);
        (*i)++;
        return val;
    }
}

// Variation of parse_expr for command line useage.
double parse_expr_eval(int* i, char* argv[], int argc, int* error) {
    double result = parse_term_eval(i, argv, argc, error);
    while (!*error && *i < argc) {
        char* op = argv[*i];
        if (!is_operator(op)) break;
        (*i)++;
        double rhs = parse_term_eval(i, argv, argc, error);

        if (*error) return 0;

        if (strcmp(op, "*") == 0 || strcmp(op, "x") == 0 || strcmp(op, "X") == 0 ||
            strcmp(op, "/") == 0 || strcmp(op, "p") == 0 || strcmp(op, "P") == 0) {
            // set for decimal count
            hasDec = 0;
            maxDec = 0;
            offDec = 1;
        }
        /* addition */
        if (strcmp(op, "+") == 0) result += rhs;

        /* subtraction */
        else if (strcmp(op, "-") == 0) result -= rhs;

        /* multiplication */
        else if (strcmp(op, "x") == 0 || strcmp(op, "X") == 0 ||
                 strcmp(op, "*") == 0) result *= rhs;

        /* division */
        else if (strcmp(op, "/") == 0) {
            if (rhs == 0) {
                *error = 1;
                return 0;
            }
            result /= rhs;
        }
        else if (strcmp(op, "p") == 0 || strcmp(op, "P") == 0) {
            if (result < 0)
                result = rhs == 0 ? -1 : power_of(result, rhs);
            else
                result = rhs == 0 ? 1 : power_of(result, rhs);
        }
    }
    return result;
}

// Internal token-array based evaluator.
double evaluate(int argc, char* argv[], int* error) {
    *error = 0;
    if (argc < 1) {
        *error = 1;
        return 0;
    }

    int index = 0;
    double result = parse_expr_eval(&index, argv, argc, error);

    if (index != argc) {
        *error = 1;
        return 0;
    }
    return result;
}

// COMMAND LINE TOOL - MAIN FUNCTION:
//////////////////////////////////////////////////////////////////////////////

// If not compiling GUI with:
// gcc -DBUILDING_GUI ccal.c ccal_gui.c -o ccal_gui.exe -mwindows
#ifndef BUILDING_GUI
int main(int argc, char* argv[]) {
    if (argc == 1 ||
       (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
       )
    {
        // output help document
        fwrite(help_txt, 1, help_txt_len, stdout);
        return 0;
    }

    int error;
    double result;

    // check if quoted option passed
    if (strcmp(argv[1], "-q") == 0 || strcmp(argv[1], "--quote") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing expression after -q\n");
            return 1;
        }

        // duplicate and clean expression
        char* expr = strdup(argv[2]);
        if (!expr) {
            fprintf(stderr, "Memory error\n");
            return 1;
        }
        remove_format(expr);
        result = evaluate_expr_string(expr, &error);
        free(expr);
    } else {
        // regular token-based input
        char** cleaned_args = malloc((argc - 1) * sizeof(char*));
        if (!cleaned_args) {
            fprintf(stderr, "Memory error\n");
            return 1;
        }

        for (int i = 0; i < argc - 1; ++i) {
            cleaned_args[i] = strdup(argv[i + 1]);
            if (!cleaned_args[i]) {
                fprintf(stderr, "Memory error\n");
                return 1;
            }
            remove_format(cleaned_args[i]);
        }

        result = evaluate(argc - 1, cleaned_args, &error);

        for (int i = 0; i < argc - 1; ++i) {
            free(cleaned_args[i]);
        }

        free(cleaned_args);
    }

    if (error) {
        printf("Error: Invalid expression\n");
        return 1;
    }
    // output according to equation type and max decimals
    int maxDecLocal = 0;
    expr_ptr = argv[1];  // reset expression pointer to beginning
    max_decimals(&maxDecLocal);

    // condition for final output
    if (hasDec) {
        maxDec = maxDecLocal;
    }
    if (!hasDec) {
        printf("%.16g\n", result);
    }
    else if (maxDec <= 2) {
        if (offDec == 1) {
            printf("%.16g\n", result);
        }
        else {
            printf("%.2f\n", result);
        }
    }
    else {
        if (offDec == 1) {
            printf("%.16g\n", result);
        }
        else {
            printf("%.*f\n", maxDec, result);
        }
    }

    return 0;
}
#endif


