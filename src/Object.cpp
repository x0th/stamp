/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "DefaultStores.h"
#include "Object.h"
#include "Interpreter.h"

std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*>
        Object::send(std::string message, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Object *forwarder, Interpreter &interpreter) {
	if (is_default_store(message)) {
		return default_stores_map[message](forwarder ? forwarder : this, stamp, interpreter);
	}
	if (stores.count(message)) {
#define __UNWRAP_STORE(t, c) \
		case InternalStore::Type::t: return static_cast<c*>(stores[message])->unwrap();
		switch(stores[message]->get_type()) {
			ENUMERATE_STORE_TYPES(__UNWRAP_STORE)
		}
#undef __UNWRAP_STORE
	} else {
		if (prototype)
			return prototype->send(message, stamp, this, interpreter);
		else {
			// FIXME: Error!
		}
	}
	// FIXME: Error!
	Object *error = nullptr;
	return error;
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