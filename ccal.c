// ccal.c
// Simple GUI and command line calculator, allowing for arithmetic equations.
// Supports numbers, + - * x / operators, unary minus, and grouping with (), [], {}.
// Works as engine for calc_gui.c, and as a standalone command line tool.
// Note 1 This translation unit contains both the reusable parser and the CLI entry point, so reading sequentially reveals how the evaluator is layered from helpers up to user-facing interfaces.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "remove_format.h"
#include "help.h"
// Note 2 The headers above mix standard C libraries for core facilities with project headers; recognizing which features stem from libc helps when porting this parser to constrained environments.

// GLOBAL ELEMENTS:
//////////////////////////////////////////////////////////////////////////////

// Global variables.
int hasDec = 0; // don't round if no decimal
int maxDec = 0; // maximum number of meaningful decimals
int offDec = 0; // if 1 turn off decimal formatting always
// Note 3 These globals act as formatting state that persists across parsing passes, which mirrors the parser's single-threaded nature; in multi-threaded contexts you would wrap them in a struct instead.

// Global pointer to current position in expression string
static const char* expr_ptr;
// Note 4 Maintaining a global pointer into the source string enables the classic recursive-descent pattern where each function consumes characters and leaves the remainder for its caller.

// Skip whitespace characters in the expression
void skip_spaces() {
    // Note 75 Although minimal, this loop saves every other parser function from duplicating whitespace handling, highlighting the DRY principle in C.
    while (*expr_ptr == ' ') expr_ptr++;
}
// Note 5 Skipping whitespace must be called before each token-consuming routine so that the parser treats space as optional syntactic sugar rather than significant input.

// Forward declarations
double parse_expr(int* error);
double parse_term(int* error);
double parse_factor(int* error);
// Note 6 These forward declarations mirror the precedence hierarchy (expr > term > factor), reinforcing the idea that each level delegates to tighter-binding lower levels.
// Note 76 Having prototypes near the top also makes it easy to swap the implementation order later without breaking older C90 compilers that require declarations before use.

// Global Functions.
// Remove formatiing characters.
void remove_format(char* s) {
    char* d = s;
    // Note 60 Using separate read and write pointers lets us strip unwanted characters in-place without extra allocations—a common C idiom worth practicing.
    while (*s) {
        if (*s != ',' && *s != '$') *d++ = *s; // mind paste values - remove formatting
        if (*s == '.' && hasDec == 0) hasDec = 1;
        s++;
    }
    *d = '\0';
}
// Note 7 This sanitizer doubles as a fast path for detecting decimal input, demonstrating how preprocessing can cheaply seed state for later formatting decisions.
// Note 77 Terminating the string manually is crucial because we may have skipped characters, so relying on the original null terminator would risk leaving stray data at the end.

// Get the max number of decimals for formatting output.
void max_decimals(int* num) {
    skip_spaces();

    const char* start = expr_ptr;
    char* end;
    // Note 61 Capturing the start pointer allows us to detect whether strtod consumed any characters, which differentiates numbers from operators or stray symbols.

    strtod(expr_ptr, &end); // parse the number to move expr_ptr.
    // Note 50 Calling strtod without storing the result is intentional—we only care how many characters form the number to infer decimal precision.

    if (end == start) return; // not a number

    const char* dot = strchr(start, '.');
    if (offDec == 1) {
        hasDec = 0;
        return; // no decimal formatting
    }
    else {
        // Note 51 When offDec is clear we honor user formatting, treating decimal places as significant unless they are trailing zeros beyond cents precision.
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
        // Note 80 Advancing the scanning pointer ensures the next iteration evaluates the remainder of the expression, eventually terminating when we reach '\0'.
    }

    expr_ptr = end;
}
// Note 8 Tracking the maximum decimals while walking the expression lets the formatter respect user intent—notice how trailing zeros are trimmed only beyond two places to balance fidelity and readability.

