/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>
#include <set>
#include <string>

#include "Context.h"

void Context::add(std::string name, Object *object) {
	context[name] = object;
}

Object *Context::get(std::string &name) {
	if (context.count(name))
		return context[name];
	return nullptr;
}

void Context::dump() {
	std::cout << "----------------------\nContext:\n";
	for (auto const& c : context) {
		std::cout << c.first << " = " << c.second->to_string();
	}
	std::cout << "----------------------\n";
}

Context *Context::make_global_context() {
	static Context *global_context = new Context();

	// Object
	auto object = new Object(nullptr, "Object");
	object->add_store<StoreLiteral>("type", "Object");
	std::set<std::string> object_stores = {"clone", "==", "!="};
	object->add_default_stores(object_stores);

	global_context->add("Object", object);

	return global_context;
}