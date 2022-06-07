/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "Object.h"

std::variant<Object *, std::string> Object::send(std::string message, std::optional<std::variant<Register, std::string, uint32_t>> stamp) {
	if (stores.count(message)) {
#define __UNWRAP_STORE(t, c) \
		case InternalStore::Type::t: return static_cast<c*>(stores[message])->unwrap();
		switch(stores[message]->get_type()) {
			ENUMERATE_STORE_TYPES(__UNWRAP_STORE)
		}
#undef __UNWRAP_STORE
	}
	return "lole";
}

std::string Object::to_string() const {
	std::stringstream s;
	s << type << "-" << std::hex << hash;
	return s.str();
}