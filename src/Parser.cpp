/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Parser.h"
#include "Lexer.h"
//#include "context.h"

#include <iostream>
#include <utility>

using namespace std;

vector<string> file;
string filename;
long unsigned int line_number;
string raw_str;
long unsigned int position;
Token tok;

ASTNode *parse_program();
void parse_statement_list(ASTNode *s);
ASTNode *parse_statement();
ASTNode *parse_statement_tail(ASTNode *object);
ASTNode *parse_message_tail(ASTNode *previous_message);
ASTNode *parse_parameters(ASTNode *s);
ASTNode *parse_function_signature(ASTNode *s);
ASTNode *parse_param_type(ASTNode *param);
ASTNode *parse_next_param(ASTNode *so_far);
ASTNode *parse_function_tail(ASTNode *function);
ASTNode *parse_rhs(ASTNode *object);
ASTNode *parse_statement_rhs();
ASTNode *parse_if();
ASTNode *parse_if_tail();
ASTNode *parse_else_tail();
ASTNode *parse_while();
ASTNode *parse_list(ASTNode *list);

#define clone_obj(obj_name)                                                                                     \
	vector<ASTNode *> children;                                                                                 \
	children.push_back(new ASTNode(Token { type: TokObject, value: obj_name }));                                \
	children.push_back(new ASTNode(Token { type: TokMessage, value: "clone" }));                                \
	children[1]->get_children().push_back(object);                                                              \
	auto cloned = new ASTNode(Token { type: TokSend, value: "" }, children);                                    \

string error_msg(string error) {
	string out;
	if (filename != "") {
		out += filename + ":" + std::to_string(line_number+1) + ":"; 
	} else {
		for (long unsigned int i = 0; i < position+1; i++)
			out += " ";
		out += "^\n";
	}

	out += std::to_string(position+1) + ": " + error + "\n";

	return out;
}

inline void next_token() {
	if (raw_str.size() > position)
		tok = scan(raw_str, &position);
	else
		tok = Token({ type: TokEOF, value: "" });
}

inline void match(TokenType expected) {
	next_token();
	if (tok.type != expected) {
		Token t = Token({ type: expected, value: "" });
		throw error_msg(error_msg("Expected " + token_readable(&t) + ". Found: " + token_readable(&tok)));
	}
}

bool request_line() {
	if (line_number + 1 >= file.size())
		return false;
	
	raw_str = file[++line_number];
	position = 0;
	next_token();
	
	return true;
}

ASTNode *parse_program() {
	ASTNode *s = new ASTNode(Token { type: TokSList, value: "" });
	try {
		next_token();
		parse_statement_list(s);
	} catch (string &msg) {
		cerr << msg;
		return nullptr;
	}

	return s;
}

void parse_statement_list(ASTNode *s) {
	ASTNode *st;
	switch (tok.type) {
		case TokStatementEnd: {
			next_token();
			parse_statement_list(s);
			return;
		}
		case TokObject:
		case TokValue:
		case TokFn:
		case TokIf:
		case TokWhile:
		case TokInt:
		case TokChar:
		case TokString:
		case TokSqBracketL:
		case TokMessage:
		{
			st = parse_statement();
			if (st) {
				s->get_children().push_back(st);
			}
			parse_statement_list(s);
			return;
		}
		case TokEOF:
			if (request_line())
				parse_statement_list(s);
			return;
		case TokSListBegin: {
			ASTNode *new_slist = new ASTNode(Token { type: TokSList, value: "" });
			next_token();
			parse_statement_list(new_slist);
			if (new_slist)
				s->get_children().push_back(new_slist);
			parse_statement_list(s);
			return;
		}
		case TokSListEnd:
			next_token();
			return;
		default:
			return;
	}
}

bool is_operator(string &s) {
//	auto lst = global_context->get("Operators")->get_obj()->get_stores()["table"]->get_obj()->get_stores()["value"]->get_list();
//
//	for (long unsigned int i = 0; i < lst->size(); i++) {
//		auto p = (*lst)[i]->get_list();
//		for (auto op : *p) {
//			if (*op->get_lit() == s) {
//				return true;
//			}
//		}
//	}

	return false;
}

