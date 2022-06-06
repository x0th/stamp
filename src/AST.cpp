/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "AST.h"
#include "Generator.h"
#include "Register.h"

#define WHILE_SCOPE_FLAGS SCOPE_CAN_CONTINUE | SCOPE_CAN_BREAK

std::optional<Register> ASTNode::generate_bytecode(Generator &generator) {
#define __GENERATE_BASIC_OBJECT(tok, lit)                                                        \
            case tok: {                                                                          \
        auto obj = generator.next_register();                                                    \
		generator.append<Load>(obj, lit);                                                        \
																								 \
		std::optional<std::string> clone_stamp = "::lit_" + token.value; \
		auto clone_dst = generator.next_register();                                              \
		generator.append<Send>(clone_dst, obj, "clone", clone_stamp);                            \
                                                                                                 \
		std::optional<std::string> stamp = token.value;                  \
        auto dst = generator.next_register();                                                    \
		generator.append<Send>(dst, clone_dst, "store_value", stamp);                            \
        return dst;                                                                              \
		}

	switch (token.type) {
		case TokProgram: {
			auto scope = generator.add_scope_beginning(0);
			token.type = TokSList;
			generate_bytecode(generator);
			generator.end_scope(scope);
			break;
		}
		case TokSList: {
			for (long unsigned int i = 0; i < children.size(); i++) {
				auto c = children[i];
				if (c->token.type == TokSList) {
					auto scope = generator.add_scope_beginning_current_bb(0);
					c->generate_bytecode(generator);
					generator.end_scope(scope);

					// child was a beginning of the scope it's not the last one of the children
					if (i != children.size() - 1)
						generator.add_basic_block();
				} else {
					c->generate_bytecode(generator);
				}
			}
			break;
		}
		case TokFn: {
			auto callable_obj = generator.next_register();
			generator.append<Load>(callable_obj, "Callable");
			auto callable_name = generator.next_register();
			std::optional<std::string> st = children[0]->token.value;
			generator.append<Send>(callable_name, callable_obj, "clone", st);
			auto last_register = callable_name;
			for (long unsigned i = 1; i < children.size() - 1; i++) {
				std::optional<std::string> stamp = children[i]->token.value;
				auto temp_register = generator.next_register();
				generator.append<Send>(temp_register, last_register, "store_param", stamp);
				last_register = temp_register;
			}
			auto scope = generator.add_scope_beginning(SCOPE_CAN_RETURN);
			std::optional<uint32_t> stamp = generator.get_num_bbs() - 1;
			children[children.size() - 1]->generate_bytecode(generator);
			generator.end_scope(scope);
			generator.add_basic_block();
			auto final = generator.next_register();
			generator.append<Send>(final, last_register, "pass_body", stamp);
			return final;
		}
		case TokFnCall: {
			auto last_register = *children[0]->generate_bytecode(generator);
			for (long unsigned int i = 1; i < children.size(); i++) {
				std::optional<std::string> stamp = children[i]->token.value;
				auto temp_register = generator.next_register();
				generator.append<Send>(temp_register, last_register, "pass_param", stamp);
				last_register = temp_register;
			}
			auto final = generator.next_register();
			std::optional<Register> stamp = {};
			generator.append<Send>(final, last_register, "call", stamp);
			return final;
		};
		case TokIf: {
			auto condition = children[0]->generate_bytecode(generator);
			auto ji = generator.append<JumpFalse>(*condition);
			auto true_condition = children[1]->generate_bytecode(generator);
			if (children.size() == 3) {
				ji->set_jump(generator.get_num_bbs());
				children[2]->generate_bytecode(generator);
			} else {
				ji->set_jump(generator.add_basic_block()->get_index());
			}
			return true_condition;
		}
		case TokWhile: {
			auto condition_bb = generator.add_basic_block();
			auto condition = children[0]->generate_bytecode(generator);
			auto ji = generator.append<JumpFalse>(*condition);
			auto body_scope = generator.add_scope_beginning(WHILE_SCOPE_FLAGS);
			body_scope->set_continue_dest(condition_bb->get_index());
			auto body = children[1]->generate_bytecode(generator);
			generator.append<Jump>(condition_bb->get_index());
			auto after_while = generator.add_basic_block()->get_index();
			body_scope->set_break_dest(after_while);
			generator.end_scope(body_scope);
			ji->set_jump(after_while);
			return body;
		}
		case TokBreak: {
			for (int i = generator.get_num_scopes() - 1; i >= 0; i--) {
				auto lscope = generator.get_scope(i);
				if (lscope->can_break) {
					lscope->add_pending_break(generator.append<Jump>(0));
					return {};
				}
			}
			// FIXME: error!
			break;
		}
		case TokContinue: {
			for (int i = generator.get_num_scopes() - 1; i >= 0; i--) {
				auto lscope = generator.get_scope(i);
				if (lscope->can_continue) {
					generator.append<Jump>(lscope->get_continue_dest());
				}
			}
			// FIXME: error!
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
			if (children[1]->children.size() != 0) {
				if (children[1]->get_children()[0]->token.type != TokSend) {
					std::optional<std::string> stamp = children[1]->get_children()[0]->token.value;
					auto dst = generator.next_register();
					generator.append<Send>(dst, *obj, children[1]->token.value, stamp);
					return dst;
				} else {
					std::optional<Register> stamp = children[1]->get_children()[0]->generate_bytecode(generator);
					auto dst = generator.next_register();
					generator.append<Send>(dst, *obj, children[1]->token.value, stamp);
					return dst;
				}
			} else {
				std::optional<Register> stamp = {};
				auto dst = generator.next_register();
				generator.append<Send>(dst, *obj, children[1]->token.value, stamp);
				return dst;
			}
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