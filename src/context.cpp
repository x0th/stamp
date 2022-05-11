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
#define default_store(obj, name) obj->store_lit(name, new string(string("::") + name))
#define add_global(obj, name) global_context->add(new Store(obj), name)
#define lit(lit_name, name) ASTNode lit_name(Token { type: TokValue, value: name })

	srand(time(nullptr));

	global_context = new Context();

	// Object
	auto object = new Object(nullptr);
	object->store_lit("type", new string("Object"));
	default_store(object, "clone");
	default_store(object, "==");
	default_store(object, "!=");
	// FIXME: maybe store these in a separate object?
	default_store(object, "if_true");
	default_store(object, "if_false");

	// true, false
	lit(tr_lit, "true");
	lit(fs_lit, "false");
	auto tr = object->clone(&tr_lit);
	auto fs = object->clone(&fs_lit);
	tr->get_stores()["clone"] = new Store(tr);
	fs->get_stores()["clone"] = new Store(fs);

	// List
	lit(list_lit, "List");
	auto list = object->clone(&list_lit);
	default_store(list, "get");
	default_store(list, "push");
	default_store(list, "size");
	default_store(list, "store_value");

	// Operators
	lit(operators_lit, "Operators");
	auto operators = object->clone(&operators_lit);
	lit(table_lit, "table");
	auto table = list->clone(&table_lit);
	table->store_list("value", new vector<Store *>());
	// 1. % * /
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("%")), new Store(new string("*")), new Store(new string("/"))}));
	// 2. + -
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("+")), new Store(new string("-"))}));
	// 3. << >>
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("<<")), new Store(new string(">>"))}));
	// 4. < <= > >=
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("<")), new Store(new string("<=")), new Store(new string(">")), new Store(new string(">="))}));
	// 5. != ==
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("!=")), new Store(new string("=="))}));
	// 6. &
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("&"))}));
	// 7. ><
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("><"))}));
	// 8. |
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("|"))}));
	// 9. &&
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("&&"))}));
	// 10. ||
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("||"))}));
	// 11. !
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("!"))}));
	// 12. return
	table->get_stores()["value"]->get_list()->push_back(new Store(new vector<Store *>{new Store(new string("return"))}));
	operators->store_obj("table", table);
	operators->get_stores()["clone"] = new Store(operators);

	// Callable
	lit(callable_lit, "Callable");
	auto callable = object->clone(&callable_lit);
	lit(param_names_lit, "param_names");
	lit(param_binds_lit, "param_binds");
	auto param_names_list = list->clone(&param_names_lit);
	auto param_binds_list = list->clone(&param_binds_lit);
	param_names_list->store_list("value", new vector<Store *>());
	param_binds_list->store_list("value", new vector<Store *>());
	callable->store_obj("param_names", param_names_list);
	callable->store_obj("param_binds", param_binds_list);
	callable->store_lit("clone", new string("::clone_callable"));
	default_store(callable, "store_param");
	default_store(callable, "pass_param");
	default_store(callable, "call");
	default_store(callable, "store_body");
	callable->get_context()->add(new Store(param_names_list), "param_names_list");
	callable->get_context()->add(new Store(param_binds_list), "param_binds_list");

	// if
	lit(if_lit, "if");
	auto i_f = object->clone(&if_lit);
	default_store(i_f, "if_true");
	default_store(i_f, "if_false");
	i_f->get_stores()["if"] = new Store(i_f);

	// while
	lit(while_lit, "while");
	auto while_ = object->clone(&while_lit);
	default_store(while_, "exec_while_true");
	default_store(while_, "exec_while_false");
	while_->get_stores()["while"] = new Store(while_);

	// Int
	lit(int_lit, "Int");
	auto Int = object->clone(&int_lit);
	default_store(Int, "store_value");
	default_store(Int, "+");
	default_store(Int, "-");
	default_store(Int, "*");
	default_store(Int, "/");
	default_store(Int, "%");

	// Char
	lit(char_lit, "Char");
	auto Char = object->clone(&char_lit);
	default_store(Char, "store_value");
	default_store(Char, "+");

	// String
	lit(string_lit, "String");
	auto String = object->clone(&string_lit);
	default_store(String, "store_value");
	default_store(String, "+");
	default_store(String, "*");

	// add to global context
	add_global(object, "Object");
	add_global(tr, "true");
	add_global(fs, "false");
	add_global(callable, "Callable");
	add_global(list, "List");
	add_global(operators, "Operators");
	add_global(i_f, "if");
	add_global(while_, "while");
	add_global(Int, "Int");
	add_global(Char, "Char");
	add_global(String, "String");

#undef default_store
#undef add_global
#undef lit
}