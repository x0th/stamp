/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>
#include <string>
#include <time.h>

#include "Generator.h"
#include "Object.h"
#include "Context.h"

class Generator;
class LexicalScope;

class Interpreter {
public:
	Interpreter(Generator &generator) : generator(generator) {
		srand(time(nullptr));
	}

	void dump();

	void run();

	void store_at(uint32_t register_index, std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> value) {
		// if register index is beyond the current allocated registers, fill the register vector
		for (long unsigned int i = reg_values.size(); i < (long unsigned int) register_index; i++)
			reg_values.push_back(std::nullopt);

		auto it = reg_values.begin() + register_index;
		reg_values.insert(it, value);
	}

	Register store_at_next_available(std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> value) {
		auto next_register = generator.next_register();
		auto it = reg_values.begin() + next_register.get_index();
		reg_values.insert(it, value);
		return next_register;
	}

	std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> &at(uint32_t register_index) {
		if (!reg_values[register_index])
			terminating_error(StampError::ExecutionError, "Attempted to read an empty register.");
		return *reg_values[register_index];
	}

	inline void jump_bb(uint32_t bb_index) {
		current_bb = bb_index;
		should_terminate_bb = true;
	}

	inline void save_next_bb() {
		saved_bbs.push_back(current_bb + 1);
	}

	inline void jump_saved_bb() {
		uint32_t bb_index = saved_bbs[saved_bbs.size() - 1];
		// remove all scopes that (lexically) end at the current basic block
		while(true) {
			if (!scopes.is_empty() && scopes.lexical_scopes.back()->ends_at(current_bb))
				scopes.pop_scope();
			else
				break;
		}
		saved_bbs.pop_back();
		jump_bb(bb_index);
	}

	void put_object(std::string name, Object *object) {
		if (in_global_scope)
			global_scope.contexts[0]->add(name, object);
		else
			scopes.contexts[scopes.contexts.size() - 1]->add(name, object);
	}

	void put_object_at_scope(std::string name, Object *object, uint32_t bb_index) {
		for (unsigned long int i = 0; i < scopes.lexical_scopes.size(); i++) {
			if (scopes.lexical_scopes[i]->starts_at(bb_index)) {
				scopes.contexts[i]->add(name, object);
			}
		}
		if (in_global_scope)
			global_scope.contexts[0]->add(name, object);
		else
			scopes.contexts[scopes.contexts.size() - 1]->add(name, object);
	}

	void push_retval(std::optional<Register> ret) {
		// FIXME: if this fails for some reason (e.g. multithreading), the data structure for return value should be changed
		if (retval) {
			terminating_error(StampError::ExecutionError, "A previous return value would be overwritten.");
			return;
		}
		retval = ret;
	}

	std::optional<Register> pop_retval() {
		auto ret = retval;
		retval = {};
		return ret;
	}

	Object *fetch_object(std::string &name);
	Object *fetch_global_object(std::string name);
private:
	class Scopes {
	public:
		Scopes() {}

		std::vector<LexicalScope*> lexical_scopes;
		std::vector<Context*> contexts;

		void add_scope(LexicalScope *lscope, Context *context) {
			lexical_scopes.push_back(lscope);
			contexts.push_back(context);
		}

		void add_lscope(LexicalScope *lscope) {
			lexical_scopes.push_back(lscope);
		}

		void pop_scope() {
			lexical_scopes.pop_back();
			contexts.pop_back();
		}

		bool is_empty() {
			return lexical_scopes.size() == 0;
		}
	};

	bool should_terminate_bb = { false };
	uint32_t current_bb = { 0 };
	Generator &generator;
	std::vector<std::optional<std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*>>> reg_values;
	Scopes scopes;
	Scopes global_scope;
	bool in_global_scope = { false };
	std::vector<uint32_t> saved_bbs;
	std::optional<Register> retval;
};