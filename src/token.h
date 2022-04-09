/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>

enum TokenType {
	TokObject,
	TokMessage,
	TokSend,
	TokStore,
	TokValue,
	TokEOF,
	TokStatementEnd,
	TokSList,
	TokSListBegin,
	TokSListEnd,
	TokOpenParend,
	TokCloseParend,
	TokComa,
	TokFn,
	TokIf,
	TokElse,
};

struct Token
{
	TokenType type;
	std::string value;
};

std::string token_str(Token *token);
std::string token_readable(Token *token);