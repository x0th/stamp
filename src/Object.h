/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <vector>

#define ENUMERATE_STORE_TYPES(T)       \
	T(StoreObject)                     \
	T(StoreLiteral)

class Object;

class InternalStore {
public:
	enum class Type {
#define __STORE_TYPES(t) \
    t,
		ENUMERATE_STORE_TYPES(__STORE_TYPES)
#undef __STORE_TYPES
	};

	InternalStore(Type type) : type(type) {}
private:
	Type type;
};

class StoreObject : public InternalStore {
public:
	StoreObject(Object *object) : InternalStore(Type::StoreObject), object(object) {}

private:
	Object *object;
};

class StoreLiteral : public InternalStore {
public:
	StoreLiteral(std::string literal) : InternalStore(Type::StoreLiteral), literal(literal) {}

private:
	std::string literal;
};

class Object {
public:
	Object(Object *prototype, std::string type) : prototype(prototype), type(type) {
		hash = rand();
	}

	std::string to_string() const;
private:
	int hash;
	Object *prototype;
	std::string type;
	std::vector<InternalStore> stores;
};