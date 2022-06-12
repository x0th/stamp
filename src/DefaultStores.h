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

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> clone_object(Object *original, std::optional<std::variant<Register, std::string, uint32_t>> name, Interpreter&interpreter) {
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

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> object_equals(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> other, Interpreter &interpreter) {
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

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> object_nequals(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> other, Interpreter &interpreter) {
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

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> store_value(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> _stamp, Interpreter &) {
	auto stamp = std::get<std::string>(*_stamp);
	if (object->get_type() == "Int") {
		object->add_store<StoreInt>("value", std::stoi(stamp));
		return object;
	} else if (object->get_type() == "Char") {
		object->add_store<StoreChar>("value", stamp[0]);
		return object;
	} else {
		// FIXME: Error!
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> get(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto index = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()))->send("value", nullptr, nullptr, interpreter);
	auto value = (static_cast<StoreVec*>(object->get_store("value")))->unwrap()->at(std::get<int32_t>(index));
#define __UNWRAP_STORE(t, c) \
		case InternalStore::Type::t: return static_cast<c*>(value)->unwrap();
	switch(value->get_type()) {
			ENUMERATE_STORE_TYPES(__UNWRAP_STORE)
		default: {
			// FIXME: Error!
			Object *error = nullptr;
			return error;
		}
	}
#undef __UNWRAP_STORE
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> push(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto store = static_cast<InternalStore*>(new StoreObject(std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()))));
	(static_cast<StoreVec*>(object->get_store("value")))->unwrap()->push_back(store);
	return object;
}

#define ENUMERATE_DEFAULT_STORES(DS) \
	DS("clone", clone_object), \
	DS("==", object_equals), \
	DS("!=", object_nequals), \
	DS("store_value", store_value), \
	DS("get", store_value), \
	DS("push", push)

#define __ADD_DEFAULT_STORE(fn, fn_name) { fn, fn_name }
std::map<std::string, std::function<std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*>(Object*,std::optional<std::variant<Register, std::string, uint32_t>>,Interpreter&)>>
default_stores_map ={
		ENUMERATE_DEFAULT_STORES(__ADD_DEFAULT_STORE)
};
#undef __ADD_DEFAULT_STORE