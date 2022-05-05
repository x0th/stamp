/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <iostream>

#include "token.h"

using namespace std;

string token_str(Token *token) {
	string s = "";
	switch (token->type) {
#define tok_str(type) case type: s += #type; break;
		tok_str(TokObject)
		tok_str(TokMessage)
		tok_str(TokSend)
		tok_str(TokStore)
		tok_str(TokValue)
		tok_str(TokEOF)
		tok_str(TokStatementEnd)
		tok_str(TokSList)
		tok_str(TokSListBegin)
		tok_str(TokSListEnd)
		tok_str(TokOpenParend)
		tok_str(TokCloseParend)
		tok_str(TokComa)
		tok_str(TokFn)
		tok_str(TokIf)
		tok_str(TokElse)
		tok_str(TokInt)
		tok_str(TokChar)
		tok_str(TokString)
		tok_str(TokSqBracketL)
		tok_str(TokSqBracketR)
		tok_str(TokList)
#undef tok_str
	}
	return s + "(" + token->value + ")";
}

string token_readable(Token *token) {
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
	}
	return "";
}