ASTNode *parse_statement() {
	switch (tok.type) {
		case TokInt:
		case TokChar:
		case TokString:
		case TokObject: {
			auto object = new ASTNode(tok);
			next_token(); // obj
			return parse_statement_tail(object);
		}
		case TokValue: {
			if (is_operator(tok.value)) {
				tok.type = TokMessage;
				return parse_statement();
			}

			auto object = new ASTNode(Token { type: TokObject, value: tok.value });
			next_token(); // obj
			return parse_statement_tail(object);
		}
//		case TokSqBracketL: {
//			create_basic_obj(TokList, "List");
//			next_token(); // [
//			return parse_message_tail(parse_list(out));
//		}
		case TokFn: {
			next_token(); // fn
			auto object = new ASTNode(Token { type: TokValue, value: tok.value });
			clone_obj("Callable");
			next_token(); // fn_name
			return parse_function_tail(cloned);
		}
		case TokIf: {
			return parse_if();
		}
		case TokWhile: {
			return parse_while();
		}
		case TokMessage: {
			if (!is_operator(tok.value)) {
				throw error_msg("Message at the start of statement was not an operator.");
			}

			vector<ASTNode *> children;
			auto msg = tok;
			next_token();
			children.push_back(parse_statement());
			children.push_back(new ASTNode(msg));
			return new ASTNode(Token{ type: TokSend, value: "" }, children);
		}
		default:
			return nullptr;
	}
}

