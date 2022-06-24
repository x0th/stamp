/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>

#include "AST.h"
#include "Generator.h"
#include "Register.h"
#include "Error.h"

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
		case Token::Program: {
			auto scope = generator.add_scope_beginning(0, true);
			token.type = Token::SList;
			generate_bytecode(generator);
			generator.end_scope(scope);
			break;
		}
		case Token::SList: {
			for (long unsigned int i = 0; i < children.size(); i++) {
				auto c = children[i];
				if (c->token.type == Token::SList) {
					auto scope = generator.add_scope_beginning_current_bb(0, true);
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
		case Token::Fn: {
			auto callable_obj = generator.next_register();
			generator.append<Load>(callable_obj, "Callable");
			auto callable_name = generator.next_register();
			std::optional<std::string> st = children[0]->token.value;
			generator.append<Send>(callable_name, callable_obj, "clone_callable", st);
			auto last_register = callable_name;
			for (long unsigned i = 1; i < children.size() - 1; i++) {
				std::optional<std::string> stamp = children[i]->token.value;
				auto temp_register = generator.next_register();
				generator.append<Send>(temp_register, last_register, "store_param", stamp);
				last_register = temp_register;
			}
			auto skip_function = generator.append<Jump>(0);
			auto scope = generator.add_scope_beginning(SCOPE_CAN_RETURN, false);
			std::optional<uint32_t> stamp = generator.get_num_bbs() - 1;
			children[children.size() - 1]->generate_bytecode(generator);
			generator.append<JumpSaved>();
			generator.end_scope(scope);
			skip_function->set_jump(generator.add_basic_block()->get_index());
			auto final = generator.next_register();
			generator.append<Send>(final, last_register, "pass_body", stamp);
			return final;
		}
		case Token::FnCall: {
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
			generator.add_basic_block();
			return final;
		};
		case Token::If: {
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
		case Token::While: {
			auto condition_bb = generator.add_basic_block();
			auto condition = children[0]->generate_bytecode(generator);
			auto ji = generator.append<JumpFalse>(*condition);
			auto body_scope = generator.add_scope_beginning(WHILE_SCOPE_FLAGS, false);
			body_scope->set_continue_dest(condition_bb->get_index());
			auto body = children[1]->generate_bytecode(generator);
			generator.append<Jump>(condition_bb->get_index());
			auto after_while = generator.add_basic_block()->get_index();
			body_scope->set_break_dest(after_while);
			generator.end_scope(body_scope);
			ji->set_jump(after_while);
			return body;
		}
		case Token::Break: {
			for (int i = generator.get_num_scopes() - 1; i >= 0; i--) {
				auto lscope = generator.get_scope(i);
				if (lscope->can_break) {
					lscope->add_pending_break(generator.append<Jump>(0));
					return {};
				}
			}
			terminating_error(StampError::BytecodeGenerationError, token.position() + ": break used outside of statement that can break.");
			break;
		}
		case Token::Continue: {
			for (int i = generator.get_num_scopes() - 1; i >= 0; i--) {
				auto lscope = generator.get_scope(i);
				if (lscope->can_continue) {
					generator.append<Jump>(lscope->get_continue_dest());
				}
			}
			terminating_error(StampError::BytecodeGenerationError, token.position() + ": continue used outside of statement that can break.");
			break;
		}
		case Token::Vec: {
			auto vec = generator.next_register();
			generator.append<Load>(vec, "Vec");

			std::optional<std::string> clone_stamp = "::lit_vec";
			auto dst = generator.next_register();
			generator.append<Send>(dst, vec, "clone", clone_stamp);

			for (auto val : children) {
				auto val_reg = val->generate_bytecode(generator);
				auto temp = generator.next_register();
				generator.append<Send>(temp, dst, "push", val_reg);
				dst = temp;
			}

			return dst;
		}
		case Token::Store: {
			auto obj = children[0]->generate_bytecode(generator);
			if (!obj.has_value()) {
				terminating_error(StampError::BytecodeGenerationError, token.position() + ": attempted to store to not an object.");
			}
			auto rhs = children[2]->generate_bytecode(generator);
			if (!rhs.has_value()) {
				terminating_error(StampError::BytecodeGenerationError, token.position() + ": attempted to store not an object.");
			}
			bool is_mutable = children.size() == 4 && children[3]->token.type == Token::Mut;
			generator.append<Store>(*obj, children[1]->token.value, *rhs, is_mutable);
			return {};
		}
		case Token::Send: {
			auto obj = children[0]->generate_bytecode(generator);
			if (!obj.has_value()) {
				terminating_error(StampError::BytecodeGenerationError, token.position() + ": attempted to send to not an Object.");
			}
			if (children[1]->children.size() != 0) {
				if (children[1]->get_children()[0]->token.type == Token::Object || children[1]->get_children()[0]->token.type == Token::Value) {
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
		case Token::Object: {
			auto dst = generator.next_register();
			generator.append<Load>(dst, token.value);
			return dst;
		}
		ENUMERATE_BASIC_OBJECTS(__GENERATE_BASIC_OBJECT)
		default: {
			terminating_error(StampError::BytecodeGenerationError,  "No bytecode generation implemented for " + token.token_str() + " at " + token.position());
		}
	}
	return {};

#undef __GENERATE_BASIC_OBJECT
}

std::string ASTNode::to_string_indent(std::string indent) {
	std::stringstream s;
	s << indent << token.token_str() << "\n";
	for (auto &c : children) {
		s << c->to_string_indent(indent + "  ");
	}
	return s.str();
}