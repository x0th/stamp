/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "AST.h"
#include "Generator.h"

std::optional<Register> ASTNode::generate_bytecode(Generator &generator) {
	switch (token.type) {
		case TokSList: {
			for (auto const c: children) {
				c->generate_bytecode(generator);
			}
			break;
		}
		case TokStore: {
			auto obj = children[0]->generate_bytecode(generator);
			if (!obj.has_value()) {
				// FIXME: Error!
			}
			auto rhs = children[2]->generate_bytecode(generator);
			if (!rhs.has_value()) {
				// FIXME: Error!
			}
			generator.append<Store>(*obj, children[1]->token.value, *rhs);
			return {};
		}
		case TokSend: {
			auto obj = children[0]->generate_bytecode(generator);
			if (!obj.has_value()) {
				// FIXME: Error!
			}
			auto dst = generator.next_register();
			generator.append<Send>(dst, *obj, children[1]->token.value);
			return dst;
		}
		case TokObject: {
			auto dst = generator.next_register();
			generator.append<Load>(dst, token.value);
			return dst;
		}
		default: {
			// FIXME: Error!
		}
	}
	return {};
}

std::string ASTNode::to_string_indent(std::string indent) {
	std::stringstream s;
	s << indent << token_str(&token) << "\n";
	for (auto &c : children) {
		s << c->to_string_indent(indent + "  ");
	}
	return s.str();
}