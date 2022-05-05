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
		case TokObject: s += "TokObject"; break;
		case TokMessage: s += "TokMessage"; break;
		case TokSend: s += "TokSend"; break;
		case TokStore: s += "TokStore"; break;
		case TokValue: s += "TokValue"; break;
		case TokEOF: s += "TokEOF"; break;
		case TokStatementEnd: s += "TokStatementEnd"; break;
		case TokSList: s += "TokSList"; break;
		case TokSListBegin: s += "TokSListBegin"; break;
		case TokSListEnd: s += "TokSListEnd"; break;
		case TokOpenParend: s += "TokOpenParend"; break;
		case TokCloseParend: s += "TokCloseParend"; break;
		case TokComa: s += "TokComa"; break;
		case TokFn: s += "TokFn"; break;
		case TokIf: s += "TokIf"; break;
		case TokElse: s += "TokElse"; break;
		case TokInt: s += "TokInt"; break;
	}
	return s + "(" + token->value + ")";
}

string token_readable(Token *token) {
	switch (token->type) {
		case TokObject: return token->value;
		case TokMessage: return token->value;
		case TokSend: return "Send";
		case TokStore: return "=";
		case TokValue: return token->value;
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
		case TokInt: return token->value;
	}
	return "";
}