// Format the final output based on decimal precision found in the expression.
void FormatOutput(const char* expr, double result, char* fin_str) {
    if (offDec == 1) {
        hasDec = 0;
        maxDec = 0;
        snprintf(fin_str, 64, "%.16g", result);
        return;
    }
    // Note 9 The offDec flag short-circuits formatting for operations like multiplication where scientific precision matters more than user-friendly grouping.

    int localHasDec = 0;
    int localMaxDec = 0;
    const char* p = expr;

    while (*p) {
        while (*p == ' ' || *p == '\t')
            ++p;
        // Note 10 Skipping spaces on every iteration keeps the scanner tolerant of user formatting, mirroring the approach used by the parser itself.
        // Note 79 Only space and tab are handled because command-line parsing from argv strips other whitespace characters, simplifying the normalization logic.

        char* end;
        strtod(p, &end);
        if (end == p) {
            if (*p == '\0')
                break;
            ++p;
            continue;
        }
        // Note 11 Using strtod here avoids hand-written numeric parsing and automatically supports locale-independent decimal notation required by the calculator.
        // Note 78 If strtod fails, we advance by one character to avoid getting stuck—this mirrors primitive lexers that recover by skipping unknown symbols.

        const char* dot = NULL;
        for (const char* q = p; q < end; ++q) {
            if (*q == '.') {
                dot = q;
                break;
            }
        }

        if (dot) {
            localHasDec = 1;
            int count = 0;
            const char* s = dot + 1;
            while (s < end && isdigit((unsigned char)*s)) {
                ++count;
                ++s;
            }
            // Note 12 Counting digits after the decimal helps decide whether to keep precision or clamp to two decimals for currency-style outputs.

            if (count > 2) {
                const char* t = end - 1;
                while (count > 2 && *t == '0') {
                    --count;
                    --t;
                }
                // Note 62 This backward scan prevents numbers like 3.1400 from being reported with four decimals when only two carry meaning, a nuance finance learners often appreciate.
            }
            // Note 13 Trimming trailing zeros illustrates a common formatting compromise: align with human expectations without losing core numeric intent.

            if (count > localMaxDec)
                localMaxDec = count;
        }

        p = end;
    }

    if (!localHasDec) {
        hasDec = 0;
        maxDec = 0;
        snprintf(fin_str, 64, "%.16g", result);
    }
    else {
        hasDec = 1;
        maxDec = localMaxDec;
        
        // Format with appropriate precision first
        char temp_str[64];
        if (maxDec <= 2)
            snprintf(temp_str, 64, "%.2f", result);
        else
            snprintf(temp_str, 64, "%.*f", maxDec, result);
            
        // Check if the result is effectively an integer (decimal part is all zeros)
        char* dot_pos = strchr(temp_str, '.');
        if (dot_pos != NULL) {
            // Check if all characters after decimal point are zeros
            char* p = dot_pos + 1;
            int all_zeros = 1;
            while (*p != '\0') {
                if (*p != '0') {
                    all_zeros = 0;
                    break;
                }
                p++;
            }
            
            // If all decimal digits are zeros, format as integer
            if (all_zeros) {
                // Extract the integer part and format without decimal
                *dot_pos = '\0';  // Terminate string at decimal point
                strcpy(fin_str, temp_str);
            } else {
                strcpy(fin_str, temp_str);
            }
        } else {
            strcpy(fin_str, temp_str);
        }
        // Note 63 Leveraging snprintf with a precision field (%.*f) is a powerful technique when formatting rules depend on runtime analysis rather than fixed templates.
    }
}
// Note 14 The conditional at the end keeps integer results compact while still honoring high-precision operands, showcasing a user-centric formatting strategy.

// Calculate power of call.
double power_of(double base, int expo) {
    double mathOut = base;
    for (int j = 1; j < expo; j++) {
        base = mathOut * base;
    }
    return base;
}
// Note 15 This simple loop avoids pow from math.h, sidestepping floating-point rounding differences that could complicate regression testing.

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
// Note 16 Advancing expr_ptr past an operator before parsing the right-hand side keeps parse_term concise and emphasizes that token consumption happens at the higher-precedence caller.

