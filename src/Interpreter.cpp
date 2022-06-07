/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <iostream>

#include "Interpreter.h"

void Interpreter::run() {
	uint32_t lexical_scope_index = 0;
	for (auto bb : generator.get_bbs()) {
		uint32_t bb_index = bb->get_index();

		// add all lexical scopes that start with the current basic block index to the context
		LexicalScope *lscope = generator.get_scope(lexical_scope_index);
		while (lscope && lscope->starts_at(bb->get_index())) {
			if (bb_index == 0)
				scopes.add_scope(lscope, Context::make_global_context());
			else
				scopes.add_scope(lscope, new Context());
			lscope = generator.get_scope(++lexical_scope_index);
		}

		// execute instruction within the current basic block
		for (auto instruction : bb->get_instructions()) {
			instruction->execute(*this);
		}

		// remove all scopes that (lexically) end at the current basic block
		while(true) {
			if (!scopes.is_empty() && scopes.lexical_scopes.back()->ends_at(bb_index))
				scopes.pop_scope();
			else
				break;
		}
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

void Interpreter::dump() {
	for (long unsigned int i = 0; i < reg_values.size(); i++) {
		auto r = reg_values[i];
		std::cout << "r" << i << " ";
		auto *str = std::get_if<std::string>(&r);
		if (str) {
			std::cout << *str << "\n";
		} else {
			std::cout << std::get<Object*>(r)->to_string() << "\n";
		}
	}
}