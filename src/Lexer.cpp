/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cctype>

#include "Lexer.h"
#include "Token.h"
#include "Parser.h"

using namespace std;

char token_image[MAX_TOKEN_LEN];

char scan_char(string &raw_string, long unsigned int *position) {
	return raw_string[++(*position)];
}

Token scan(string &raw_string, long unsigned int *position) {
#define next_char scan_char(raw_string, position)
	int c = raw_string[*position];
	int i = 0;

	while (isspace(c)) c = next_char;

	if (c == '\0') return Token({ type: TokEOF, value: "" });

	bool is_message = false;
	if (c == '.') {
		c = next_char;
		is_message = true;
	}

	switch (c) {
		case ';': next_char; return Token({ type: TokStatementEnd, value: ""});
		case '{': next_char; return Token({ type: TokSListBegin, value: ""});
		case '}': next_char; return Token({ type: TokSListEnd, value: ""});
		case '(': next_char; return Token({ type: TokOpenParend, value: ""});
		case ')': next_char; return Token({ type: TokCloseParend, value: ""});
		case ',': next_char; return Token({ type: TokComa, value: ""});
		case '[': next_char; return Token({ type: TokSqBracketL, value: ""});
		case ']': next_char; return Token({ type: TokSqBracketR, value: ""});
		case '^': next_char; return Token({ type: TokMessage, value: "clone"});
		case '%': next_char; return Token({ type: TokMessage, value: "%" });
		case '+': next_char; return Token({ type: TokMessage, value: "+" });
		case '-': next_char; return Token({ type: TokMessage, value: "-" });
		case '/': {
			c = next_char;
			switch (c) {
				case '/': next_char; return Token({ type: TokSlashSlash, value: "" });
				case '*': next_char; return Token({ type: TokSlashStar, value: "" });
				default: return Token({ type: TokMessage, value: "/" });
			}
		}
		case '*': {
			c = next_char;
			switch (c) {
				case '/': next_char; return Token({ type: TokStarSlash, value: "" });
				default: return Token({ type: TokMessage, value: "*" });
			}
		}
		case '<': {
			c = next_char;
			switch (c) {
				case '<': next_char; return Token({ type: TokMessage, value: "<<"});
				case '=': next_char; return Token({ type: TokMessage, value: "<=" });
				default: return Token({ type: TokMessage, value: "<"});
			}
		}
		case '>': {
			c = next_char;
			switch (c) {
				case '>': next_char; return Token({ type: TokMessage, value: ">" });
				case '=': next_char; return Token({ type: TokMessage, value: ">=" });
				case '<': next_char; return Token({ type: TokMessage, value: "><" });
				default: return Token({ type: TokMessage, value: ">" });
			}
		}
		case '!': {
			c = next_char;
			if (c == '=') {
				next_char; return Token({ type: TokMessage, value: "!=" });
			}
			return Token({ type: TokMessage, value: "!" });
		}
		case '&': {
			c = next_char;
			if (c == '&') {
				next_char; return Token({ type: TokMessage, value: "&&" });
			}
			return Token({ type: TokMessage, value: "&" });
		}
		case '|': {
			c = next_char;
			if (c == '=') {
				next_char; return Token({ type: TokMessage, value: "||" });
			}
			return Token({ type: TokMessage, value: "|" });
		}
		case '=': {
			c = next_char;
			if (c == '=') {
				next_char;
				return Token({type: TokMessage, value: "=="});
			}
			return Token({type: TokStore, value: ""});
		}
		case '\'': {
			char ch = next_char;
			// (Most) escape sequences from https://en.wikipedia.org/wiki/Escape_sequences_in_C
			if (ch == '\\') {
				char esc = next_char;
				switch (esc) {
					case 'a': ch = '\a'; break;
					case 'b': ch = '\b'; break;
					case 'e': ch = static_cast<char>(0x1b); break;
					case 'f': ch = '\f'; break;
					case 'n': ch = '\n'; break;
					case 'r': ch = '\r'; break;
					case 't': ch = '\t'; break;
					case 'v': ch = '\v'; break;
					case '\\': ch = '\\'; break;
					case '\'': ch = '\''; break;
					case '\"': ch = '\"'; break;
					case '\?': ch = '\?'; break;
					case 'x': {
						// FIXME: Only reads "proper" characters (of hex length 2), maybe should support lengths 1, 3 and beyond?
						char num[] = {next_char, next_char};
						std::stringstream s;
						int hex;
						s << std::hex << num;
						s >> hex;
						ch = static_cast<char>(hex);
						break;
					}
					// FIXME: Unicode (\u) support?
					default: ch = esc; break;
				}
			}
			auto tok = Token({ type: TokChar, value: string{ch} });
			c = next_char;
			next_char;
			if (c != '\'')
				throw error_msg("Length of character is not valid.");
			return tok;
		}
		case '\"': {
			c = next_char;
			do {
				if (c == '\\') {
					char esc = next_char;
					switch (esc) {
						case 'a': c = '\a'; break;
						case 'b': c = '\b'; break;
						case 'e': c = static_cast<char>(0x1b); break;
						case 'f': c = '\f'; break;
						case 'n': c = '\n'; break;
						case 'r': c = '\r'; break;
						case 't': c = '\t'; break;
						case 'v': c = '\v'; break;
						case '\\': c = '\\'; break;
						case '\'': c = '\''; break;
						case '\"': c = '\"'; break;
						case '\?': c = '\?'; break;
						case 'x': {
							// FIXME: Only reads "proper" characters (of hex length 2), maybe should support lengths 1, 3 and beyond?
							char num[] = {next_char, next_char};
							std::stringstream s;
							int hex;
							s << std::hex << num;
							s >> hex;
							c = static_cast<char>(hex);
							break;
						}
							// FIXME: Unicode (\u) support?
						default: c = esc; break;
					}
				}
				token_image[i++] = c;
				if (i >= MAX_TOKEN_LEN)
					throw error_msg("Maximum token length exceeded.");
				c = next_char;
			} while (c != '\"');
			token_image[i] = '\0';
			next_char;

			return Token({ type: TokString, value: token_image });
		}
		default: {
			if (isdigit(c)) {
				do {
					token_image[i++] = c;
					if (i >= MAX_TOKEN_LEN)
						throw error_msg("Maximum token length exceeded.");
					c = next_char;
				} while (isdigit(c));
				token_image[i] = '\0';
				return Token({ type: TokInt, value: token_image });
			} else {
				do {
					token_image[i++] = c;
					if (i >= MAX_TOKEN_LEN)
						throw error_msg("Maximum token length exceeded.");
					c = next_char;
				} while (isalpha(c) || isdigit(c) || c == '_');
				token_image[i] = '\0';

				if (token_image == string("fn")) return Token({ type: TokFn, value: "" });
				if (token_image == string("if")) return Token({ type: TokIf, value: "" });
				if (token_image == string("else")) return Token({ type: TokElse, value: "" });
				if (token_image == string("while")) return Token({ type: TokWhile, value: "" });
				if (token_image == string("break")) return Token({ type: TokBreak, value: "" });
				if (token_image == string("continue")) return Token({ type: TokContinue, value: "" });
				if (token_image[0] >= 65 && token_image[0] <= 90) return Token({ type: TokObject, value: token_image });
				return is_message ? Token({ type: TokMessage, value: token_image }) : Token({ type: TokValue, value: token_image });
			}
		}
	}
#undef next_char
}