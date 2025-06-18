// ccal.c
// Simple recursive descent parser for arithmetic expressions.
// Supports numbers, + - * x / operators, unary minus, and grouping with (), [], {}.
// Works as engine for calc_gui.c, and as a standalone command line tool.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// GLOBAL ELEMENTS:
//////////////////////////////////////////////////////////////////////////////

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

/*****************************************************************************
*  GUI APPLICATION USEAGE:                                                   *
*****************************************************************************/

// GUI APPLICATION - SUPPORT FUNCTIONS:
//////////////////////////////////////////////////////////////////////////////

// Parse a number from the expression.
double parse_number(int* error) {
    skip_spaces();
    char* end;
    double val = strtod(expr_ptr, &end);  // convert string to double
    if (end == expr_ptr) {  // no number found
        *error = 1;
        return 0;
    }
    expr_ptr = end;  // move pointer past parsed number
    return val;
}

// Parse expressions with parentheses or brackets: (), [], {}.
double parse_paren(int* error) {
    skip_spaces();
    if (*expr_ptr == '(' || *expr_ptr == '[' || *expr_ptr == '{') {
        char open = *expr_ptr++;  // remember opening bracket and move forward
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
        if (*expr_ptr == 'x' || *expr_ptr == '*') {
            expr_ptr++;
            double right = parse_factor(error);
            left *= right;
        }
        else if (*expr_ptr == '/') {
            expr_ptr++;
            double right = parse_factor(error);
            if (right == 0) {
                *error = 1;  // division by zero error
                return 0;
            }
            left /= right;
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
        strcmp(s, "x") == 0 || strcmp(s, "/") == 0);
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
        if (*error || *i >= argc || !is_close_paren(argv[*i]) || !paren_match(open, argv[*i])) {
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

        if (strcmp(op, "+") == 0) result += rhs;
        else if (strcmp(op, "-") == 0) result -= rhs;
        else if (strcmp(op, "x") == 0) result *= rhs;
        else if (strcmp(op, "/") == 0) {
            if (rhs == 0) {
                *error = 1;
                return 0;
            }
            result /= rhs;
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
    if (argc < 2) {
        printf("Usage: %s num1 op num2 [op num3 ...] - use (), [], {} for grouping.\n", argv[0]);
        return 1;
    }

    int error;
    double result = evaluate(argc - 1, &argv[1], &error);
    if (error) {
        printf("Error: Invalid expression\n");
        return 1;
    }

    printf("%g\n", result);
    return 0;
}
#endif
