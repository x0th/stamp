/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <iostream>

#include "object.h"
#include "context.h"
#include "ast.h"

using namespace std;

Object::Object(Object *_prototype) {
	hash = rand();
	prototype = _prototype;
	context = new Context();
}

Object *Object::send(Message &message, string *out) {
	if (message.get_name() == "proto") {
		if (!prototype)
			*out = "None";
		return prototype;
	} else if (message.get_name() == "context") {
		// FIXME: for debugging purposes only
		context->dump();

		*out = " ";
		return nullptr;
	} else {
		auto name = message.get_name();
		// have in our table, handle
		if (stores.count(name)) {
			auto store = stores.at(name);
			switch (store->get_store_type()) {
			case Store::Type::Object: return store->get_obj();
			case Store::Type::Literal: {
				auto lit = *store->get_lit();
				if (lit.size() >= 2 && lit[0] == ':' && lit[1] == ':')
					return handle_default(lit, message.get_sender(), out);
				*out = lit;
				return nullptr;
			}
			default: return nullptr; // FIXME: error
			}
		}

		// forward to prototype
		if (prototype) {
			return prototype->send(message, out);
		}

		// no prototype, should throw error
		return nullptr;
	}

	return nullptr;
}

Object *Object::handle_default(string &lit, ASTNode *sender, string *out) {
	if (lit == "::clone") {
		return clone(sender);
	} else if (lit == "::clone_callable") {
		return clone_callable(sender);
	} else if (lit == "::clone_list") {
		return clone_list(sender);
	} else if (lit == "::==") {
		auto sender_obj = global_context->get(sender->token.value);
		if (hash == sender_obj->get_hash()) {
			return global_context->get("true");
		} else {
			return global_context->get("false");
		}
	} else if (lit == "::!=") {
		auto sender_obj = global_context->get(sender->token.value);
		if (hash != sender_obj->get_hash()) {
			return global_context->get("true");
		} else {
			return global_context->get("false");
		}
	} else if (lit == "::get") {
		auto element = (*stores["internal"]->get_list())[stoi(sender->token.value)];
		auto element_type = element->get_store_type();
		if (element_type == Store::Type::Object) {
			return element->get_obj();
		} else if (element_type == Store::Type::Literal) {
			*out = *element->get_lit();
			return nullptr;
		}
	} else if (lit == "::push") {
		switch (sender->token.type) {
			case TokObject: stores["internal"]->get_list()->push_back(new Store(context->get(sender->token.value))); break;
			case TokSList: stores["internal"]->get_list()->push_back(new Store(sender)); break;
			case TokSend: {
				string sout;
				auto obj = sender->visit_statement(&sout, context);
				if (sout != "")
					stores["internal"]->get_list()->push_back(new Store(new string(sout)));
				else
					stores["internal"]->get_list()->push_back(new Store(obj));
				break;
			}
			case TokValue: stores["internal"]->get_list()->push_back(new Store(new string(sender->token.value))); break;
			default: exit(1); // FIXME: error
		}
		return this;
	} else if (lit == "::size") {
		*out = std::to_string(stores["internal"]->get_list()->size());
		return nullptr;
	} else if (lit == "::store_param") {
		Message msg("push", sender);
		string sout;
		stores["param_names"]->get_obj()->send(msg, &sout);
		return this;
	} else if (lit == "::store_body") {
		store_exe("body", sender);
		return this;
	} else if (lit == "::call") {
		// Store context
		auto stored_context = context;
		
		// Create empty context
		context = new Context();
		
		// put passed parameters in context
		auto param_names = stores["param_names"]->get_obj();
		auto param_binds = stores["param_binds"]->get_obj();
		
		// FIXME: verify that two lists are of equal size
		Message size_msg("size", nullptr);
		string size_str;
		param_names->send(size_msg, &size_str);
		int size = stoi(size_str);

		auto get_sender = new ASTNode(Token { type: TokValue, value: "0" });
		Message get_msg("get", get_sender);

		for (int i = 0; i < size; i++) {
			get_sender->token.value = std::to_string(i);
			string obj_name;
			param_names->send(get_msg, &obj_name);
			auto obj = param_binds->send(get_msg, nullptr);

			context->add(obj, obj_name);
		}

		// execute body
		string sout;
		auto obj = stores["body"]->get_exe()->visit_statement(&sout, context);
		if (sout != "")
			*out = sout;

		// restore context
		delete context;
		context = stored_context;

		return obj;
	} else if (lit == "::pass_param") {
		Message msg("push", sender);
		string sout;
		stores["param_binds"]->get_obj()->send(msg, &sout);
		// FIXME: verify that we stored an object

		return this;
	}

	return nullptr;
}

void Object::store_obj(string store_name, Object *obj) {
	stores[store_name] = new Store(obj);
}

void Object::store_lit(string store_name, string *lit) {
	stores[store_name] = new Store(lit);
}

void Object::store_exe(string store_name, ASTNode *exe) {
	stores[store_name] = new Store(exe);
}

void Object::store_list(string store_name, vector<Store *> *list) {
	stores[store_name] = new Store(list);
}

Object *Object::clone(ASTNode *sender) {
	Object *cloned = new Object(this);
	string new_type = sender ? sender->token.value : std::to_string(cloned->get_hash());

	cloned->get_stores().insert({ "==", stores.at("==") });
	cloned->get_stores().insert({ "!=", stores.at("!=") });
	if (new_type[0] >= 65 && new_type[0] <= 90) {
		cloned->store_lit("type", new string(new_type));
		cloned->store_lit("clone", new string("::clone"));
	}

	return cloned;
};

Object *Object::clone_callable(ASTNode *sender) {
	Object *cloned = clone(sender);

	ASTNode param_names_lit(Token { type: TokValue, value: "param_names" });
	ASTNode param_binds_lit(Token { type: TokValue, value: "param_binds" });
	auto param_names_list = global_context->get("List")->clone_list(&param_names_lit);
	auto param_binds_list = global_context->get("List")->clone_list(&param_binds_lit);
	cloned->store_obj("param_names", param_names_list);
	cloned->store_obj("param_binds", param_binds_list);
	cloned->store_lit("clone", new string("::clone_callable"));
	cloned->store_lit("store_param", new string("::store_param"));
	cloned->store_lit("pass_param", new string("::pass_param"));
	cloned->store_lit("call", new string("::call"));
	cloned->store_lit("store_body", new string("::store_body"));
	cloned->context->add(param_names_list, "param_names_list");
	cloned->context->add(param_binds_list, "param_binds_list");

	return cloned;
}

Object *Object::clone_list(ASTNode *sender) {
	Object * cloned = clone(sender);

	cloned->store_list("internal", new vector<Store *>());
	cloned->store_lit("get", new string("::get"));
	cloned->store_lit("push", new string("::push"));
	cloned->store_lit("clone", new string("::clone_list"));
	cloned->store_lit("size", new string("::size"));

	return cloned;
}

string Object::to_string() {
	if (hash == global_context->get("true")->get_hash())
		return "true";
	else if (hash == global_context->get("false")->get_hash())
		return "false";

	stringstream s;
	string out;
	Message msg("type", nullptr);
	send(msg, &out);
	s << out << "_" << hex << hash;
	return s.str();
}