// Parse a number from the expression.
double parse_number(int* error) {
    skip_spaces();
    // Note 17 Every numeric parse begins by normalizing whitespace, mirroring lexical scanners that separate tokenization from grammar handling.

    char* end;
    const char* start = expr_ptr;
    double val = strtod(expr_ptr, &end);

    if (end == start) {
        *error = 1;
        return 0;
    }
    // Note 18 Returning an error when no digits are consumed prevents infinite loops where the caller would otherwise keep retrying at the same position.

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
        // Note 19 The loop counts only digits, so scientific notation like 1e-3 would stop at 'e'; adapting to new formats would require extending this branch.

        // check trailing zeros
        const char* t = end - 1;
        while (count > 2 && *t == '0') {
            count--;
            t--;
        }
        // Note 20 This trimming mirrors the formatting rules, teaching how evaluation and presentation share responsibilities in numeric software.

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
    // Note 64 Recursive descent shines here: handling nested groups is as straightforward as calling parse_expr again, with the call stack tracking context for us.
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
// Note 21 Allowing three bracket styles makes the parser friendlier to clipboard input from spreadsheets or programming languages; the matching check guards against silent math errors when the user mistypes.

// Parse factors, handle unary minus.
double parse_factor(int* error) {
    skip_spaces();
    if (*expr_ptr == '-') {
        expr_ptr++;  // skip '-'
        return -parse_factor(error);  // unary minus
        // Note 65 Flipping the sign after the recursive call maintains compatibility with expressions like -(-3), which should resolve to +3.
    }
    return parse_paren(error);
}
// Note 22 Handling unary minus here keeps the grammar simple: a factor can become negative without introducing separate tokens or precedence rules.

// Parse terms: factors connected by * or / (or x).
double parse_term(int* error) {
    double left = parse_factor(error);
    while (1) {
        skip_spaces();
        // Note 52 Because the parser is character-driven, skipping spaces inside the loop ensures operators like "x" or "/" are detected even when the user adds extra padding.
        if (*expr_ptr == 'x' || *expr_ptr == 'X' || *expr_ptr == '*') {
            double right = shift_parse(error);
            left *= right;
            // Note 66 Accepting both 'x' and '*' makes the calculator ergonomic on keyboards where typing '*' requires Shift, a thoughtful UX choice.
        }
        else if (*expr_ptr == '/') {
            double right = shift_parse(error);
            if (right == 0) {
                *error = 1;  // division by zero error
                return 0;
            }
            left /= right;
            // Note 67 Division falls back to floating-point, so even integer inputs can yield fractional results, reinforcing why formatting must adapt dynamically.
        }
        else if (*expr_ptr == 'p' || *expr_ptr == 'P' || *expr_ptr == '^') {
            double right = shift_parse(error);
            // power_of does not set to 1 or -1 when exponent is 0
            if (left < 0)
                left = right == 0 ? -1 : power_of(left, right);
            else
                left = right == 0 ? 1 : power_of(left, right);
            // Note 68 Using integer exponents keeps evaluation predictable—raising a number to 2.5 would require a more sophisticated numeric library.
        }
        else {
            break;
        }
    }
    return left;
}
// Note 23 This loop embodies operator precedence: by staying here until a non-term operator appears, multiplication, division, and exponentiation naturally bind tighter than addition or subtraction.
// Note 24 The explicit division-by-zero check produces a clean parser error instead of relying on IEEE exceptions, which keeps feedback consistent across platforms.
// Note 25 Exponentiation treats zero exponents as a special case to avoid raising 0^0, a helpful nod to discrete math rules that learners often encounter in coursework.

// Parse expressions: terms connected by + or -.
double parse_expr(int* error) {
    double left = parse_term(error);
    while (1) {
        skip_spaces();
        if (*expr_ptr == '+') {
            expr_ptr++;
            // Note 53 Incrementing expr_ptr consumes the operator so the recursive call sees the remainder of the expression without extra bookkeeping.
            double right = parse_term(error);
            left += right;
        }
        else if (*expr_ptr == '-') {
            expr_ptr++;
            // Note 54 The same pattern applies to subtraction, reinforcing that recursive descent can be implemented with minimal state.
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
                // Note 69 Encountering a high-precedence operator here usually means the user omitted an operand; flipping offDec ensures whatever happens next is shown without rounding assumptions.
            }
            break;
        }
    }
    return left;
}
// Note 26 By calling parse_term for both operands, this level implements left associativity—addition and subtraction are applied in the order encountered.
// Note 27 The final branch toggles offDec when encountering higher-precedence symbols in unexpected positions, reinforcing that formatting should err on the side of precision if the grammar becomes ambiguous.

// GUI APPLICATION - MAIN FUNCTION:
//////////////////////////////////////////////////////////////////////////////

// Evaluates an expression string and returns result. If error occurs,
// *error is set to 1.
double evaluate_expr_string(const char* expr, int* error) {
    *error = 0;
    expr_ptr = expr;  // initialize global pointer to start of expression
    double result = parse_expr(error);
    skip_spaces();
    // Note 70 Trailing spaces are ignored so that copying expressions from text editors does not inadvertently trigger parse errors.
    // if there are leftover characters after parsing, error
    if (*expr_ptr != '\0') {
        *error = 1;
        return 0;
    }
    return result;
}
// Note 28 Resetting expr_ptr at the entry point lets you reuse the same parsing machinery for both GUI input and command-line expressions without reinitializing global state elsewhere.

/*****************************************************************************
*  COMMAND LINE TOOL USEAGE:                                                 *
*****************************************************************************/

// COMMAND LINE TOOL - SUPPORT FUNCTIONS:
//////////////////////////////////////////////////////////////////////////////
// Note 59 Everything below adapts the core evaluator for argv-style inputs, showing how parsing logic can be shared across interfaces with thin wrappers.

