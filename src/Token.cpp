/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>

#include "Token.h"


std::string token_str(Token *token) {
	std::string s = "";
	switch (token->type) {
#define __TOK_STR(type) case type: s += #type; break;
		ENUMERATE_TOKENS(__TOK_STR)
#undef __TOK_STR
	}
	return s + "(" + token->value + ")";
}

std::string token_readable(Token *token) {
	switch (token->type) {
		case TokObject:
		case TokMessage:
		case TokInt:
		case TokChar:
		case TokString:
		case TokList:
		case TokValue: return token->value;
		case TokSend: return "Send";
		case TokStore: return "=";
		case TokEOF: return "EOF";
		case TokStatementEnd: return ";";
		case TokSList: return "Statement List";
		case TokSListBegin: return "{";
		case TokSListEnd: return "}";
		case TokOpenParend: return "(";
		case TokCloseParend: return ")";
		case TokComa: return ",";
		case TokFn: return "fn";
		case TokIf: return "if";
		case TokElse: return "else";
		case TokSqBracketL: return "[";
		case TokSqBracketR: return "]";
		case TokWhile: return "while";
		case TokProgram: return "Program";
	}
	return "";
}