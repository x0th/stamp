/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <map>
#include <string>
#include <variant>
#include <set>
#include <functional>

#include "Object.h"
#include "Register.h"
#include "Interpreter.h"

std::variant<Object *, std::string> clone_object(Object *original, std::optional<std::variant<Register, std::string, uint32_t>> name, Interpreter&interpreter) {
	std::string new_type = std::get<std::string>(*name);
	if (std::isupper(new_type[0])) {
		Object *cloned = new Object(original, new_type);
		std::set<std::string> default_stores = { "clone" };
		cloned->add_default_stores(default_stores);
		interpreter.put_object(new_type, cloned);
		return cloned;
	} else {
		Object *cloned = new Object(original, original->get_type());
		interpreter.put_object(new_type, cloned);
		return cloned;
	}
}

std::variant<Object *, std::string> object_equals(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> other, Interpreter &interpreter) {
	Object *other_object;
	if (std::get_if<Register>(&*other))
		other_object = *std::get_if<Object*>(&interpreter.at(std::get<Register>(*other).get_index()));
	else
		other_object = interpreter.fetch_object(std::get<std::string>(*other));

	if (object->get_hash() == other_object->get_hash())
		return interpreter.fetch_global_object("True");
	else
		return interpreter.fetch_global_object("False");
}

std::variant<Object *, std::string> object_nequals(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> other, Interpreter &interpreter) {
	Object *other_object;
	if (std::get_if<Register>(&*other))
		other_object = *std::get_if<Object*>(&interpreter.at(std::get<Register>(*other).get_index()));
	else
		other_object = interpreter.fetch_object(std::get<std::string>(*other));

	if (object->get_hash() != other_object->get_hash())
		return interpreter.fetch_global_object("True");
	else
		return interpreter.fetch_global_object("False");
}

#define ENUMERATE_DEFAULT_STORES(DS) \
	DS("clone", clone_object), \
	DS("==", object_equals), \
	DS("!=", object_nequals)

#define __ADD_DEFAULT_STORE(fn, fn_name) { fn, fn_name }
std::map<std::string, std::function<std::variant<Object *, std::string>(Object*,std::optional<std::variant<Register, std::string, uint32_t>>,Interpreter&)>>
default_stores_map ={
		ENUMERATE_DEFAULT_STORES(__ADD_DEFAULT_STORE)
};
#undef __ADD_DEFAULT_STORE