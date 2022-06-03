/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <variant>
#include <optional>

#include "Register.h"

#define ENUMERATE_INSTRUCTION_TYPES(T)       \
	T(Send)                                  \
	T(Load)                                  \
	T(Store)

class Instruction {
public:
	enum class Type {
#define __INSTRUCTION_TYPES(t) \
	t,
		ENUMERATE_INSTRUCTION_TYPES(__INSTRUCTION_TYPES)
#undef __INSTRUCTION_TYPES
	};

	Instruction(Type type) : type(type) {}
	void dealloc();

	Type get_type() const { return type; }

	std::string to_string() const;
private:
	Type type;
};

class Load final : public Instruction {
public:
	Load(Register dst, std::string value) : Instruction(Type::Load), dst(dst), value(value) {}

	std::string to_string() const;
private:
	Register dst;
	std::string value;
};

class Send final : public Instruction {
public:
	Send(Register dst, Register obj, std::string msg, std::optional<std::variant<Register, std::string>> stamp) :
		Instruction(Type::Send), dst(dst), obj(obj), msg(msg), stamp(stamp) {}

	std::string to_string() const;
private:
	Register dst;
	Register obj;
	std::string msg;
	std::optional<std::variant<Register, std::string>> stamp;
};

class Store final : public Instruction {
public:
	Store(Register obj, std::string store_name, Register store) :
		Instruction(Type::Store), obj(obj), store_name(store_name), store(store) {}

	std::string to_string() const;
private:
	Register obj;
	std::string store_name;
	Register store;
};