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

Store *Object::send(Message &message, string *out) {
	if (message.get_name() == "proto") {
		if (!prototype) {
			*out = "None";
			return nullptr;
		}
		return new Store(prototype);
#ifdef DEBUG
	} else if (message.get_name() == "context") {
		context->dump();

		*out = " ";
		return nullptr;
#endif
	} else {
		auto name = message.get_name();
		// have in our table, handle
		if (stores.count(name)) {
			auto store = stores.at(name);
			switch (store->get_store_type()) {
			case Store::Type::Object: return store;
			case Store::Type::Literal: {
				auto lit = *store->get_lit();
				if (lit.size() >= 2 && lit[0] == ':' && lit[1] == ':') {
					auto store_out = handle_default(lit, message.get_sender(), out, message.get_requester());
					// if (store_out->get_store_type() == Store::Type::Object)
					// 	return store_out;
					return store_out;
				}
				*out = lit;
				return nullptr;
			}
			default: return nullptr; // FIXME: error
			}
		}

		// forward to prototype
		if (prototype) {
			if (!message.get_requester())
				message.set_requester(this);
			return prototype->send(message, out);
		}

		// no prototype, should throw error
		return nullptr;
	}

	return nullptr;
}

Store *Object::handle_default(string &lit, ASTNode *sender, string *out, Object *requester) {
	if (lit == "::clone") {
		return new Store(clone(sender));
	} else if (lit == "::clone_callable") {
		return new Store(clone_callable(sender));
	} else if (lit == "::clone_list") {
		return new Store(clone_list(sender));
	} else if (lit == "::==") {
		int use_hash = hash;
		if (requester)
			use_hash = requester->get_hash();
		
		if (use_hash == global_context->get(sender->token.value)->get_obj()->get_hash())
			return global_context->get("true");
		else
			return global_context->get("false");
	} else if (lit == "::!=") {
		int use_hash = hash;
		if (requester)
			use_hash = requester->get_hash();

		if (use_hash != global_context->get(sender->token.value)->get_obj()->get_hash())
			return global_context->get("true");
		else
			return global_context->get("false");
	} else if (lit == "::get") {
		auto use_stores = requester ? requester->get_stores()["internal"] : stores["internal"];
		auto element = (*use_stores->get_list())[stoi(sender->token.value)];
		if (element->get_store_type() == Store::Type::Literal) {
			*out = *element->get_lit();
			return nullptr;
		}
		return element;
	} else if (lit == "::push") {
		auto use_stores = requester ? requester->get_stores()["internal"] : stores["internal"];
		switch (sender->token.type) {
			case TokObject: use_stores->get_list()->push_back(new Store(context->get(sender->token.value)->get_obj())); break;
			case TokSend:
			case TokSList: use_stores->get_list()->push_back(new Store(sender)); break;
			case TokValue: use_stores->get_list()->push_back(new Store(new string(sender->token.value))); break;
			default: exit(1); // FIXME: error
		}
		if (requester)
			return new Store(requester);
		return new Store(this);
	} else if (lit == "::size") {
		auto use_stores = requester ? requester->get_stores()["internal"] : stores["internal"];
		*out = std::to_string(use_stores->get_list()->size());
		return nullptr;
	} else if (lit == "::store_param") {
		Message msg("push", sender, nullptr);
		string sout;

		if (requester) {
			requester->get_stores()["param_names"]->get_obj()->send(msg, &sout);
			return new Store(requester);
		}

		stores["param_names"]->get_obj()->send(msg, &sout);
		return new Store(this);
	} else if (lit == "::store_body") {
		if (requester) {
			requester->store_exe("body", sender);
			return new Store(requester);
		}

		store_exe("body", sender);
		return new Store(this);
	} else if (lit == "::call") {
		auto obj = requester ? requester : this;

		// Store context
		auto stored_context = context;
		
		// Create empty context
		context = new Context();
		
		// put passed parameters in context
		auto param_names = obj->get_stores()["param_names"]->get_obj();
		auto param_binds = obj->get_stores()["param_binds"]->get_obj();
		
		// FIXME: verify that two lists are of equal size
		Message size_msg("size", nullptr, nullptr);
		string size_str;
		param_names->send(size_msg, &size_str);
		int size = stoi(size_str);

		auto get_sender = new ASTNode(Token { type: TokValue, value: "0" });
		Message get_msg("get", get_sender, nullptr);

		for (int i = 0; i < size; i++) {
			get_sender->token.value = std::to_string(i);
			string obj_name;
			get_msg.set_requester(nullptr);
			param_names->send(get_msg, &obj_name);
			get_msg.set_requester(nullptr);
			auto msg_obj = param_binds->send(get_msg, nullptr);

			context->add(msg_obj, obj_name);
		}

		// execute body
		string sout;
		auto obj_out = obj->get_stores()["body"]->get_exe()->visit_statement(&sout, context);

		// restore context
		delete context;
		context = stored_context;

		if (sout != "") {
			*out = sout;
			return nullptr;
		}

		return new Store(obj_out);
	} else if (lit == "::pass_param") {
		Message msg("push", sender, nullptr);
		string sout;

		// FIXME: verify that we stored an object
		if (requester) {
			requester->get_stores()["param_binds"]->get_obj()->send(msg, &sout);
			return new Store(requester);
		}

		stores["param_binds"]->get_obj()->send(msg, &sout);
		return new Store(this);
	} else if (lit == "::if_true") {
		if (sender->get_children()[0]->visit_object(nullptr, context) == global_context->get("true")->get_obj()) {
			string sout;
			auto obj = sender->get_children()[1]->visit_statement(&sout, context);
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return  new Store(obj);
		}
		*out = " ";
		return nullptr;
	} else if (lit == "::if_false") {
		if (sender->get_children()[0]->visit_object(nullptr, context) == global_context->get("false")->get_obj()) {
			string sout;
			auto obj = sender->get_children()[1]->visit_statement(&sout, context);
			if (sout != "") {
				*out = sout;
				return nullptr;
			}
			return  new Store(obj);
		}
		*out = " ";
		return nullptr;
	}

	return nullptr; // FIXME: error
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
	auto param_names_list = global_context->get("List")->get_obj()->clone_list(&param_names_lit);
	auto param_binds_list = global_context->get("List")->get_obj()->clone_list(&param_binds_lit);
	cloned->store_obj("param_names", param_names_list);
	cloned->store_obj("param_binds", param_binds_list);
	cloned->store_lit("clone", new string("::clone_callable"));
	cloned->context->add(new Store(param_names_list), "param_names_list");
	cloned->context->add(new Store(param_binds_list), "param_binds_list");

	return cloned;
}

Object *Object::clone_list(ASTNode *sender) {
	Object * cloned = clone(sender);

	cloned->store_list("internal", new vector<Store *>());
	cloned->store_lit("clone", new string("::clone_list"));

	return cloned;
}

string Object::to_string() {
	if (hash == global_context->get("true")->get_obj()->get_hash())
		return "true";
	else if (hash == global_context->get("false")->get_obj()->get_hash())
		return "false";

	stringstream s;
	string out;
	Message msg("type", nullptr, nullptr);
	send(msg, &out);
	s << out << "-" << hex << hash;
	return s.str();
}