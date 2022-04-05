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

using namespace std;

Context *global_context;

void Context::add(Object *obj, string name) {
	context[name] = obj;
}

Object *Context::get(string obj) {
	if (context.count(obj))
		return context.at(obj);
	
	if (this != global_context)
		return global_context->get(obj);

	return nullptr;
}

void Context::dump() {
	cout << "----------------------\nContext:\n";
	for (auto &c : context) {
		cout << c.first << " = " << c.second->to_string() << "\n";
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
	list->store_list("internal", new vector<Store *>());
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
	callable->get_context()->add(param_names_list, "param_names_list");
	callable->get_context()->add(param_binds_list, "param_binds_list");

	// add to global context
	global_context->add(object, "Object");
	global_context->add(tr, "true");
	global_context->add(fs, "false");
	global_context->add(list, "List");
	global_context->add(callable, "Callable");
}