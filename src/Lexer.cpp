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
#include "Error.h"

char token_image[MAX_TOKEN_LEN];

char scan_char(std::string &raw_string, long unsigned int *position) {
	return raw_string[++(*position)];
}

Token scan(std::string &raw_string, long unsigned int *position, std::string &file, size_t line) {
#define next_char scan_char(raw_string, position)
	int c = raw_string[*position];
	int i = 0;

	while (isspace(c)) c = next_char;

	size_t column = *position;
	if (c == '\0') return Token(Token::Eof, file, line, column);

	bool is_message = false;
	if (c == '.') {
		c = next_char;
		is_message = true;
	}

	switch (c) {
		case ';': next_char; return Token(Token::StatementEnd, file, line, column);
		case '{': next_char; return Token(Token::SListBegin, file, line, column);
		case '}': next_char; return Token(Token::SListEnd, file, line, column);
		case '(': next_char; return Token(Token::OpenParend, file, line, column);
		case ')': next_char; return Token(Token::CloseParend, file, line, column);
		case ',': next_char; return Token(Token::Coma, file, line, column);
		case '[': next_char; return Token(Token::SqBracketL, file, line, column);
		case ']': next_char; return Token(Token::SqBracketR, file, line, column);
		case '^': next_char; return Token(Token::Message, "clone", file, line, column);
		case '%': next_char; return Token(Token::Message, "%", file, line, column);
		case '+': next_char; return Token(Token::Message, "+", file, line, column);
		case '-': next_char; return Token(Token::Message, "-", file, line, column);
		case '/': {
			c = next_char;
			switch (c) {
				case '/': next_char; return Token(Token::SlashSlash, file, line, column);
				case '*': next_char; return Token(Token::SlashStar, file, line, column);
				default: return Token(Token::Message, "/", file, line, column);
			}
		}
		case '*': {
			c = next_char;
			switch (c) {
				case '/': next_char; return Token(Token::StarSlash, file, line, column);
				default: return Token(Token::Message, "*", file, line, column);
			}
		}
		case '<': {
			c = next_char;
			switch (c) {
				case '<': next_char; return Token(Token::Message, "<<", file, line, column);
				case '=': next_char; return Token(Token::Message, "<=", file, line, column);
				default: return Token(Token::Message, "<", file, line, column);
			}
		}
		case '>': {
			c = next_char;
			switch (c) {
				case '>': next_char; return Token(Token::Message, ">>", file, line, column);
				case '=': next_char; return Token(Token::Message, ">=", file, line, column);
				case '<': next_char; return Token(Token::Message, "><", file, line, column);
				default: return Token(Token::Message, ">", file, line, column);
			}
		}
		case '!': {
			c = next_char;
			if (c == '=') {
				next_char; return Token(Token::Message, "!=", file, line, column);
			}
			return Token(Token::Message, "!", file, line, column);
		}
		case '&': {
			c = next_char;
			if (c == '&') {
				next_char; return Token(Token::Message, "&&", file, line, column);
			}
			return Token(Token::Message, "&", file, line, column);
		}
		case '|': {
			c = next_char;
			if (c == '=') {
				next_char; return Token(Token::Message, "||", file, line, column);
			}
			return Token(Token::Message, "|", file, line, column);
		}
		case '=': {
			c = next_char;
			if (c == '=') {
				next_char;
				return Token(Token::Message, "==", file, line, column);
			}
			return Token(Token::Store, file, line, column);
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
			auto tok =  Token(Token::Char, std::string{ch}, file, line, column);
			c = next_char;
			next_char;
			if (c != '\'')
				// FIXME: change to hinting error when we implement error recovery
				terminating_error(StampError::LexingError, file + ":" + std::to_string(line) + ":" + std::to_string(column) + ": Length of character is not valid.");
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
					// FIXME: change to hinting error when we implement error recovery
					terminating_error(StampError::LexingError, file + ":" + std::to_string(line) + ":" + std::to_string(column) + ": Maximum token length exceeded.");
				c = next_char;
			} while (c != '\"');
			token_image[i] = '\0';
			next_char;

			return Token(Token::String, token_image, file, line, column);
		}
		default: {
			if (isdigit(c)) {
				do {
					token_image[i++] = c;
					if (i >= MAX_TOKEN_LEN)
						// FIXME: change to hinting error when we implement error recovery
						terminating_error(StampError::LexingError, file + ":" + std::to_string(line) + ":" + std::to_string(column) + ": Maximum token length exceeded.");
					c = next_char;
				} while (isdigit(c));
				token_image[i] = '\0';
				return Token(Token::Int, token_image, file, line, column);
			} else {
				do {
					token_image[i++] = c;
					if (i >= MAX_TOKEN_LEN)
						// FIXME: change to hinting error when we implement error recovery
						terminating_error(StampError::LexingError, file + ":" + std::to_string(line) + ":" + std::to_string(column) + ": Maximum token length exceeded.");
					c = next_char;
				} while (isalpha(c) || isdigit(c) || c == '_');
				token_image[i] = '\0';

				if (token_image == std::string("fn")) return Token(Token::Fn, file, line, column);
				if (token_image == std::string("if")) return Token(Token::If, file, line, column);
				if (token_image == std::string("else")) return Token(Token::Else, file, line, column);
				if (token_image == std::string("while")) return Token(Token::While, file, line, column);
				if (token_image == std::string("break")) return Token(Token::Break, file, line, column);
				if (token_image == std::string("continue")) return Token(Token::Continue, file, line, column);
				if (token_image == std::string("mut")) return Token(Token::Mut, file, line, column);
				if (token_image[0] >= 65 && token_image[0] <= 90) return Token(Token::Object, token_image, file, line, column);
				return Token(is_message ? Token::Message : Token::Value, token_image, file, line, column);
			}
		}
	}
#undef next_char
}