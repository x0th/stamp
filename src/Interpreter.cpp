/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>

#include "Interpreter.h"

void Interpreter::run() {
	uint32_t lexical_scope_index = 0;
	while (current_bb < generator.get_num_bbs()) {
		auto bb = generator.get_bbs()[current_bb];

		// add all lexical scopes that start with the current basic block index to the context
		LexicalScope *lscope = generator.get_scope(lexical_scope_index);
		while (lscope && lscope->starts_at(current_bb)) {
			if (current_bb == 0)
				scopes.add_scope(lscope, Context::make_global_context());
			else
				scopes.add_scope(lscope, new Context());
			lscope = generator.get_scope(++lexical_scope_index);
		}

		// execute instruction within the current basic block
		for (auto instruction : bb->get_instructions()) {
			instruction->execute(*this);
			if (should_terminate_bb)
				break;
		}

		if (should_terminate_bb) {
			should_terminate_bb = false;
			continue;
		}

		// remove all scopes that (lexically) end at the current basic block
		while(true) {
			if (!scopes.is_empty() && scopes.lexical_scopes.back()->ends_at(current_bb))
				scopes.pop_scope();
			else
				break;
		}
		current_bb++;
	}
}

Object *Interpreter::fetch_object(std::string &name) {
	for (auto context = scopes.contexts.rbegin(); context != scopes.contexts.rend(); context++) {
		auto obj = (*context)->get(name);
		if (obj)
			return obj;
	}
	// FIXME: Error!
	return nullptr;
}

Object *Interpreter::fetch_global_object(std::string name) {
	return scopes.contexts[0]->get(name);
}

void Interpreter::dump() {
	for (long unsigned int i = 0; i < reg_values.size(); i++) {
		auto r = reg_values[i];
		std::cout << "r" << i << " ";
		auto *str = std::get_if<std::string>(&r);
		if (str) {
			std::cout << *str << "\n";
		} else {
			auto obj = std::get<Object*>(r);
			if (obj)
				std::cout << obj->to_string();
			std::cout << "\n";
		}
	}
}