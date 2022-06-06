/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <variant>
#include <optional>
#include <cinttypes>

#include "Register.h"
#include "Interpreter.h"

class Interpreter;

#define ENUMERATE_INSTRUCTION_TYPES(T)       \
	T(Send)                                  \
	T(Load)                                  \
	T(Store)                                 \
    T(Jump)                                  \
	T(JumpTrue)                              \
	T(JumpFalse)

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
	// FIXME: should be error or void
	void execute(Interpreter &interpreter);
private:
	Type type;
};

class Load final : public Instruction {
public:
	Load(Register dst, std::string value) : Instruction(Type::Load), dst(dst), value(value) {}

	std::string to_string() const;
	void execute(Interpreter &interpreter);
private:
	Register dst;
	std::string value;
};

class Send final : public Instruction {
public:
	Send(Register dst, Register obj, std::string msg, std::optional<Register> st) : Instruction(Type::Send), dst(dst), obj(obj), msg(msg) {
		if (st)
			stamp = *st;
	}
	Send(Register dst, Register obj, std::string msg, std::optional<std::string> st) : Instruction(Type::Send), dst(dst), obj(obj), msg(msg) {
		if (st)
			stamp = *st;
	}
	Send(Register dst, Register obj, std::string msg, std::optional<uint32_t> st) : Instruction(Type::Send), dst(dst), obj(obj), msg(msg) {
		if (st)
			stamp = *st;
	}
	Send(const Send& other) : Instruction(Type::Send), dst(other.dst), obj(other.obj), msg(other.msg), stamp(other.stamp) {}

	std::string to_string() const;
	void execute(Interpreter &interpreter);
private:
	Register dst;
	Register obj;
	std::string msg;
	std::optional<std::variant<Register, std::string, uint32_t>> stamp;
};

class Store final : public Instruction {
public:
	Store(Register obj, std::string store_name, Register store) :
		Instruction(Type::Store), obj(obj), store_name(store_name), store(store) {}

	std::string to_string() const;
	void execute(Interpreter &interpreter);
private:
	Register obj;
	std::string store_name;
	Register store;
};

class Jump final : public Instruction {
public:
	Jump(uint32_t block_index) : Instruction(Type::Jump), block_index(block_index) {}

	std::string to_string() const;
	void execute(Interpreter &interpreter);
private:
	uint32_t block_index;
};

class JumpTrue final : public Instruction {
public:
	JumpTrue(uint32_t block_index, Register condition) : Instruction(Type::JumpTrue), block_index(block_index), condition(condition) {}
	JumpTrue(Register condition) : Instruction(Type::JumpTrue), block_index(0), condition(condition) {}

	void set_jump(uint32_t jump_location) { block_index = jump_location; }

	std::string to_string() const;
	void execute(Interpreter &interpreter);
private:
	uint32_t block_index;
	Register condition;
};

class JumpFalse final : public Instruction {
public:
	JumpFalse(uint32_t block_index, Register condition) : Instruction(Type::JumpFalse), block_index(block_index), condition(condition) {}
	JumpFalse(Register condition) : Instruction(Type::JumpFalse), block_index(0), condition(condition) {}

	void set_jump(uint32_t jump_location) { block_index = jump_location; }

	std::string to_string() const;
	void execute(Interpreter &interpreter);
private:
	uint32_t block_index;
	Register condition;
};