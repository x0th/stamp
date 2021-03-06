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
#include "Error.h"

#define int_arithmetic(op) \
	auto result = static_cast<StoreInt*>(object->get_store("value"))->unwrap() op static_cast<StoreInt*>(other->get_store("value"))->unwrap(); \
	auto result_obj = std::get<Object*>(clone_object(object->get_prototype(), "::lit_" + std::to_string(result), interpreter)); \
	result_obj->add_store<StoreInt>("value", result, true); \
	return result_obj;

#define int_compare(op) \
	auto result = static_cast<StoreInt*>(object->get_store("value"))->unwrap() op static_cast<StoreInt*>(other->get_store("value"))->unwrap(); \
	return result ? interpreter.fetch_global_object("True") : interpreter.fetch_global_object("False");

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

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> object_equals(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	Object *other;
	if (std::get_if<Register>(&*stamp))
		other = *std::get_if<Object*>(&interpreter.at(std::get<Register>(*stamp).get_index()));
	else
		other = interpreter.fetch_object(std::get<std::string>(*stamp));

	if (object->get_type() == "Int") {
		int_compare(==)
	} else {
		if (object->get_hash() == other->get_hash())
			return interpreter.fetch_global_object("True");
		else
			return interpreter.fetch_global_object("False");
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> object_nequals(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	Object *other;
	if (std::get_if<Register>(&*stamp))
		other = *std::get_if<Object*>(&interpreter.at(std::get<Register>(*stamp).get_index()));
	else
		other = interpreter.fetch_object(std::get<std::string>(*stamp));

	if (object->get_type() == "Int") {
		int_compare(!=)
	} else {
		if (object->get_hash() == other->get_hash())
			return interpreter.fetch_global_object("False");
		else
			return interpreter.fetch_global_object("True");
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> store_value(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> _stamp, Interpreter &) {
	auto stamp = std::get<std::string>(*_stamp);
	if (object->get_type() == "Int") {
		object->add_store<StoreInt>("value", std::stoi(stamp), true);
	} else if (object->get_type() == "Char") {
		object->add_store<StoreChar>("value", stamp[0], true);
	} else if (object->get_type() == "String") {
		object->add_store<StoreLiteral>("value", stamp, true);
	} else {
		terminating_error(StampError::DefaultStoreError, "store_value is not implemented for " + object->get_type());
	}
	return object;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> get(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	// FIXME: maybe move to start of interpreter (or something similar), otherwise will have to add to all Vec functions
	if (!object->get_store("value")) {
		object->add_store<StoreVec>("value", new std::vector<InternalStore*>(), true);
	}
	auto index = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()))->send("value", std::nullopt, nullptr, interpreter);
	auto value = (static_cast<StoreVec*>(object->get_store("value")))->unwrap()->at(std::get<int32_t>(index));
#define __UNWRAP_STORE(t, c) \
		case InternalStore::Type::t: return static_cast<c*>(value)->unwrap();
	switch(value->get_type()) {
			ENUMERATE_STORE_TYPES(__UNWRAP_STORE)
		default: {
			terminating_error(StampError::DefaultStoreError, "Unable to unwrap store.");
		}
	}
	Object *error = nullptr;
	return error;
#undef __UNWRAP_STORE
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> push(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto store = static_cast<InternalStore*>(new StoreObject(std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index())), true));
	// FIXME: maybe move to start of interpreter (or something similar), otherwise will have to add to all Vec functions
	if (!object->get_store("value")) {
		object->add_store<StoreVec>("value", new std::vector<InternalStore*>(), true);
	}
	(static_cast<StoreVec*>(object->get_store("value")))->unwrap()->push_back(store);
	return object;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> clone_callable(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto new_fn = std::get<Object*>(clone_object(object, stamp, interpreter));

	std::set<std::string> ds = { "clone_callable" };
	new_fn->add_default_stores(ds);
	auto new_param_names = std::get<Object*>(clone_object(static_cast<StoreObject*>(object->get_store("param_names"))->unwrap(), "::param_names", interpreter));
	new_fn->add_store<StoreObject>("param_names", new_param_names, true);
	auto num_passed_params = std::get<Object*>(clone_object(interpreter.fetch_global_object("Int"), "::num_passed_params", interpreter));
	store_value(num_passed_params, "0", interpreter);
	new_fn->add_store<StoreObject>("num_passed_params", num_passed_params, true);

	return new_fn;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> store_param(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto vec = static_cast<StoreObject*>(object->get_store("param_names"))->unwrap();
	auto param = std::get<Object*>(clone_object(interpreter.fetch_global_object("String"), "::" + std::get<std::string>(*stamp), interpreter));
	store_value(param, stamp, interpreter);
	push(vec, interpreter.store_at_next_available(param), interpreter);
	return object;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> pass_body(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &) {
	object->add_store<StoreRegister>("body", std::get<uint32_t>(*stamp), false);
	return object;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> pass_param(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	uint32_t bb_index = static_cast<StoreRegister*>(object->get_store("body"))->unwrap();
	Object *param;
	if (std::holds_alternative<std::string>(*stamp))
		param = interpreter.fetch_object(std::get<std::string>(*stamp));
	else
		param = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	auto num_passed_params_obj = static_cast<StoreObject*>(object->get_store("num_passed_params"))->unwrap();
	auto param_names = static_cast<StoreObject*>(object->get_store("param_names"))->unwrap();
	auto this_param_name_obj = std::get<Object*>(get(param_names, interpreter.store_at_next_available(num_passed_params_obj), interpreter));
	auto this_param_name = std::get<std::string>(this_param_name_obj->send("value", std::nullopt, nullptr, interpreter));
	interpreter.put_object_at_scope(this_param_name, param, bb_index);
	// FIXME: change this when/if there is a better way to change the value
	store_value(num_passed_params_obj, std::to_string(static_cast<StoreInt*>(num_passed_params_obj->get_store("value"))->unwrap() + 1), interpreter);
	return object;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> call(Object *object, std::optional<std::variant<Register, std::string, uint32_t>>, Interpreter &interpreter) {
	// FIXME: verify that number of passed params is the same as number of param names
	uint32_t bb_index = static_cast<StoreRegister*>(object->get_store("body"))->unwrap();
	interpreter.save_next_bb();
	interpreter.jump_bb(bb_index);
	return object;
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> get_return_value(Object *, std::optional<std::variant<Register, std::string, uint32_t>>, Interpreter &interpreter) {
	auto retval = interpreter.pop_retval();
	if (retval)
		return interpreter.at((*retval).get_index());
	else {
		Object *out = nullptr;
		return out;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> mod(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(%)
	} else {
		terminating_error(StampError::DefaultStoreError, "% default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> mul(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(*)
	} else {
		terminating_error(StampError::DefaultStoreError, "* default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> divop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(/)
	} else {
		terminating_error(StampError::DefaultStoreError, "/ default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> add(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(+)
	} else {
		terminating_error(StampError::DefaultStoreError, "+ default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> sub(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(-)
	} else {
		terminating_error(StampError::DefaultStoreError, "- default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> shl(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(<<)
	} else {
		terminating_error(StampError::DefaultStoreError, "<< default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> shr(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(>>)
	} else {
		terminating_error(StampError::DefaultStoreError, ">> default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> lop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_compare(<)
	} else {
		terminating_error(StampError::DefaultStoreError, ">> default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> leop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_compare(<=)
	} else {
		terminating_error(StampError::DefaultStoreError, ">> default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> gop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_compare(>)
	} else {
		terminating_error(StampError::DefaultStoreError, ">> default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> geop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_compare(>=)
	} else {
		terminating_error(StampError::DefaultStoreError, ">> default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> andop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(&)
	} else {
		terminating_error(StampError::DefaultStoreError, "+ default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> xorop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(^)
	} else {
		terminating_error(StampError::DefaultStoreError, "+ default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> orop(Object *object, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	auto other = std::get<Object *>(interpreter.at(std::get<Register>(*stamp).get_index()));
	if (object->get_type() == "Int") {
		int_arithmetic(|)
	} else {
		terminating_error(StampError::DefaultStoreError, "+ default store not implemented for " + object->get_type() + " and " + other->get_type() + ".");
		Object *error = nullptr;
		return error;
	}
}

#define ENUMERATE_DEFAULT_STORES(DS) \
	DS("clone", clone_object), \
	DS("==", object_equals), \
	DS("!=", object_nequals), \
	DS("store_value", store_value), \
	DS("get", get), \
	DS("push", push), \
    DS("clone_callable", clone_callable), \
	DS("store_param", store_param), \
	DS("pass_body", pass_body), \
	DS("pass_param", pass_param), \
	DS("call", call), \
    DS("get_return_value", get_return_value), \
	DS("%", mod), \
	DS("*", mul), \
	DS("/", divop), \
	DS("+", add), \
	DS("-", sub), \
	DS("<<", shl), \
	DS(">>", shr), \
    DS("<", lop), \
	DS("<=", leop), \
	DS(">", gop), \
	DS(">=", geop), \
	DS("&", andop), \
	DS("><", xorop), \
	DS("|", orop) \

#define __ADD_DEFAULT_STORE(fn, fn_name) { fn, fn_name }
std::map<std::string, std::function<std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*>(Object*,std::optional<std::variant<Register, std::string, uint32_t>>,Interpreter&)>>
default_stores_map ={
		ENUMERATE_DEFAULT_STORES(__ADD_DEFAULT_STORE)
};
#undef __ADD_DEFAULT_STORE