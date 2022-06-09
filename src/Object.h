/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <set>
#include <optional>
#include <map>
#include <variant>

#include "Register.h"

class Object;
class StoreObject;
class StoreLiteral;

#define ENUMERATE_STORE_TYPES(T)       \
	T(StoreObject, StoreObject)        \
	T(StoreLiteral, StoreLiteral)

class InternalStore {
public:
	enum class Type {
#define __STORE_TYPES(t, c) \
    t,
		ENUMERATE_STORE_TYPES(__STORE_TYPES)
#undef __STORE_TYPES
	};

	InternalStore(Type type) : type(type) {}

	Type get_type() const { return type; }
private:
	Type type;
};

class StoreObject : public InternalStore {
public:
	StoreObject(Object *object) : InternalStore(Type::StoreObject), object(object) {}

	Object *unwrap() const { return object; }
private:
	Object *object;
};

class StoreLiteral : public InternalStore {
public:
	StoreLiteral(std::string literal) : InternalStore(Type::StoreLiteral), literal(literal) {}

	std::string unwrap() const { return literal; }
private:
	std::string literal;
};

class Object {
public:
	Object(Object *prototype, std::string type) : prototype(prototype), type(type) {
		hash = rand();
	}

	std::variant<Object *, std::string> send(std::string message, std::optional<std::variant<Register, std::string, uint32_t>> stamp);

	template<class T, typename... Args>
	void add_store(std::string store_name, Args&&... args) {
		stores[store_name] = static_cast<InternalStore*>(new T(std::forward<Args>(args)...));
	}

	void add_default_stores(std::set<std::string> &stores) {
		for (auto store : stores)
			default_stores.insert(store);
	}

	bool is_default_store(std::string &store) { return default_stores.count(store) == 1; }

	std::string get_type() const { return type; }

	std::string to_string() const;
private:
	int hash;
	Object *prototype;
	std::string type;
	std::map<std::string, InternalStore*> stores;
	std::set<std::string> default_stores;
};