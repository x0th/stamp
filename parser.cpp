/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "parser.h"
#include "lexer.h"

#include <iostream>

using namespace std;

string raw_str;
int position;
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
ASTNode *parse_statement_tail_rhs(ASTNode *object);


ASTNode *parse_program() {
	ASTNode *s = new ASTNode(Token { type: TokSList, value: "" });
	tok = scan(raw_str, &position);
	parse_statement_list(s);
	return s;
}

void parse_statement_list(ASTNode *s) {
	ASTNode *st;
	switch (tok.type) {
		case TokStatementEnd: {
			tok = scan(raw_str, &position);
			parse_statement_list(s);
			return;
		}
		case TokObject:
		case TokValue:
		case TokFn:
		{
			st = parse_statement();
			//cout << st->to_string() << "\n";
			if (st) {
				s->get_children().push_back(st);
			}
			parse_statement_list(s);
			return;
		}
		case TokEOF:
		case TokSListEnd:
		default:
			return;
	}
}


ASTNode *parse_statement() {
	switch (tok.type) {
		case TokObject:
		{
			auto object = new ASTNode(tok);
			tok = scan(raw_str, &position); // obj
			return parse_statement_tail(object);
		}
		case TokValue:
		{
			auto object = new ASTNode(Token { type: TokObject, value: tok.value });
			tok = scan(raw_str, &position); // obj
			//cout << object->to_string() << "\n";
			return parse_statement_tail(object);
		}
		case TokFn:
		{
			tok = scan(raw_str, &position); // fn
			auto fn_name = new ASTNode(Token { type: TokValue, value: tok.value });
			vector<ASTNode *> callable_children;
			callable_children.push_back(new ASTNode(Token { type: TokObject, value: "Callable" }));
			callable_children.push_back(new ASTNode(Token { type: TokMessage, value: "clone" }));
			callable_children[1]->get_children().push_back(fn_name);
			ASTNode *clone_callable = new ASTNode(Token { type: TokSend, value: "" }, callable_children);
			tok = scan(raw_str, &position); // fn_name
			return parse_function_tail(clone_callable);
		}
		default:
			return nullptr;
	}
}

ASTNode *parse_function_tail(ASTNode *function) {
	switch(tok.type) {
	case TokOpenParend:
	{
		tok = scan(raw_str, &position); // (
		vector<ASTNode *> children;
		children.push_back(parse_function_signature(function));
		tok = scan(raw_str, &position); // )
		vector<ASTNode *> msg_children;
		msg_children.push_back(parse_program());
		children.push_back(new ASTNode(Token { type: TokMessage, value: "store_body" }, msg_children));
		return new ASTNode(Token { type: TokSend, value: "" }, children);
	}
	default:
		std::cout << "Parse error: parse_function_tail\n";
		exit(1);
	}
}

ASTNode *parse_statement_tail(ASTNode *object) {
	switch (tok.type) {
	case TokMessage:
	case TokObject:
	case TokOpenParend:
	{
		return parse_message_tail(object);
	}
	case TokValue:
	{
		vector<ASTNode *>children;
		children.push_back(object);
		children.push_back(new ASTNode(tok));
		tok = scan(raw_str, &position); // =
		tok = scan(raw_str, &position);
		children.push_back(parse_rhs(children[1]));
		return new ASTNode(Token { type: TokStore, value: "" }, children);
	}
	case TokStore:
	{
		tok = scan(raw_str, &position);
		auto rhs = parse_rhs(object);
		rhs->get_children()[1]->get_children().push_back(object);
		return rhs;
	}
	case TokStatementEnd:
	case TokEOF:
	case TokComa:
	case TokCloseParend:
	case TokSListEnd:
		tok = scan(raw_str, &position);
		return object;
	default: {
		std::cout << "Parse error: parse_statement_tail.\n";
		exit(1);
	}
	};
}

