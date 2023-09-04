#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int parseExpression(const char **input);
int parseTerm(const char **input);
int parseFactor(const char **input);

// Helper function to check if a character is a valid variable name
int isVariable(char c) {
    return isupper(c);
}

// Function to parse expressions
int parseExpression(const char **input) {
    int result = parseTerm(input);

    while (**input == '+' || **input == '-') {
        char op = *(*input)++;
        
        int term = parseTerm(input);

        if (op == '+') {
            result += term;
        } else {
            result -= term;
        }
    }

    return result;
}

// Function to parse terms
int parseTerm(const char **input) {
    int result = parseFactor(input);

    while (**input == '*' || **input == '/') {
        char op = *(*input)++;

        int factor = parseFactor(input);

        if (op == '*') {
            result *= factor;
        } else {
            if (factor == 0) {
                fprintf(stderr, "Error: Division by zero\n");
                exit(1);
            }
            result /= factor;
        }
    }

    return result;
}

// Function to parse factors
int parseFactor(const char **input) {
    if (isVariable(**input)) {
        (*input)++; // Consume the variable name
        return 0;   // Return 0 for variables without values
    } else if (**input == '(') {
        (*input)++; // Consume '('
        int result = parseExpression(input);

        if (**input == ')') {
            (*input)++; // Consume ')'
            return result;
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis\n");
            exit(1);
        }
    } else if (isdigit(**input)) {
        int result = 0;
        while (isdigit(**input)) {
            result = result * 10 + (**input - '0');
            (*input)++;
        }
        return result;
    } else {
        fprintf(stderr, "Error: Invalid input\n");
        exit(1);
    }
}

int main() {
    const char *input = "a+(b*c)";
    
    int result = parseExpression(&input);
    
    if (*input == '\0') {
        printf("Parsing successful\n");
    } else {
        fprintf(stderr, "Error: Invalid input\n");
        return 1;
    }
    
    return 0;
}
