/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>
#include <string>

#include "token.h"
#include "object.h"
#include "message.h"

using namespace std;

class Context;

class ASTNode {
public:
	ASTNode(Token token) : token(token) {}

	ASTNode(Token token, vector<ASTNode *> children) : token(token), children(children) {}

	Token token;

	vector<ASTNode *> &get_children() { return children; }
	string to_string() { return to_string_indent(""); };

	string visit();
	Object *visit_statement(string *out, Context *context, bool *should_return);
	Object *visit_object(string *out, Context *context);
	Message visit_message();
	Object *visit_send(string *out, Context *context, bool *should_return);
	Object *visit_store(Context *context);
private:
	vector<ASTNode *> children;

	string to_string_indent(string indent);
};