// Check if parameter passed is operator.
int is_operator(const char* s) {
    return (strcmp(s, "+") == 0 || strcmp(s, "-") == 0 ||
            strcmp(s, "x") == 0 || strcmp(s, "X") == 0 ||
            strcmp(s, "p") == 0 || strcmp(s, "P") == 0 ||
            strcmp(s, "/") == 0);
}
// Note 29 Token-based evaluation uses string comparisons instead of characters so the same function can interpret user-provided argv segments verbatim.

// Handle opening nest characters.
int is_open_paren(const char* s) {
    return (strcmp(s, "(") == 0 || strcmp(s, "[") == 0 || strcmp(s, "{") == 0);
}
// Note 30 Treating all bracket styles uniformly keeps the CLI behavior aligned with the GUI, making automated tests easier to write.

// Handle closing nest characters.
int is_close_paren(const char* s) {
    return (strcmp(s, ")") == 0 || strcmp(s, "]") == 0 || strcmp(s, "}") == 0);
}
// Note 31 Having symmetric helpers for open and close delimiters avoids duplicating matching logic later in the stack.

// Ensure nest characters match.
int paren_match(const char* open, const char* close) {
    return (strcmp(open, "(") == 0 && strcmp(close, ")") == 0) ||
           (strcmp(open, "[") == 0 && strcmp(close, "]") == 0) ||
           (strcmp(open, "{") == 0 && strcmp(close, "}") == 0);
}
// Note 32 Pairwise comparison makes the nesting check explicit; alternatives like mapping tables would add complexity without much benefit here.

// Forward declaration.
double parse_expr_eval(int* i, char* argv[], int argc, int* error);

