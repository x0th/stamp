/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "parser.h"
#include "lexer.h"

#include <iostream>

using namespace std;

vector<string> file;
string filename;
long unsigned int line_number;
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

string error_msg(string error) {
	string out;
	if (filename != "") {
		out += filename + ":" + std::to_string(line_number+1) + ":"; 
	} else {
		for (int i = 0; i < position+1; i++)
			out += " ";
		out += "^\n";
	}

	out += std::to_string(position+1) + ": " + error + "\n";

	return out;
}

inline void next_token() {
	tok = scan(raw_str, &position);
}

inline void match(TokenType expected) {
	next_token();
	if (tok.type != expected) {
		Token t = Token({ type: expected, value: "" });
		throw error_msg(error_msg("Expected " + token_readable(&t) + ". Found: " + token_readable(&tok)));
	}
}

bool request_line() {
	if (line_number + 1 == file.size())
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
		case TokSListEnd:
			next_token();
			return;
		default:
			return;
	}
}


ASTNode *parse_statement() {
	switch (tok.type) {
		case TokObject:
		{
			auto object = new ASTNode(tok);
			next_token(); // obj
			return parse_statement_tail(object);
		}
		case TokValue:
		{
			auto object = new ASTNode(Token { type: TokObject, value: tok.value });
			next_token(); // obj
			return parse_statement_tail(object);
		}
		case TokFn:
		{
			next_token(); // fn
			auto fn_name = new ASTNode(Token { type: TokValue, value: tok.value });
			vector<ASTNode *> callable_children;
			callable_children.push_back(new ASTNode(Token { type: TokObject, value: "Callable" }));
			callable_children.push_back(new ASTNode(Token { type: TokMessage, value: "clone" }));
			callable_children[1]->get_children().push_back(fn_name);
			ASTNode *clone_callable = new ASTNode(Token { type: TokSend, value: "" }, callable_children);
			next_token(); // fn_name
			return parse_function_tail(clone_callable);
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
		case TokStatementEnd:
		case TokEOF:
		case TokComa:
		case TokCloseParend:
		case TokSListEnd:
			next_token();
			return object;
		default:
			throw error_msg("Expected Object, message, value, =, (, ), ;, } or ,. Found: " + token_readable(&tok) + ".");
	};
}

ASTNode *parse_rhs(ASTNode *object) {
	switch(tok.type) {
		case TokFn: {
			next_token(); // fn
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
	switch(tok.type) {
		case TokComa:
			next_token();
			return parse_function_signature(so_far);
		case TokCloseParend:
			return so_far;
		default:
			throw error_msg("Expected , or ). Found: " + token_readable(&tok) + ".");
	}
}

ASTNode *parse_message_tail(ASTNode *previous_message) {
	switch (tok.type) {
		case TokObject:
		case TokValue: {
			previous_message->get_children()[1]->get_children().push_back(new ASTNode(tok));
			next_token();
			return previous_message;
		}
		case TokMessage: {
			vector<ASTNode *> children;
			children.push_back(previous_message);
			children.push_back(new ASTNode(tok));
			next_token();
			return parse_message_tail(new ASTNode(Token { type: TokSend, value: "" }, children));
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
			return previous_message;
		default:
			throw error_msg("Expected Object, message, value, message, (, ), ;, }, EOF or ,. Found: " + token_readable(&tok) + ".");
	};
}

ASTNode *parse(string &fname, vector<string> &f) {
	file = f;
	filename = fname;
	line_number = 0;

	raw_str = f[0];
	position = 0;

	return parse_program();
}