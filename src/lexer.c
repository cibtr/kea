#include <pcre2posix.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "keac.h"
#include "lexer.h"

#define IDENTIFIER_LENGTH 255

static void next_word(Lexer *this);
static bool is_repeatable(char c);
static bool is_identifier_part(char c);

static void identify_token(Lexer *this);

static void regex_compile(regex_t *dest, const char *src, int cflags);
static bool regex_match(const char *src, regex_t *regex);

static void add_token(Lexer *this, TokenType type, uint64_t line);

typedef struct lexer {
	SymbolTable *table;

	const char *file;
	const char *src;
	size_t src_length;
	uint64_t index;

	uint64_t line, column;

	char lexeme[IDENTIFIER_LENGTH + 1]; // + 1 for the null character
	regex_t valid_int_reg, invalid_int_reg, identifier_reg;

	Token *token;
} Lexer;


Lexer *lexer_create(const char *src, const char *file_name, SymbolTable *table)
{
	Lexer *this = malloc(sizeof(Lexer));

	this->table = table;

	this->file = file_name;
	this->src = src;
	this->src_length = strlen(src);
	this->index = 0;

	this->line = 1;
	this->column = 0;

	this->token = malloc(sizeof(Token));

	// Compiling the regex that will be used before lexing to improve performance and reduce memory usage
	regex_compile(&this->valid_int_reg, "^[0-9]{1,20}$", REG_EXTENDED);
	regex_compile(&this->invalid_int_reg, "^[0-9]{1,20}.*$", 0);
	regex_compile(&this->identifier_reg, "^[A-Za-z_$][A-Za-z_$0-9]{0,254}$", REG_EXTENDED);

	return this;
}

void lexer_free(Lexer *this)
{
	pcre2_regfree(&this->valid_int_reg);
	pcre2_regfree(&this->invalid_int_reg);
	pcre2_regfree(&this->identifier_reg);

	add_token(this, TOKEN_EOF, 0);

	free(this->token);

	free(this);
}

Token *lexer_next_token(Lexer *this)
{
	next_word(this);
	identify_token(this);
	printf("%s ", lexer_str_token(this->token->type));

	return this->token;
}

void next_word(Lexer *this)
{
	memset(this->lexeme, 0, IDENTIFIER_LENGTH + 1);

	char current;

	// Skip whitespace
	while (isspace((current = this->src[this->index]))) {
		if (current == '\n') {
			++this->line;
			this->column = 0;

			putchar('\n');
		}
		else if (current == '\t') {
			putchar('\t');
		}
		++this->index;
	}

	for (uint32_t lexeme_index = 0; !isspace((current = this->src[this->index])); ++lexeme_index) {
		++this->column;

		if (lexeme_index == IDENTIFIER_LENGTH - 1) {
			keac_error(this->file, this->line, this->column, "identifier length exceeds limit of %d\n", IDENTIFIER_LENGTH);
			exit(EXIT_FAILURE);
		}

		if (current == 0)
			return;

		if (is_identifier_part(current)) {
			this->lexeme[lexeme_index] = current;
			++this->index;
			continue;
		} 

		if (lexeme_index == 0) {
			this->lexeme[lexeme_index++] = current;

			if (is_repeatable(current)) {
				++this->index;
				char current_peek = this->src[this->index++];
				if (!isspace(current_peek)) {
					if (current_peek == 0)
						return;

					if (current == '+') {
						if (current_peek == '+' || current_peek == '=') {
							this->lexeme[lexeme_index] = current_peek;
							return;
						}
					}
					else if (current == '-') {
						if (current_peek == '-' || current_peek == '=' || current_peek == '>') {
							this->lexeme[lexeme_index] = current_peek;
							return;
						}
					}
					else if (current == '&') {
						if (current_peek == '&' || current_peek == '=') {
							this->lexeme[lexeme_index] = current_peek;
							return;
						}
					}
					else if (current == '|') {
						if (current_peek == '|' || current_peek == '=') {
							this->lexeme[lexeme_index] = current_peek;
							return;
						}
					}
					else if (current == '=' || current == '*' || current == '/' || current == '%' ||
					         current == '!' || current == '^') {
						if (current_peek == '=') {
							this->lexeme[lexeme_index] = current_peek;
							return;
						}
					}
					else if (current == '<') {
						if (current_peek == '<' || current_peek == '=') {
							this->lexeme[lexeme_index++] = current_peek;

							char newercurr = this->src[this->index++];
							if (newercurr == 0)
								return;

							if (current_peek == '<' && newercurr == '=') {
								this->lexeme[lexeme_index] = newercurr;
							}

							return;
						}
					}

					--this->index;
				}

				return;
			}

			break;
		}

		return;
	}

	++this->index;
}

