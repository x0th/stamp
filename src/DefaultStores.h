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

std::variant<Object *, std::string> clone_object(Object *original, std::optional<std::variant<Register, std::string, uint32_t>> name) {
	std::string new_type = std::get<std::string>(*name);
	if (std::isupper(new_type[0])) {
		Object *cloned = new Object(original, new_type);
		std::set<std::string> default_stores = { "clone" };
		cloned->add_default_stores(default_stores);
		return cloned;
	} else {
		Object *cloned = new Object(original, original->get_type());
		return cloned;
	}
}

#define ENUMERATE_DEFAULT_STORES(DS) \
	DS("clone", clone_object),

#define __ADD_DEFAULT_STORE(fn, fn_name) { fn, fn_name }
std::map<std::string, std::function<std::variant<Object *, std::string>(Object*,std::optional<std::variant<Register, std::string, uint32_t>>)>>
default_stores_map ={
		ENUMERATE_DEFAULT_STORES(__ADD_DEFAULT_STORE)
};
#undef __ADD_DEFAULT_STORE