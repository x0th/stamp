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
	bool _should_return; // dummy variable
	auto obj = visit_statement(&out, global_context, &_should_return, nullptr);
	if (out != "")
		return out;
	return obj->to_string();
}

Object *ASTNode::visit_statement(string *out, Context *context, bool *should_return, Object *external_object) {
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
				bool statement_returned = false;
				obj = c->visit_statement(&sout, context, &statement_returned, external_object);
				if (statement_returned)
					break;
			}
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return obj;
		}
		case TokObject:
		case TokValue:
		{
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
			bool send_returned = false;
			auto obj = visit_send(&sout, context, &send_returned, external_object);
			if (send_returned)
				*should_return = true;
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return obj;
		}
		case TokStore: return visit_store(context, external_object);
		default: *out = "error."; return nullptr; // FIXME: error
	}
}

Object *ASTNode::visit_object(string *out, Context *context) {
	auto store = context->get(token.value);
	switch (store->get_store_type()) {
		case Store::Type::Object: return store->get_obj();
		case Store::Type::Literal: *out = *store->get_lit(); return nullptr;
		case Store::Type::Executable: {
			string sout;
			bool _should_return = false; // dummy variable
			auto obj = store->get_exe()->visit_statement(&sout, context, &_should_return, nullptr);
			if (sout != "") {
				*out = sout;
				context->add(new Store(new string(sout)), token.value);
				return nullptr;
			}
			context->add(new Store(obj), token.value);
			return obj;
		}
		default: return nullptr; // FIXME error
	}
	return nullptr; // FIXME: error
}

Message ASTNode::visit_message() {
	if (children.size() == 0)
		return Message(token.value, nullptr, nullptr);
	
	return Message(token.value, children[0], nullptr);
}

Object *ASTNode::visit_send(string *out, Context *context, bool *should_return, Object *external_object) {
	Object *obj;
	if (children[0]->token.value == "this") {
		if (external_object) {
			obj = external_object;
		}
		else {
			// FIXME: Error
		}
	}
	else
		obj = children[0]->token.type == TokSend ? children[0]->visit_send(out, context, should_return, external_object) : children[0]->visit_object(nullptr, context);

	auto msg = children[1]->visit_message();

	if (msg.get_name() == "return") {
		// We're in a function
		if (context != global_context) {
			*should_return = true;
			return obj;
		} else {
			// FIXME: Error
		}
	}

	auto msg_obj = obj->send(msg, out, external_object);

	if (msg.get_name() == "clone")
		context->add(msg_obj, msg.get_sender()->token.value);

	if (*out != "")
		return nullptr;
	return msg_obj->get_obj();
}

Object *ASTNode::visit_store(Context *context, Object *external_object) {
	Object *obj;
	if (children[0]->token.value == "this") {
		if (external_object) {
			obj = external_object;
		}
		else {
			// FIXME: Error
		}
	}
	else
		obj = children[0]->visit_object(nullptr, context);

	string sout;
	bool should_return = false;
	auto rhs = children[2]->token.type == TokSend ? children[2]->visit_send(&sout, obj->get_context(), &should_return, external_object) : children[2]->visit_object(nullptr, obj->get_context());

	if (should_return) {
		// FIXME: Error
	}

	if (sout != "")
		obj->get_stores()[children[1]->token.value] = new Store(new string(sout));
	else {
		obj->get_stores()[children[1]->token.value] = new Store(rhs);
		rhs->set_external(obj);
	}

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