// Variation of parse_term for command line usage.
double parse_term_eval(int* i, char* argv[], int argc, int* error) {
    if (*i >= argc) {
        *error = 1;
        return 0;
    }
    // Note 33 Here the parser walks argv[] manually, so bounds checking prevents reading past the provided command-line tokens.

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
// Note 71 atof tolerates leading plus/minus signs, allowing command-line users to write expressions like "-5 + 3" without extra syntax.
// Note 34 Using atof here accepts the same formatting as strtod; additional validation happens at higher levels where operators are expected between numbers.

// Variation of parse_expr for command line useage.
double parse_expr_eval(int* i, char* argv[], int argc, int* error) {
    double result = parse_term_eval(i, argv, argc, error);
    while (!*error && *i < argc) {
        char* op = argv[*i];
        if (!is_operator(op)) break;
        (*i)++;
    // Note 72 Advancing the index before parsing the RHS mimics consuming a token from a stream, keeping the control flow consistent with pointer-based parsing.
        double rhs = parse_term_eval(i, argv, argc, error);

        if (*error) return 0;
        // Note 81 Early exit keeps error propagation simple: once a subexpression fails, the caller immediately unwinds without mutating accumulated state.

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
// Note 49 Even though the operators are expressed as strings, the arithmetic remains straightforward; this demonstrates how lexical concerns can be kept separate from evaluation logic.
// Note 35 Parsing command-line tokens mirrors the recursive-descent structure with indexes instead of pointers, highlighting how grammar logic can be reused across input modalities.
// Note 36 offDec is toggled when high-precision operations appear, ensuring the formatting logic later honors potential fractional outputs even if prior operands looked like integers.

// Internal token-array based evaluator.
double evaluate(int argc, char* argv[], int* error) {
    *error = 0;
    if (argc < 1) {
        *error = 1;
        return 0;
    }
    // Note 37 The CLI requires at least one operand; empty input is flagged early so the user sees a clear error message instead of undefined behavior later.

    int index = 0;
    double result = parse_expr_eval(&index, argv, argc, error);

    if (index != argc) {
        *error = 1;
        return 0;
    }
    return result;
}
// Note 74 Returning the accumulated result rather than modifying a global variable keeps the function reentrant—two evaluations in a row won't interfere with each other.
// Note 38 Checking that every token was consumed protects against stray arguments, e.g., forgetting an operator at the end, which could otherwise be silently ignored.

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
    // Note 39 The CLI mirrors Unix conventions: no arguments or --help prints documentation, making the tool friendly for shell usage and automated scripts.

    int error;
    int fatalError = 0;
    double result;
    char* expressionForFormat = NULL;
    int expressionNeedsFree = 0;
    // Note 47 fatalError distinguishes allocation failures from syntax errors so we can exit with a non-zero status without printing misleading parser diagnostics.

    // Note 40 These variables buffer the cleaned expression so that formatting decisions later can treat tokenized and quoted inputs uniformly.
    // check if quoted option passed
    if (strcmp(argv[1], "-q") == 0 || strcmp(argv[1], "--quote") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing expression after -q\n");
            return 1;
        }
        // Note 83 Quoted mode lets users supply spaces or traditional '*' and '^' characters without shell tokenization breaking the expression apart.

        // duplicate and clean expression
        char* expr = strdup(argv[2]);
        if (!expr) {
            fprintf(stderr, "Memory error\n");
            return 1;
        }
        // Note 41 Duplicating the string keeps the original argv untouched, which is important when other code might inspect it after evaluation.
        hasDec = 0;
        maxDec = 0;
        offDec = 0;
        remove_format(expr);
    // Note 55 Removing commas and currency symbols mirrors GUI behavior, so command-line usage can accept pasted spreadsheet values without surprises.
        result = evaluate_expr_string(expr, &error);
        if (!error) {
            expressionForFormat = expr;
            expressionNeedsFree = 1;
        } else {
            free(expr);
        }
    } else {
        // regular token-based input
        char** cleaned_args = malloc((argc - 1) * sizeof(char*));
        if (!cleaned_args) {
            fprintf(stderr, "Memory error\n");
            return 1;
        }
        // Note 42 Allocating a new argv array allows the tool to strip commas and currency symbols without mutating the real argv passed by the OS.

        size_t exprLen = 0;
        for (int i = 0; i < argc - 1; ++i) {
            cleaned_args[i] = strdup(argv[i + 1]);
            if (!cleaned_args[i]) {
                fprintf(stderr, "Memory error\n");
                return 1;
            }
            remove_format(cleaned_args[i]);
            exprLen += strlen(cleaned_args[i]);
            // Note 56 Each token is sanitized independently, enabling expressions like "1,000 - 200" where only some inputs carry separators.
        }
        // Note 43 The loop both cleans and measures the expression, collecting enough information to rebuild a human-readable string later.

        hasDec = 0;
        maxDec = 0;
        offDec = 0;
        result = evaluate(argc - 1, cleaned_args, &error);

        if (!error) {
            size_t bufferSize = exprLen + (argc - 2) + 1; // spaces between tokens
            expressionForFormat = malloc(bufferSize);
            if (!expressionForFormat) {
                fprintf(stderr, "Memory error\n");
                error = 1;
                fatalError = 1;
            } else {
                // Note 82 The buffer leaves room for spaces between tokens plus the null terminator, mirroring how join operations work in higher-level languages.
                size_t pos = 0;
                for (int i = 0; i < argc - 1; ++i) {
                    size_t tokLen = strlen(cleaned_args[i]);
                    memcpy(expressionForFormat + pos, cleaned_args[i], tokLen);
                    pos += tokLen;
                    if (i + 1 < argc - 1)
                        expressionForFormat[pos++] = ' ';
                    // Note 57 Rebuilding the string with explicit spaces keeps the final formatting predictable regardless of the spacing in the original command line.
                }
                expressionForFormat[pos] = '\0';
                expressionNeedsFree = 1;
            }
        }
        // Note 44 Reassembling the cleaned tokens into a single string provides the same formatting context that quoted mode enjoys, keeping output consistent.

        for (int i = 0; i < argc - 1; ++i) {
            free(cleaned_args[i]);
        }

        free(cleaned_args);
    }

    if (error) {
        if (expressionNeedsFree)
            free(expressionForFormat);
        if (fatalError)
            return 1;
        printf("Error: Invalid expression\n");
        // Note 58 Reporting parse errors on stdout matches the historical behavior of many calculators, but returning non-zero still allows shell scripts to detect failures.
        return 1;
    }
    // Note 45 From here on we know evaluation succeeded, so the emphasis shifts to presenting the answer with the correct precision.

    char formatted[64];
    if (!expressionForFormat) {
        snprintf(formatted, sizeof(formatted), "%.16g", result);
    } else {
        FormatOutput(expressionForFormat, result, formatted);
    }
    // Note 46 Falling back to %.16g ensures we still return a value if formatting context was unavailable, such as after memory-allocation failures.

    printf("%s\n", formatted);
    // Note 73 Printing the result with a trailing newline lets users pipe the output into other shell commands without additional formatting.

    if (expressionNeedsFree)
        free(expressionForFormat);
    // Note 48 Manual memory management is unavoidable in portable C; freeing only when the pointer was allocated prevents double-free bugs while keeping success path fast.

    return 0;
}
#endif


