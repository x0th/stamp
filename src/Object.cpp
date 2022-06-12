/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "DefaultStores.h"
#include "Object.h"
#include "Interpreter.h"

std::variant<Object *, std::string, int32_t> Object::send(std::string message, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Interpreter &interpreter) {
	if (is_default_store(message)) {
		return default_stores_map[message](this, stamp, interpreter);
	}
	if (stores.count(message)) {
#define __UNWRAP_STORE(t, c) \
		case InternalStore::Type::t: return static_cast<c*>(stores[message])->unwrap();
		switch(stores[message]->get_type()) {
			ENUMERATE_STORE_TYPES(__UNWRAP_STORE)
		}
#undef __UNWRAP_STORE
	}
	// FIXME: Error!
	return nullptr;
}

std::string Object::to_string() const {
	std::stringstream s;
	if (type == "True")
		s << "True";
	else if (type == "False")
		s << "False";
	else
		s << type << "-" << std::hex << hash;
	return s.str();
}