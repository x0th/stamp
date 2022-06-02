/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>
#include <string>
#include <optional>

#include "token.h"
//#include "object.h"
//#include "message.h"
#include "Generator.h"

class Context;

class ASTNode {
public:
	ASTNode(Token token) : token(token) {}

	ASTNode(Token token, std::vector<ASTNode *> children) : token(token), children(children) {}

	Token token;

	std::optional<Register> generate_bytecode(Generator& generator);

	std::vector<ASTNode *> &get_children() { return children; }
	std::string to_string() { return to_string_indent(""); };

	std::string visit();
//	Object *visit_statement(string *out, Context *context, bool *should_return, Object *external_object);
//	Object *visit_object(string *out, Context *context);
//	Message visit_message();
//	Object *visit_send(string *out, Context *context, bool *should_return, Object *external_object);
//	Object *visit_store(Context *context, Object *external_object);
private:
	std::vector<ASTNode *> children;

	std::string to_string_indent(std::string indent);
};