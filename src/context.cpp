/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <time.h>

#include "object.h"
#include "token.h"
#include "context.h"
#include "ast.h"
#include "parser.h"

using namespace std;

Context *global_context;

void Context::add(Store *store, string name) {
	context[name] = store;
}

Store *Context::get(string name) {
	if (context.count(name))
		return context.at(name);
	
	if (this != global_context)
		return global_context->get(name);

	return nullptr;
}

void Context::dump() {
	cout << "----------------------\nContext:\n";
	for (auto &c : context) {
		cout << c.first << " = ";
		switch(c.second->get_store_type()) {
			case Store::Type::Object: cout << c.second->get_obj()->to_string() << "\n"; break;
			case Store::Type::Executable: cout << "Code\n"; break;
			case Store::Type::Literal: cout << "Literal(" << *c.second->get_lit() << ")\n"; break;
			default: cout << "\n"; continue; // FIXME: print other types
		}
	}
	cout << "----------------------\n";
}

void initialize_global_context() {
	srand(time(nullptr));

	global_context = new Context();

	// Object
	auto object = new Object(nullptr);
	object->store_lit("type", new string("Object"));
	object->store_lit("clone", new string("::clone"));
	object->store_lit("==", new string("::=="));
	object->store_lit("!=", new string("::!="));
	// FIXME: maybe store these in a separate object?
	object->store_lit("if_true", new string("::if_true"));
	object->store_lit("if_false", new string("::if_false"));

	// true, false
	ASTNode tr_lit(Token { type: TokValue, value: "true" });
	ASTNode fs_lit(Token { type: TokValue, value: "false" });
	auto tr = object->clone(&tr_lit);
	auto fs = object->clone(&fs_lit);
	tr->get_stores()["clone"] = new Store(tr);
	fs->get_stores()["clone"] = new Store(fs);

	// List
	ASTNode list_lit(Token { type: TokValue, value: "List" });
	auto list = object->clone(&list_lit);
	list->store_list("value", new vector<Store *>());
	list->store_lit("get", new string("::get"));
	list->store_lit("push", new string("::push"));
	list->store_lit("clone", new string("::clone_list"));
	list->store_lit("size", new string("::size"));

	// Callable
	ASTNode callable_lit(Token { type: TokValue, value: "Callable" });
	auto callable = object->clone(&callable_lit);
	ASTNode param_names_lit(Token { type: TokValue, value: "param_names" });
	ASTNode param_binds_lit(Token { type: TokValue, value: "param_binds" });
	auto param_names_list = list->clone_list(&param_names_lit);
	auto param_binds_list = list->clone_list(&param_binds_lit);
	callable->store_obj("param_names", param_names_list);
	callable->store_obj("param_binds", param_binds_list);
	callable->store_lit("clone", new string("::clone_callable"));
	callable->store_lit("store_param", new string("::store_param"));
	callable->store_lit("pass_param", new string("::pass_param"));
	callable->store_lit("call", new string("::call"));
	callable->store_lit("store_body", new string("::store_body"));
	callable->get_context()->add(new Store(param_names_list), "param_names_list");
	callable->get_context()->add(new Store(param_binds_list), "param_binds_list");

	// if
	ASTNode if_lit(Token { type: TokValue, value: "if" });
	auto i_f = object->clone(&if_lit);
	i_f->store_lit("if_true", new string("::if_true"));
	i_f->store_lit("if_false", new string("::if_false"));
	i_f->get_stores()["if"] = new Store(i_f);

	// Int
	ASTNode int_lit(Token { type: TokValue, value: "Int" });
	auto Int = object->clone(&int_lit);
	Int->store_lit("store_value", new string("::store_value"));

	// Char
	ASTNode char_lit(Token { type: TokValue, value: "Char" });
	auto Char = object->clone(&char_lit);
	Char->store_lit("store_value", new string("::store_value"));

	// add to global context
	global_context->add(new Store(object), "Object");
	global_context->add(new Store(tr), "true");
	global_context->add(new Store(fs), "false");
	global_context->add(new Store(callable), "Callable");
	global_context->add(new Store(list), "List");
	global_context->add(new Store(i_f), "if");
	global_context->add(new Store(Int), "Int");
	global_context->add(new Store(Char), "Char");
}