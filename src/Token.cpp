/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>

#include "Token.h"


std::string Token::token_str() const {
	std::string s = "";
	switch (type) {
#define __TOK_STR(type) case type: s += #type; break;
		ENUMERATE_TOKENS(__TOK_STR)
#undef __TOK_STR
	}
	return s + "(" + value + ")";
}

std::string Token::token_readable() const {
	switch (type) {
		case Object:
		case Message:
		case Int:
		case Char:
		case String:
		case List:
		case Value: return value;
		case Send: return "Send";
		case Store: return "=";
		case Eof: return "EOF";
		case StatementEnd: return ";";
		case SList: return "Statement List";
		case SListBegin: return "{";
		case SListEnd: return "}";
		case OpenParend: return "(";
		case CloseParend: return ")";
		case Coma: return ",";
		case Fn: return "fn";
		case If: return "if";
		case Else: return "else";
		case SqBracketL: return "[";
		case SqBracketR: return "]";
		case While: return "while";
		case Program: return "Program";
		case FnCall: return "Function call";
		case SlashSlash: return "Comment";
		case SlashStar: return "Multiline comment begin";
		case StarSlash: return "Multiline comment end";
		case Break: return "break";
		case Continue: return "continue";
		case Mut: return "mut";
		case Vec: return "Vec";
	}
	return "";
}

std::string Token::position() const {
	if (file != "")
		return file + ":" + std::to_string(line) + ":" + std::to_string(column);
	return std::to_string(line) + ":" + std::to_string(column);
}