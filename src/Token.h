/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>

#define ENUMERATE_TOKENS(T) \
	T(Object) \
	T(Message) \
	T(Send) \
	T(Store) \
	T(Value) \
	T(Eof) \
	T(StatementEnd) \
	T(SList) \
	T(SListBegin) \
	T(SListEnd) \
	T(OpenParend) \
	T(CloseParend) \
	T(Coma) \
	T(Fn) \
	T(If) \
	T(Else) \
	T(Int) \
	T(Char) \
	T(String) \
	T(SqBracketL) \
	T(SqBracketR) \
	T(List) \
	T(While) \
	T(Program) \
	T(FnCall) \
	T(SlashSlash) \
	T(SlashStar) \
	T(StarSlash) \
	T(Break) \
	T(Continue) \
	T(Mut) \
	T(Vec)

class Token
{
public:
	enum TokenType {
#define __DEFINE_TOKEN_TYPES(t) \
	t,
		ENUMERATE_TOKENS(__DEFINE_TOKEN_TYPES)
#undef __DEFINE_TOKEN_TYPES
	};

	Token(TokenType type, std::string value, std::string file, size_t line, size_t column)
		: type(type), value(value), file(file), line(line), column(column) {}
	Token(TokenType type, std::string file, size_t line, size_t column)
		: type(type), file(file), line(line), column(column) {}

	std::string token_str() const;
	std::string token_readable() const;
	std::string position() const;

	TokenType type;
	std::string value;
	std::string file;
	size_t line;
	size_t column;
};