ASTNode *parse_function_tail(ASTNode *function) {
	switch(tok.type) {
		case TokOpenParend: {
			next_token(); // (
			vector<ASTNode *> children;
			children.push_back(parse_function_signature(function));
			next_token(); // )
			vector<ASTNode *> msg_children;
			msg_children.push_back(parse_program());
			children.push_back(new ASTNode(Token { type: TokMessage, value: "store_body" }, msg_children));
			return new ASTNode(Token { type: TokSend, value: "" }, children);
		}
		case TokEOF: {
			if (request_line())
				return parse_function_tail(function);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected (. Found " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_statement_tail(ASTNode *object) {
	switch (tok.type) {
		case TokMessage:
		case TokObject:
		case TokOpenParend: {
			return parse_message_tail(object);
		}
		case TokValue: {
			vector<ASTNode *>children;
			children.push_back(object);
			children.push_back(new ASTNode(tok));
			match(TokStore); // =
			next_token();
			children.push_back(parse_rhs(children[1]));
			return new ASTNode(Token { type: TokStore, value: "" }, children);
		}
		case TokStore: {
			next_token();
			auto rhs = parse_rhs(object);
			rhs->get_children()[1]->get_children().push_back(object);
			return rhs;
		}
		case TokSListEnd:
			return object;
		case TokStatementEnd:
		case TokEOF:
		case TokComa:
		case TokCloseParend:
			next_token();
			return object;
		default:
			throw error_msg("Expected Object, value, message, =, (, ), ;, } or ,. Found: " + token_readable(&tok) + ".");
	};
}

ASTNode *parse_list(ASTNode *list) {
	switch(tok.type) {
		case TokSqBracketR: {
			next_token(); return list;
		}
		case TokComa:
			next_token(); return parse_list(list);
		case TokObject:
		case TokValue:
		case TokInt:
		case TokChar:
		case TokString:
		case TokSqBracketL: {
			vector<ASTNode *> msg_children;
			msg_children.push_back(list);
			msg_children.push_back(new ASTNode(Token({ type: TokMessage, value: "push" })));
			msg_children[1]->get_children().push_back(parse_statement_rhs());
			return parse_list(new ASTNode(Token({ type: TokSend, value: "" }), msg_children));
		}
		case TokEOF: {
			if (request_line())
				return parse_list(list);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Object, value, [ or ]. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_rhs(ASTNode *object) {
	switch(tok.type) {
		case TokFn: {
			next_token(); // fn
			clone_obj("Callable");
			return parse_function_tail(cloned);
		}
		case TokInt:
		case TokObject:
		case TokValue:
		case TokChar:
		case TokString:
		case TokSqBracketL:
			return parse_statement_rhs();
		case TokEOF: {
			if (request_line())
				return parse_rhs(object);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected function declaration, Object or value. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_statement_rhs() {
	switch (tok.type) {
		case TokInt:
		case TokChar:
		case TokSqBracketL:
		case TokObject: {
			auto object = new ASTNode(tok);
			next_token(); // obj
			return parse_message_tail(object);
		}
		case TokValue: {
			auto object = new ASTNode(Token { type: TokObject, value: tok.value });
			next_token(); // obj
			return parse_message_tail(object);
		}
//		case TokSqBracketL: {
//			create_basic_obj(TokList, "List");
//			next_token(); // [
//			return parse_message_tail(parse_list(out));
//		}
		case TokCloseParend:
			return nullptr;
		default:
			throw error_msg("Expected Object or value. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_parameters(ASTNode *s) {
	ASTNode *st;
	ASTNode *further = s;
	switch (tok.type) {
		case TokCloseParend:
			return s;
		case TokComa:
			next_token();
			return parse_parameters(s);
		case TokObject:
		case TokValue:
		case TokInt:
		case TokChar:
		case TokString:
		case TokSqBracketL:
			st = parse_statement_rhs();
			if (st) {
				vector<ASTNode *> children;
				children.push_back(s);
				ASTNode *message = new ASTNode(Token { type: TokMessage, value: "pass_param" });
				message->get_children().push_back(st);
				children.push_back(message);
				further = new ASTNode(Token { type: TokSend, value: "" }, children);
			}
			return parse_parameters(further);
		case TokSListBegin: {
			st = parse_program();
			if (st) {
				vector<ASTNode *> children;
				children.push_back(s);
				ASTNode *message = new ASTNode(Token { type: TokMessage, value: "pass_param" });
				message->get_children().push_back(st);
				children.push_back(message);
				further = new ASTNode(Token { type: TokSend, value: "" }, children);	
			}
			return parse_parameters(further);
		}
		default:
			throw error_msg("Expected Object, value, ) or ,. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_function_signature(ASTNode *function) {
	switch (tok.type) {
		case TokCloseParend:
			return function;
		case TokValue:
		{
			vector<ASTNode *> children;
			children.push_back(function);
			ASTNode *message = new ASTNode(Token { type: TokMessage, value: "store_param" });
			message->get_children().push_back(new ASTNode(tok));
			children.push_back(message);
			next_token();
			return parse_param_type(new ASTNode(Token { type: TokSend, value: "" }, children));
		}
		case TokEOF: {
			if (request_line())
				return parse_function_signature(function);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected value or ). Found: " + token_readable(&tok) + ".");
	}	
}

ASTNode *parse_param_type(ASTNode *param) {
	switch (tok.type) {
		//case TokColon:
			// FIXME: param type
		case TokComa:
		case TokCloseParend:
			return parse_next_param(param);
		case TokEOF: {
			if (request_line())
				return parse_param_type(param);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected , or ). Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_next_param(ASTNode *so_far) {
	switch (tok.type) {
		case TokComa:
			next_token();
			return parse_function_signature(so_far);
		case TokCloseParend:
			return so_far;
		default:
			throw error_msg("Expected , or ). Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_if() {
	switch (tok.type) {
		case TokIf: {
			auto if_ast = new ASTNode(tok);
			next_token();
			auto full_param = parse_statement_rhs();
			auto true_branch = parse_program();
			auto false_branch = parse_if_tail();

			if_ast->get_children().push_back(full_param);
			if_ast->get_children().push_back(true_branch);
			if (false_branch)
				if_ast->get_children().push_back(false_branch);

			return if_ast;
		}
		default:
			throw error_msg("Expected if. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_if_tail() {
	switch (tok.type) {
		case TokElse: {
			next_token();
			return parse_else_tail();
		}
		case TokIf:
		case TokObject:
		case TokValue:
		case TokInt:
		case TokString:
		case TokChar:
		case TokWhile:
		case TokSListEnd:
			return nullptr;
		case TokEOF:
			if (request_line())
				return parse_if_tail();
			return nullptr;
		default:
			throw error_msg("Expected if, else, Object or value. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_else_tail() {
	switch (tok.type) {
		case TokIf:
			return parse_if();
		case TokSListBegin:
			return parse_program();
		case TokEOF:
			if (request_line())
				return parse_else_tail();
			throw "Unexpected end of file.";
		default:
			throw error_msg("Expected if or {. Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_while() {
	switch (tok.type) {
		case TokWhile: {
			match(TokOpenParend); // (
			next_token(); // obj
			auto full_param = parse_statement_rhs();
			auto body = parse_program();

			vector<ASTNode *> children;
			children.push_back(new ASTNode(Token { type: TokObject, value: "while" }));

			vector<ASTNode *> list_children;
			list_children.push_back(full_param);
			list_children.push_back(body);

			auto msg = new ASTNode(Token { type: TokMessage, value: "exec_while_true" });
			msg->get_children().push_back(new ASTNode(Token { type: TokSList, value: "" }, list_children));

			children.push_back(msg);
			return new ASTNode(Token { type: TokSend, value: "" }, children);
		}
		default:
			throw error_msg("Expected while. Found: " + token_readable(&tok) + ".");
	}
}

inline bool swap_precedence(string &old_operator, string &new_operator) {
//	int precedence_old = -1, precedence_new = -1;
//	auto lst = global_context->get("Operators")->get_obj()->get_stores()["table"]->get_obj()->get_stores()["value"]->get_list();
//
//	for (long unsigned int i = 0; i < lst->size(); i++) {
//		auto p = (*lst)[i]->get_list();
//		for (auto op : *p) {
//			if (*op->get_lit() == old_operator) {
//				precedence_old = i;
//			}
//			if (*op->get_lit() == new_operator) {
//				precedence_new = i;
//			}
//		}
//	}
//
//	if (precedence_old <= precedence_new)
//		return false;
	return true;
}

ASTNode *parse_message_tail(ASTNode *previous_message) {
	switch (tok.type) {
		case TokInt:
		case TokChar:
		case TokString:
		case TokObject:
		case TokValue: {
			previous_message->get_children()[1]->get_children().push_back(new ASTNode(tok));
			next_token();
			return parse_message_tail(previous_message);
		}
//		case TokSqBracketL: {
//			create_basic_obj(TokList, "List");
//			next_token(); // [
//			parse_list(out);
//			return parse_message_tail(out);
//		}
		case TokMessage: {
			vector<ASTNode *> children;
			children.push_back(previous_message);
			children.push_back(new ASTNode(tok));
			next_token();
			auto next_message =  parse_message_tail(new ASTNode(Token { type: TokSend, value: "" }, children));
			// Change order of messages based on precedence
//			if (previous_message->token.type && swap_precedence(previous_message->get_children()[1]->token.value, children[1]->token.value)) {
//				auto prev = previous_message->get_children()[1];
//				auto new_send = new ASTNode(Token { type: TokSend, value: "" }, vector<ASTNode *>{prev->get_children()[0], children[1]});
//				prev->get_children()[0] = new_send;
//				// FIXME: memory leak of the TokSend passed to parse_message_tail_above ?
//				return children[0];
//			}
			return next_message;
		}
		case TokOpenParend: {
			next_token(); // (
			vector<ASTNode *> children;
			children.push_back(parse_parameters(previous_message));
			children.push_back(new ASTNode(Token { type: TokMessage, value: "call" }));
			next_token(); // )
			return parse_message_tail(new ASTNode(Token { type: TokSend, value: "" }, children));
		}
		case TokEOF: {
			if (request_line())
				return parse_message_tail(previous_message);
			return previous_message;
		}
		case TokStatementEnd:
		case TokComa:
		case TokSListEnd:
		case TokCloseParend:
		case TokSqBracketR:
			return previous_message;
		case TokSListBegin:
		//	previous_message->get_children()[1]->get_children().push_back(parse_program());
			return previous_message;
		default:
			throw error_msg("Expected Object, value, message, (, ), ;, }, EOF or ,. Found: " + token_readable(&tok) + ".");
	};
}

ASTNode *parse(string &fname, vector<string> &f) {
	file = f;
	filename = fname;
	line_number = 0;

	if (f.size() != 0)
		raw_str = f[0];
	position = 0;

	return parse_program();
}