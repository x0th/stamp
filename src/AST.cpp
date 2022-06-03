/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "AST.h"
#include "Generator.h"
#include "Register.h"

std::optional<Register> ASTNode::generate_bytecode(Generator &generator) {
#define __GENERATE_BASIC_OBJECT(tok, lit)                                                        \
            case tok: {                                                                          \
        auto obj = generator.next_register();                                                    \
		generator.append<Load>(obj, lit);                                                        \
																								 \
		std::optional<std::variant<Register, std::string>> clone_stamp = "::lit_" + token.value; \
		auto clone_dst = generator.next_register();                                              \
		generator.append<Send>(clone_dst, obj, "clone", clone_stamp);                            \
                                                                                                 \
		std::optional<std::variant<Register, std::string>> stamp = token.value;                  \
        auto dst = generator.next_register();                                                    \
		generator.append<Send>(dst, clone_dst, "store_value", stamp);                            \
        return dst;                                                                              \
		}

	switch (token.type) {
		case TokSList: {
			auto this_scope = generator.add_scope_beginning(0);
			for (long unsigned int i = 0; i < children.size(); i++) {
				auto c = children[i];
				c->generate_bytecode(generator);

				// child was a beginning of the scope it's not the last one of the children
				if (c->token.type == TokSList && i != children.size() - 1)
					generator.add_basic_block();
			}
			generator.end_scope(this_scope);
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
			std::optional<std::variant<Register, std::string>> stamp = {};
			if (children[1]->children.size() != 0) {
				if (children[1]->get_children()[0]->token.type != TokSend)
					stamp = children[1]->get_children()[0]->token.value;
				else
					stamp = children[1]->get_children()[0]->generate_bytecode(generator);
			}
			auto dst = generator.next_register();
			generator.append<Send>(dst, *obj, children[1]->token.value, stamp);
			return dst;
		}
		case TokObject: {
			auto dst = generator.next_register();
			generator.append<Load>(dst, token.value);
			return dst;
		}
		ENUMERATE_BASIC_OBJECTS(__GENERATE_BASIC_OBJECT)
		default: {
			// FIXME: Error!
		}
	}
	return {};

#undef __GENERATE_BASIC_OBJECT
}

std::string ASTNode::to_string_indent(std::string indent) {
	std::stringstream s;
	s << indent << token_str(&token) << "\n";
	for (auto &c : children) {
		s << c->to_string_indent(indent + "  ");
	}
	return s.str();
}