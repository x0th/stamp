/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>

#define ENUMERATE_TOKENS(T) \
	T(TokObject) \
	T(TokMessage) \
	T(TokSend) \
	T(TokStore) \
	T(TokValue) \
	T(TokEOF) \
	T(TokStatementEnd) \
	T(TokSList) \
	T(TokSListBegin) \
	T(TokSListEnd) \
	T(TokOpenParend) \
	T(TokCloseParend) \
	T(TokComa) \
	T(TokFn) \
	T(TokIf) \
	T(TokElse) \
	T(TokInt) \
	T(TokChar) \
	T(TokString) \
	T(TokSqBracketL) \
	T(TokSqBracketR) \
	T(TokList) \
	T(TokWhile) \
	T(TokProgram) \
	T(TokFnCall) \
	T(TokSlashSlash) \
	T(TokSlashStar) \
	T(TokStarSlash) \
	T(TokBreak) \
	T(TokContinue) \
	T(TokMut)

enum TokenType {
#define __DEFINE_TOKEN_TYPES(t) \
	t,
			ENUMERATE_TOKENS(__DEFINE_TOKEN_TYPES)
#undef __DEFINE_TOKEN_TYPES
};

struct Token
{
	TokenType type;
	std::string value;
};

std::string token_str(Token *token);
std::string token_readable(Token *token);