ASTNode *parse_rhs(ASTNode *object) {
	switch(tok.type) {
	case TokFn: {
		tok = scan(raw_str, &position); // fn
		vector<ASTNode *> callable_children;
		callable_children.push_back(new ASTNode(Token { type: TokObject, value: "Callable" }));
		callable_children.push_back(new ASTNode(Token { type: TokMessage, value: "clone" }));
		callable_children[1]->get_children().push_back(object);
		ASTNode *clone_callable = new ASTNode(Token { type: TokSend, value: "" }, callable_children);
		return parse_function_tail(clone_callable);
	}
	case TokObject:
	case TokValue:
		return parse_statement_rhs();
	default: {
		std::cout << "Parse error: parse_rhs.\n";
		exit(1);
	}
	}
}

ASTNode *parse_statement_rhs() {
	switch (tok.type) {
	case TokObject:
	{
		auto object = new ASTNode(tok);
		tok = scan(raw_str, &position); // obj
		return parse_message_tail(object);
	}
	case TokValue:
	{
		auto object = new ASTNode(Token { type: TokObject, value: tok.value });
		tok = scan(raw_str, &position); // obj
		//cout << object->to_string() << "\n";
		return parse_message_tail(object);
	}
	default: {
		std::cout << "Parse error: parse_statement_rhs.\n";
		exit(1);
	}
	}
}

ASTNode *parse_parameters(ASTNode *s) {
	ASTNode *st;
	ASTNode *further = s;
	switch (tok.type) {
	case TokCloseParend:
		return s;
	case TokComa:
		tok = scan(raw_str, &position);
		return parse_parameters(s);
	case TokObject:
	case TokValue:
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
	default: {
		std::cout << "Parse error: parse_parameters.\n";
		exit(1);
	}
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
		tok = scan(raw_str, &position);
		return parse_param_type(new ASTNode(Token { type: TokSend, value: "" }, children));
	}
	default:
		std::cout << "Parse error: parse_function_signature.";
		exit(1);
	}	
}

ASTNode *parse_param_type(ASTNode *param) {
	switch (tok.type) {
	// case TokColon:
		// FIXME: param type
	case TokComa:
	case TokCloseParend:
		return parse_next_param(param);
	default:
		std::cout << "Parse error: parse_param_type.";
		exit(1);
	}
}

ASTNode *parse_next_param(ASTNode *so_far) {
	switch(tok.type) {
	case TokComa:
		tok = scan(raw_str, &position);
		return parse_function_signature(so_far);
	case TokCloseParend:
		return so_far;
	default:
		std::cout << "Parse error: parse_next_param.";
		exit(1);
	}
}

ASTNode *parse_message_tail(ASTNode *previous_message) {
	switch (tok.type) {
	case TokObject:
	case TokValue:
	{
		//tok = scan(raw_str, &position);
		previous_message->get_children()[1]->get_children().push_back(new ASTNode(tok));
		tok = scan(raw_str, &position);
		return previous_message;
		//return parse_statement_tail(previous_message);
	}
	case TokMessage:
	{
		vector<ASTNode *> children;
		children.push_back(previous_message);
		children.push_back(new ASTNode(tok));
		tok = scan(raw_str, &position);
		return parse_message_tail(new ASTNode(Token { type: TokSend, value: "" }, children));
	}
	case TokOpenParend:
	{
		tok = scan(raw_str, &position); // (
		vector<ASTNode *> children;
		children.push_back(parse_parameters(previous_message));
		children.push_back(new ASTNode(Token { type: TokMessage, value: "call" }));
		tok = scan(raw_str, &position); // )
		return parse_message_tail(new ASTNode(Token { type: TokSend, value: "" }, children));
	}
	case TokEOF:
	case TokStatementEnd:
	case TokComa:
	case TokSListEnd:
	case TokCloseParend:
		return previous_message;
	default: {
		std::cout << "Parse error: parse_message_tail.\n";
		exit(1);
	}
	};
}

ASTNode *parse(string rs) {
	raw_str = rs;
	position = 0;
	return parse_program();
}