bool is_identifier_part(char c)
{
	if (isalnum(c) || c == '_' || c == '$')
		return true;
	else
		return false;
}

bool is_repeatable(char c)
{
	if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '%' ||
	    c == '|' || c == '&' || c == '^' || c == '<' || c == '>' || c == '!')
		return true;
	
	return false;
}

void identify_token(Lexer *this)
{
	if (this->lexeme[0] == '(')                     add_token(this, TOKEN_LEFT_PAREN, this->line);
	else if (this->lexeme[0] == ')')                add_token(this, TOKEN_RIGHT_PAREN, this->line);
	else if (this->lexeme[0] == '[')                add_token(this, TOKEN_LEFT_BRACKET, this->line);
	else if (this->lexeme[0] == ']')                add_token(this, TOKEN_RIGHT_BRACKET, this->line);
	else if (this->lexeme[0] == '{')                add_token(this, TOKEN_LEFT_BRACE, this->line);
	else if (this->lexeme[0] == '}')                add_token(this, TOKEN_RIGHT_BRACE, this->line);
	else if (this->lexeme[0] == ',')                add_token(this, TOKEN_COMMA, this->line);
	else if (this->lexeme[0] == ';')                add_token(this, TOKEN_SEMICOLON, this->line);
	else if (this->lexeme[0] == '~')                add_token(this, TOKEN_NOT, this->line);
	else if (strcmp(this->lexeme, "=") == 0)        add_token(this, TOKEN_ASSIGN, this->line);
	else if (strcmp(this->lexeme, "+") == 0)        add_token(this, TOKEN_PLUS, this->line);
	else if (strcmp(this->lexeme, "-") == 0)        add_token(this, TOKEN_MINUS, this->line);
	else if (strcmp(this->lexeme, "*") == 0)        add_token(this, TOKEN_ASTERISK, this->line);
	else if (strcmp(this->lexeme, "/") == 0)        add_token(this, TOKEN_DIVIDE, this->line);
	else if (strcmp(this->lexeme, "%") == 0)        add_token(this, TOKEN_MODULO, this->line);
	else if (strcmp(this->lexeme, "<") == 0)        add_token(this, TOKEN_LESS_THAN, this->line);
	else if (strcmp(this->lexeme, ">") == 0)        add_token(this, TOKEN_GREATER_THAN, this->line);
	else if (strcmp(this->lexeme, "!") == 0)        add_token(this, TOKEN_GREATER_THAN, this->line);
	else if (strcmp(this->lexeme, "&") == 0)        add_token(this, TOKEN_AND, this->line);
	else if (strcmp(this->lexeme, "|") == 0)        add_token(this, TOKEN_OR, this->line);
	else if (strcmp(this->lexeme, "^") == 0)        add_token(this, TOKEN_XOR, this->line);
	else if (strcmp(this->lexeme, ":") == 0)        add_token(this, TOKEN_COLON, this->line);
	else if (strcmp(this->lexeme, "==") == 0)       add_token(this, TOKEN_EQUAL, this->line);
	else if (strcmp(this->lexeme, "!=") == 0)       add_token(this, TOKEN_NOT_EQUAL, this->line);
	else if (strcmp(this->lexeme, "<=") == 0)       add_token(this, TOKEN_LESS_EQUAL_THAN, this->line);
	else if (strcmp(this->lexeme, ">=") == 0)       add_token(this, TOKEN_GREATER_EQUAL_THAN, this->line);
	else if (strcmp(this->lexeme, "&&") == 0)       add_token(this, TOKEN_AND_AND, this->line);
	else if (strcmp(this->lexeme, "&=") == 0)       add_token(this, TOKEN_AND_EQUALS, this->line);
	else if (strcmp(this->lexeme, "||") == 0)       add_token(this, TOKEN_OR_OR, this->line);
	else if (strcmp(this->lexeme, "|=") == 0)       add_token(this, TOKEN_OR_EQUALS, this->line);
	else if (strcmp(this->lexeme, "++") == 0)       add_token(this, TOKEN_INCREMENT, this->line);
	else if (strcmp(this->lexeme, "--") == 0)       add_token(this, TOKEN_DECREMENT, this->line);
	else if (strcmp(this->lexeme, "/=") == 0)       add_token(this, TOKEN_DIVIDE_EQUALS, this->line);
	else if (strcmp(this->lexeme, "*=") == 0)       add_token(this, TOKEN_TIMES_EQUALS, this->line);
	else if (strcmp(this->lexeme, "+=") == 0)       add_token(this, TOKEN_PLUS_EQUALS, this->line);
	else if (strcmp(this->lexeme, "-=") == 0)       add_token(this, TOKEN_MINUS_EQUALS, this->line);
	else if (strcmp(this->lexeme, "%=") == 0)       add_token(this, TOKEN_MODULO_EQUALS, this->line);
	else if (strcmp(this->lexeme, "->") == 0)       add_token(this, TOKEN_ARROW, this->line);
	else if (strcmp(this->lexeme, "<<") == 0)       add_token(this, TOKEN_BITSHIFT_LEFT, this->line);
	else if (strcmp(this->lexeme, ">>") == 0)       add_token(this, TOKEN_BITSHIFT_RIGHT, this->line);
	else if (strcmp(this->lexeme, "if") == 0)       add_token(this, TOKEN_IF, this->line);
	else if (strcmp(this->lexeme, "i8") == 0)       add_token(this, TOKEN_I8, this->line);
	else if (strcmp(this->lexeme, "i16") == 0)      add_token(this, TOKEN_I16, this->line);
	else if (strcmp(this->lexeme, "i32") == 0)      add_token(this, TOKEN_I32, this->line);
	else if (strcmp(this->lexeme, "i64") == 0)      add_token(this, TOKEN_I64, this->line);
	else if (strcmp(this->lexeme, "<<=") == 0)      add_token(this, TOKEN_BITSHIFT_LEFT_EQUALS, this->line);
	else if (strcmp(this->lexeme, ">>=") == 0)      add_token(this, TOKEN_BITSHIFT_RIGHT_EQUALS, this->line);
	else if (strcmp(this->lexeme, "ui8") == 0)      add_token(this, TOKEN_UI8, this->line);
	else if (strcmp(this->lexeme, "for") == 0)      add_token(this, TOKEN_FOR, this->line);
	else if (strcmp(this->lexeme, "var") == 0)      add_token(this, TOKEN_VAR, this->line);
	else if (strcmp(this->lexeme, "ui16") == 0)     add_token(this, TOKEN_UI16, this->line);
	else if (strcmp(this->lexeme, "ui32") == 0)     add_token(this, TOKEN_UI32, this->line);
	else if (strcmp(this->lexeme, "ui64") == 0)     add_token(this, TOKEN_UI64, this->line);
	else if (strcmp(this->lexeme, "void") == 0)     add_token(this, TOKEN_VOID, this->line);
	else if (strcmp(this->lexeme, "bool") == 0)     add_token(this, TOKEN_BOOL, this->line);
	else if (strcmp(this->lexeme, "true") == 0)     add_token(this, TOKEN_TRUE, this->line);
	else if (strcmp(this->lexeme, "case") == 0)     add_token(this, TOKEN_CASE, this->line);
	else if (strcmp(this->lexeme, "else") == 0)     add_token(this, TOKEN_ELSE, this->line);
	else if (strcmp(this->lexeme, "false") == 0)    add_token(this, TOKEN_FALSE, this->line);
	else if (strcmp(this->lexeme, "while") == 0)    add_token(this, TOKEN_WHILE, this->line);
	else if (strcmp(this->lexeme, "break") == 0)    add_token(this, TOKEN_BREAK, this->line);
	else if (strcmp(this->lexeme, "switch") == 0)   add_token(this, TOKEN_SWITCH, this->line);
	else if (strcmp(this->lexeme, "return") == 0)   add_token(this, TOKEN_RETURN, this->line);
	else if (strcmp(this->lexeme, "default") == 0)  add_token(this, TOKEN_DEFAULT, this->line);
	else if (strcmp(this->lexeme, "continue") == 0) add_token(this, TOKEN_CONTINUE, this->line);
	else if (this->lexeme[0] == 0)                  add_token(this, TOKEN_EOF, 0);
	else if (regex_match(this->lexeme, &this->valid_int_reg))
		add_token(this, TOKEN_INT_LITERAL, this->line);
	else if (regex_match(this->lexeme, &this->invalid_int_reg)) {
		keac_error(this->file, this->line, this->column, "invalid integer literal: %s\n", this->lexeme);
		exit(EXIT_FAILURE);
	}
	else if (regex_match(this->lexeme, &this->identifier_reg))
		add_token(this, TOKEN_IDENTIFIER, this->line);
	else {
		keac_error(this->file, this->line, this->column, "unknown lexeme: %s\n", this->lexeme);
		
		exit(EXIT_FAILURE);
	}
}

