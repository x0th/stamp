/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>
#include <string>

#include "ast.h"
#include "context.h"

using namespace std;

string ASTNode::visit() {
	string out;
	auto obj = visit_statement(&out, global_context);
	if (out != "")
		return out;
	return obj->to_string();
}

Object *ASTNode::visit_statement(string *out, Context *context) {
	switch (token.type) {
		case TokSList: {
			string sout;
			Object *obj;
			if (children.size() == 0) {
				*out = " ";
				return nullptr;
			}

			for (auto &c : children) {
				sout = "";
				obj = c->visit_statement(&sout, context);
			}
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return obj;
		}
		case TokObject: {
			string sout;
			auto obj = visit_object(&sout, context);
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return obj;
		}
		case TokSend: {
			string sout;
			auto obj = visit_send(&sout, context);
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return obj;
		}
		case TokStore: return visit_store(context);
		default: *out = "error."; return nullptr; // FIXME: error
	}
}

Object *ASTNode::visit_object(string *out, Context *context) {
	auto store = context->get(token.value);
	if (store->get_store_type() == Store::Type::Object)
		return store->get_obj();
	else if (store->get_store_type() == Store::Type::Executable) {
		string sout;
		auto obj = store->get_exe()->visit_statement(&sout, context);
		if (sout != "") {
			*out = sout;
			context->add(new Store(new string(sout)), token.value);
			return nullptr;
		}
		context->add(new Store(obj), token.value);
		return obj;
	}
	return nullptr; // FIXME: error
}

Message ASTNode::visit_message() {
	if (children.size() == 0)
		return Message(token.value, nullptr, nullptr);
	
	return Message(token.value, children[0], nullptr);
}

Object *ASTNode::visit_send(string *out, Context *context) {
	auto obj = children[0]->token.type == TokSend ? children[0]->visit_send(out, context) : children[0]->visit_object(nullptr, context);

	auto msg = children[1]->visit_message();
	auto msg_obj = obj->send(msg, out);

	if (msg.get_name() == "clone")
		context->add(msg_obj, msg.get_sender()->token.value);

	if (*out != "")
		return nullptr;
	return msg_obj->get_obj();
}

Object *ASTNode::visit_store(Context *context) {
	auto obj = children[0]->visit_object(nullptr, context);

	string sout;
	auto rhs = children[2]->token.type == TokSend ? children[2]->visit_send(&sout, obj->get_context()) : children[2]->visit_object(nullptr, obj->get_context());

	if (sout != "")
		obj->get_stores()[children[1]->token.value] = new Store(new string(sout));
	else
		obj->get_stores()[children[1]->token.value] = new Store(rhs);

	return obj;
}

string ASTNode::to_string_indent(string indent) {
	stringstream s;
	s << indent << token_str(&token) << "\n";
	for (auto &c : children) {
		s << c->to_string_indent(indent + "  ");
	}
	return s.str();
}