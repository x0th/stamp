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
		auto it = reg_values.begin() + register_index;
		reg_values.insert(it, value);
	}

	std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*> &at(uint32_t register_index) {
		return reg_values[register_index];
	}

	inline void jump_bb(uint32_t bb_index) {
		current_bb = bb_index;
		should_terminate_bb = true;
	}

	void put_object(std::string name, Object *object) {
		scopes.contexts[scopes.contexts.size() - 1]->add(name, object);
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
	std::vector<std::variant<Object *, std::string, int32_t, std::vector<InternalStore*>*>> reg_values;
	Scopes scopes;
};