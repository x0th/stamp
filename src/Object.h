/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <algorithm>
#include <string>
#include <set>
#include <optional>
#include <map>
#include <variant>
#include <vector>
#include <sstream>

#include "Register.h"
#include "Error.h"

class Object;
class StoreObject;
class StoreLiteral;
class StoreInt;
class StoreChar;
class StoreVec;
class StoreRegister;
class Interpreter;

#define ENUMERATE_STORE_TYPES(T)       \
	T(StoreObject, StoreObject)        \
	T(StoreLiteral, StoreLiteral)      \
	T(StoreInt, StoreInt)              \
	T(StoreChar, StoreChar)            \
	T(StoreVec, StoreVec)              \
	T(StoreRegister, StoreRegister)

class InternalStore {
public:
	enum class Type {
#define __STORE_TYPES(t, c) \
    t,
		ENUMERATE_STORE_TYPES(__STORE_TYPES)
#undef __STORE_TYPES
	};

	InternalStore(Type type, bool is_mutable) : type(type), _is_mutable(is_mutable) {}

	Type get_type() const { return type; }
	bool is_mutable() const { return _is_mutable; }
	std::string to_string() const;
private:
	Type type;
	bool _is_mutable;
};

class StoreObject : public InternalStore {
public:
	StoreObject(Object *object, bool is_mutable) : InternalStore(Type::StoreObject, is_mutable), object(object) {}

	Object *unwrap() const { return object; }
	std::string to_string() const;
private:
	Object *object;
};

class StoreLiteral : public InternalStore {
public:
	StoreLiteral(std::string literal, bool is_mutable) : InternalStore(Type::StoreLiteral, is_mutable), literal(literal) {}

	std::string unwrap() const { return literal; }
	std::string to_string() const { return literal; }
private:
	std::string literal;
};

class StoreInt : public InternalStore {
public:
	StoreInt(int32_t integer, bool is_mutable) : InternalStore(Type::StoreInt, is_mutable), integer(integer) {}

	int32_t unwrap() const { return integer; }
	std::string to_string() const { return std::to_string(integer); }
private:
	int32_t integer;
};

class StoreChar : public InternalStore {
public:
	StoreChar(char c, bool is_mutable) : InternalStore(Type::StoreChar, is_mutable), c(c) {}

	char unwrap() const { return c; }
	std::string to_string() const { return std::to_string(c); }
private:
	char c;
};

class StoreVec : public InternalStore {
public:
	StoreVec(std::vector<InternalStore*> *vec, bool is_mutable) : InternalStore(Type::StoreVec, is_mutable), vec(vec) {}

	std::vector<InternalStore*> *unwrap() { return vec; }
	std::string to_string() const {
		std::stringstream s;
		s << "[";
		for (unsigned long int i = 0; i < (*vec).size(); i++) {
			auto is = (*vec)[i];
			s << is->to_string();
			if (i != (*vec).size() - 1)
				s << ", ";
		}
		s << "]";
		return s.str();
	}
private:
	std::vector<InternalStore*> *vec;
};

class StoreRegister : public InternalStore {
public:
	StoreRegister(uint32_t reg_index, bool is_mutable) : InternalStore(Type::StoreRegister, is_mutable), reg_index(reg_index) {}

	uint32_t unwrap() { return reg_index; }
	std::string to_string() const { return "r" + std::to_string(reg_index); }
private:
	uint32_t reg_index;
};

class Object {
public:
	Object(Object *prototype, std::string type) : prototype(prototype), type(type) {
		hash = rand();
	}

	std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*>
	        send(std::string message, std::optional<std::variant<Register, std::string, uint32_t>> stamp, Object *forwarder, Interpreter &interpreter);

	template<class T, typename... Args>
	void add_store(std::string store_name, Args&&... args) {
		if (!stores.empty() && stores.count(store_name) && !stores[store_name]->is_mutable()) {
			terminating_error(StampError::ExecutionError, "Cannot assign to immutable store " + store_name + " in object " + type + ".");
		} else {
			stores[store_name] = static_cast<InternalStore*>(new T(std::forward<Args>(args)...));
		}
	}

	InternalStore *get_store(std::string store_name) {
		if (stores.count(store_name))
			return stores[store_name];
		else
			return nullptr;
	}

	void add_default_stores(std::set<std::string> &stores) {
		for (auto store : stores)
			default_stores.insert(store);
	}

	bool is_default_store(std::string &store) { return default_stores.count(store) == 1; }

	std::string get_type() const { return type; }
	uint32_t get_hash() const { return hash; }
	Object *get_prototype() const { return prototype; }

	std::string to_string() const;
private:
	uint32_t hash;
	Object *prototype;
	std::string type;
	std::map<std::string, InternalStore*> stores;
	std::set<std::string> default_stores;
};