void add_token(Lexer *this, TokenType type, uint64_t line)
{
	this->token->type = type;

	Symbol *value = NULL;
	if (type == TOKEN_IDENTIFIER || type == TOKEN_INT_LITERAL || type == TOKEN_STRING_LITERAL) {
		if ((value = st_get_symbol(this->table, this->lexeme)) == NULL)
			value = st_add_symbol(this->table, this->lexeme);
	}
	this->token->value = value;

	this->token->line = line;
}

void regex_compile(regex_t *dest, const char *src, int cflags)
{
	if (pcre2_regcomp(dest, src, cflags) != 0) {
		fprintf(stderr, "internal error: regcomp failed\n");
		exit(EXIT_FAILURE);
	}
}

bool regex_match(const char *src, regex_t *regex)
{
	if (pcre2_regexec(regex, src, 0, NULL, 0) == 0)
		return true;
	else
		return false;
}

const char *lexer_str_token(TokenType token)
{
	switch (token) {
		case TOKEN_I8:                    return "I8";
		case TOKEN_IF:                    return "IF";
		case TOKEN_OR:                    return "OR";
		case TOKEN_AND:                   return "AND";
		case TOKEN_EOF:                   return "EOF";
		case TOKEN_FOR:                   return "FOR";
		case TOKEN_I16:                   return "I16";
		case TOKEN_I32:                   return "I32";
		case TOKEN_I64:                   return "I64";
		case TOKEN_NOT:                   return "NOT";
		case TOKEN_UI8:                   return "UI8";
		case TOKEN_VAR:                   return "VAR";
		case TOKEN_XOR:                   return "XOR";
		case TOKEN_BANG:                  return "BANG";
		case TOKEN_BOOL:                  return "BOOL";
		case TOKEN_CASE:                  return "CASE";
		case TOKEN_ELSE:                  return "ELSE";
		case TOKEN_PLUS:                  return "PLUS";
		case TOKEN_TRUE:                  return "TRUE";
		case TOKEN_UI16:                  return "UI16";
		case TOKEN_UI32:                  return "UI32";
		case TOKEN_UI64:                  return "UI64";
		case TOKEN_VOID:                  return "VOID";
		case TOKEN_ARROW:                 return "ARROW";
		case TOKEN_BREAK:                 return "BREAK";
		case TOKEN_COLON:                 return "COLON";
		case TOKEN_COMMA:                 return "COMMA";
		case TOKEN_EQUAL:                 return "EQUAL";
		case TOKEN_FALSE:                 return "FALSE";
		case TOKEN_MINUS:                 return "MINUS";
		case TOKEN_OR_OR:                 return "OR_OR";
		case TOKEN_WHILE:                 return "WHILE";
		case TOKEN_ASSIGN:                return "ASSIGN";
		case TOKEN_DIVIDE:                return "DIVIDE";
		case TOKEN_MODULO:                return "MODULO";
		case TOKEN_RETURN:                return "RETURN";
		case TOKEN_SWITCH:                return "SWITCH";
		case TOKEN_AND_AND:               return "AND_AND";
		case TOKEN_DEFAULT:               return "DEFAULT";
		case TOKEN_ASTERISK:              return "ASTERISK";
		case TOKEN_CONTINUE:              return "CONTINUE";
		case TOKEN_DECREMENT:             return "DECREMENT";
		case TOKEN_INCREMENT:             return "INCREMENT";
		case TOKEN_LESS_THAN:             return "LESS_THAN";
		case TOKEN_NOT_EQUAL:             return "NOT_EQUAL";
		case TOKEN_OR_EQUALS:             return "OR_EQUALS";
		case TOKEN_SEMICOLON:             return "SEMICOLON";
		case TOKEN_AND_EQUALS:            return "AND_EQUALS";
		case TOKEN_IDENTIFIER:            return "IDENTIFIER";
		case TOKEN_LEFT_BRACE:            return "LEFT_BRACE";
		case TOKEN_LEFT_PAREN:            return "LEFT_PAREN";
		case TOKEN_INT_LITERAL:           return "INT_LITERAL";
		case TOKEN_PLUS_EQUALS:           return "PLUS_EQUALS";
		case TOKEN_RIGHT_BRACE:           return "RIGHT_BRACE";
		case TOKEN_RIGHT_PAREN:           return "RIGHT_PAREN";
		case TOKEN_GREATER_THAN:          return "GREATER_THAN";
		case TOKEN_LEFT_BRACKET:          return "LEFT_BRACKET";
		case TOKEN_MINUS_EQUALS:          return "MINUS_EQUALS";
		case TOKEN_TIMES_EQUALS:          return "TIMES_EQUALS";
		case TOKEN_BITSHIFT_LEFT:         return "BITSHIFT_LEFT";
		case TOKEN_DIVIDE_EQUALS:         return "DIVIDE_EQUALS";
		case TOKEN_MODULO_EQUALS:         return "MODULO_EQUALS";
		case TOKEN_RIGHT_BRACKET:         return "RIGHT_BRACKET";
		case TOKEN_BITSHIFT_RIGHT:        return "BITSHIFT_RIGHT";
		case TOKEN_STRING_LITERAL:        return "STRING_LITERAL";
		case TOKEN_LESS_EQUAL_THAN:       return "LESS_EQUAL_THAN";
		case TOKEN_CHARACTER_LITERAL:     return "CHARACTER_LITERAL";
		case TOKEN_GREATER_EQUAL_THAN:    return "GREATER_EQUAL_THAN";
		case TOKEN_BITSHIFT_LEFT_EQUALS:  return "BITSHIFT_LEFT_EQUALS";
		case TOKEN_BITSHIFT_RIGHT_EQUALS: return "BITSHIFT_RIGHT_EQUALS";
		default:
			return "";
	}
}
