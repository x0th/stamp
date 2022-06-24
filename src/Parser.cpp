/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Parser.h"
#include "Lexer.h"
#include "Error.h"

std::vector<std::string> file;
std::string filename;
long unsigned int line_number;
std::string raw_str;
long unsigned int position;
Token tok = Token(Token::Program, "", 0, 0);
Generator *generator;

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
ASTNode *parse_vec(ASTNode *list);
ASTNode *parse_use();

// FIXME: change all errors to be hinting errors when we implement error recovery

std::string error_msg(std::string error) {
	std::string out;
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

bool request_line();

inline void next_token() {
	if (raw_str.size() > position) {
		tok = scan(raw_str, &position, filename, line_number);
		if (tok.type == Token::SlashSlash) {
			if (!request_line())
				tok = Token(Token::Eof, filename, line_number, position);
			else
				next_token();
		} if (tok.type == Token::SlashStar) {
			while (tok.type != Token::StarSlash) {
				tok = scan(raw_str, &position, filename, line_number);
				if (tok.type == Token::Eof) {
					if (!request_line())
						throw error_msg("Unmatched multiline comment start.");
				}
			}
			next_token();
		}
	} else
		tok = Token(Token::Eof, filename, line_number, position);
}

inline void match(Token::TokenType expected) {
	next_token();
	if (tok.type != expected) {
		Token t = Token(expected, filename, line_number, position);
		throw error_msg(error_msg("Expected " + t.token_readable() + ". Found: " + tok.token_readable()));
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
	ASTNode *s = new ASTNode(Token(Token::Program, filename, line_number, position));
	try {
		next_token();
		parse_statement_list(s);
	} catch (std::string &msg) {
		terminating_error(StampError::ParsingError, msg);
	}

	return s;
}

void parse_statement_list(ASTNode *s) {
	ASTNode *st;
	switch (tok.type) {
		case Token::StatementEnd: {
			next_token();
			parse_statement_list(s);
			return;
		}
		case Token::Object:
		case Token::Value:
		case Token::Fn:
		case Token::If:
		case Token::While:
		case Token::Int:
		case Token::Char:
		case Token::String:
		case Token::SqBracketL:
		case Token::Message:
		case Token::Break:
		case Token::Continue:
		case Token::Mut:
		{
			st = parse_statement();
			if (st) {
				s->get_children().push_back(st);
			}
			parse_statement_list(s);
			return;
		}
		case Token::Use:
			s->get_children().push_back(parse_use());
			parse_statement_list(s);
			break;
		case Token::Eof:
			if (request_line())
				parse_statement_list(s);
			return;
		case Token::SListBegin: {
			ASTNode *new_slist = new ASTNode(Token(Token::SList, filename, line_number, position));
			next_token();
			parse_statement_list(new_slist);
			if (new_slist)
				s->get_children().push_back(new_slist);
			parse_statement_list(s);
			return;
		}
		case Token::SListEnd:
			next_token();
			return;
		default:
			return;
	}
}

bool is_operator(std::string &s) {
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
		case Token::Int:
		case Token::Char:
		case Token::String:
		case Token::Object: {
			auto object = new ASTNode(tok);
			next_token(); // obj
			return parse_statement_tail(object);
		}
		case Token::Value: {
			if (is_operator(tok.value)) {
				tok.type = Token::Message;
				return parse_statement();
			}

			auto object = new ASTNode(Token(Token::Object, tok.value, filename, line_number, position));
			next_token(); // obj
			return parse_statement_tail(object);
		}
		case Token::SqBracketL: {
			auto vec = new ASTNode(Token(Token::Vec, filename, line_number, position));
			next_token(); // [
			return parse_message_tail(parse_vec(vec));
		}
		case Token::Mut: {
			auto mut_indicator = new ASTNode(tok);
			next_token();
			ASTNode *object;
			if (tok.type == Token::Object || tok.type == Token::Int ||
				tok.type == Token::Char || tok.type == Token::String) {
				object = new ASTNode(tok);
			} else if (tok.type == Token::Value) {
				object = new ASTNode(Token(Token::Object, tok.value, filename, line_number, position));
			} else {
				throw("mut keyword is not applicable to " + tok.token_readable());
			}

			next_token(); // obj

			if (tok.type == Token::Value) {
				auto full_statement = parse_statement_tail(object);
				full_statement->get_children().push_back(mut_indicator);
				return full_statement;
			} else
				throw error_msg("mut keyword can only be used in a store statement.");
		}
		case Token::Fn: {
			auto fn = new ASTNode(tok);
			next_token(); // fn
			fn->get_children().push_back(new ASTNode(tok));
			next_token(); // function name
			return parse_function_tail(fn);
		}
		case Token::If: {
			return parse_if();
		}
		case Token::While: {
			return parse_while();
		}
		case Token::Message: {
			if (!is_operator(tok.value)) {
				throw error_msg("Message at the start of statement was not an operator.");
			}

			std::vector<ASTNode *> children;
			auto msg = tok;
			next_token();
			children.push_back(parse_statement());
			children.push_back(new ASTNode(msg));
			return new ASTNode(Token(Token::Send, filename, line_number, position), children);
		}
		case Token::Break:
		case Token::Continue: {
			auto ast = new ASTNode(tok);
			next_token();
			return ast;
		}
		default:
			return nullptr;
	}
}

ASTNode *parse_function_tail(ASTNode *function) {
	switch(tok.type) {
		case Token::OpenParend: {
			next_token(); // (
			parse_function_signature(function);
			next_token(); // )
			next_token(); // {
			auto body = new ASTNode(Token(Token::SList,filename, line_number, position));
			parse_statement_list(body);
			function->get_children().push_back(body);
			return function;
		}
		case Token::Eof: {
			if (request_line())
				return parse_function_tail(function);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected (. Found " + tok.token_readable() + ".");
	}
}

ASTNode *parse_statement_tail(ASTNode *object) {
	switch (tok.type) {
		case Token::Message:
		case Token::Object:
		case Token::OpenParend: {
			return parse_message_tail(object);
		}
		case Token::Value: {
			std::vector<ASTNode *> children;
			children.push_back(object);
			children.push_back(new ASTNode(tok));
			match(Token::Store); // =
			next_token();
			children.push_back(parse_rhs(children[1]));

			// traverse the TokSend chain to check if the first send was a clone
			// if so, add the name to it
			auto rhs = children[2];
			while (rhs->token.type == Token::Send && rhs->get_children()[0]->token.type == Token::Send) {
				rhs = rhs->get_children()[0];
			}
			if (rhs->token.type == Token::Send && rhs->get_children().size() != 0 && rhs->get_children()[1]->token.value == "clone") {
				rhs->get_children()[1]->get_children().push_back(new ASTNode(Token(Token::Value, children[1]->token.value, filename, line_number, position)));
			}

			return new ASTNode(Token(Token::Store, filename, line_number, position), children);
		}
		case Token::Store: {
			next_token();
			auto rhs = parse_rhs(object);
			rhs->get_children()[1]->get_children().push_back(object);
			return rhs;
		}
		case Token::SListEnd:
			return object;
		case Token::StatementEnd:
		case Token::Eof:
		case Token::Coma:
		case Token::CloseParend:
			next_token();
			return object;
		default:
			throw error_msg("Expected Object, value, message, =, (, ), ;, } or ,. Found: " + tok.token_readable() + ".");
	};
}

ASTNode *parse_vec(ASTNode *vec) {
	switch(tok.type) {
		case Token::SqBracketR: {
			next_token(); return vec;
		}
		case Token::Coma:
			next_token(); return parse_vec(vec);
		case Token::Object:
		case Token::Value:
		case Token::Int:
		case Token::Char:
		case Token::String:
		case Token::SqBracketL: {
			vec->get_children().push_back(parse_statement_rhs());
			return parse_vec(vec);
		}
		case Token::Eof: {
			if (request_line())
				return parse_vec(vec);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Object, value, [ or ]. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_rhs(ASTNode *object) {
	switch(tok.type) {
		case Token::Fn: {
			auto fn = new ASTNode(tok);
			next_token();
			fn->get_children().push_back(object);
			return parse_function_tail(fn);
		}
		case Token::Int:
		case Token::Object:
		case Token::Value:
		case Token::Char:
		case Token::String:
		case Token::SqBracketL:
			return parse_statement_rhs();
		case Token::Eof: {
			if (request_line())
				return parse_rhs(object);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected function declaration, Object or value. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_statement_rhs() {
	switch (tok.type) {
		case Token::Int:
		case Token::Char:
		case Token::String:
		case Token::Object: {
			auto object = new ASTNode(tok);
			next_token(); // obj
			return parse_message_tail(object);
		}
		case Token::Value: {
			auto object = new ASTNode(Token(Token::Object, tok.value, filename, line_number, position));
			next_token(); // obj
			return parse_message_tail(object);
		}
		case Token::SqBracketL: {
			auto vec = new ASTNode(Token(Token::Vec, filename, line_number, position));
			next_token(); // [
			parse_vec(vec);
			return parse_message_tail(vec);
		}
		case Token::CloseParend:
			return nullptr;
		default:
			throw error_msg("Expected Object or value. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_parameters(ASTNode *s) {
	switch (tok.type) {
		case Token::CloseParend:
			return s;
		case Token::Coma:
			next_token();
			return parse_parameters(s);
		case Token::Object:
		case Token::Value:
		case Token::Int:
		case Token::Char:
		case Token::String:
		case Token::SqBracketL:
			s->get_children().push_back(parse_statement_rhs());
			return parse_parameters(s);
//		case TokSListBegin: {
//			st = parse_program();
//			if (st) {
//				vector<ASTNode *> children;
//				children.push_back(s);
//				ASTNode *message = new ASTNode(Token { type: TokMessage, value: "pass_param" });
//				message->get_children().push_back(st);
//				children.push_back(message);
//				further = new ASTNode(Token { type: TokSend, value: "" }, children);
//			}
//			return parse_parameters(further);
//		}
		default:
			throw error_msg("Expected Object, value, ) or ,. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_function_signature(ASTNode *function) {
	switch (tok.type) {
		case Token::CloseParend:
			return function;
		case Token::Value:
		{
			function->get_children().push_back(new ASTNode(tok));
			next_token();
			return parse_param_type(function);
		}
		case Token::Eof: {
			if (request_line())
				return parse_function_signature(function);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected value or ). Found: " + tok.token_readable() + ".");
	}	
}

ASTNode *parse_param_type(ASTNode *param) {
	switch (tok.type) {
		//case TokColon:
			// FIXME: param type
		case Token::Coma:
		case Token::CloseParend:
			return parse_next_param(param);
		case Token::Eof: {
			if (request_line())
				return parse_param_type(param);
			throw error_msg("Unexpected end of file.");
		}
		default:
			throw error_msg("Expected , or ). Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_next_param(ASTNode *so_far) {
	switch (tok.type) {
		case Token::Coma:
			next_token();
			return parse_function_signature(so_far);
		case Token::CloseParend:
			return so_far;
		default:
			throw error_msg("Expected , or ). Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_if() {
	switch (tok.type) {
		case Token::If: {
			auto if_ast = new ASTNode(tok);
			next_token();
			auto full_param = parse_statement_rhs();
			next_token();
			auto true_branch = new ASTNode(Token(Token::SList, filename, line_number, position));
			parse_statement_list(true_branch);
			auto false_branch = parse_if_tail();

			if_ast->get_children().push_back(full_param);
			if_ast->get_children().push_back(true_branch);
			if (false_branch)
				if_ast->get_children().push_back(false_branch);

			return if_ast;
		}
		default:
			throw error_msg("Expected if. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_if_tail() {
	switch (tok.type) {
		case Token::Else: {
			next_token();
			return parse_else_tail();
		}
		case Token::If:
		case Token::Object:
		case Token::Value:
		case Token::Int:
		case Token::String:
		case Token::Char:
		case Token::While:
		case Token::SListEnd:
			return nullptr;
		case Token::Eof:
			if (request_line())
				return parse_if_tail();
			return nullptr;
		default:
			throw error_msg("Expected if, else, Object or value. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_else_tail() {
	switch (tok.type) {
		case Token::If:
			return parse_if();
		case Token::SListBegin: {
			auto body = new ASTNode(Token(Token::SList, filename, line_number, position));
			next_token();
			parse_statement_list(body);
			return body;
		}
		case Token::Eof:
			if (request_line())
				return parse_else_tail();
			throw "Unexpected end of file.";
		default:
			throw error_msg("Expected if or {. Found: " + tok.token_readable() + ".");
	}
}

ASTNode *parse_while() {
	switch (tok.type) {
		case Token::While: {
			auto while_ast = new ASTNode(tok);
			next_token();
			auto full_param = parse_statement_rhs();
			next_token();
			auto body = new ASTNode(Token(Token::SList, filename, line_number, position));
			parse_statement_list(body);

			while_ast->get_children().push_back(full_param);
			while_ast->get_children().push_back(body);

			return while_ast;
		}
		default:
			throw error_msg("Expected while. Found: " + tok.token_readable() + ".");
	}
}

inline bool swap_precedence(std::string &old_operator, std::string &new_operator) {
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
		case Token::Int:
		case Token::Char:
		case Token::String:
		case Token::Object:
		case Token::Value: {
			previous_message->get_children()[1]->get_children().push_back(new ASTNode(tok));
			next_token();
			return parse_message_tail(previous_message);
		}
		case Token::SqBracketL: {
			auto vec = new ASTNode(Token(Token::Vec, filename, line_number, position));
			next_token(); // [
			parse_vec(vec);
			previous_message->get_children()[1]->get_children().push_back(vec);
			return parse_message_tail(previous_message);
		}
		case Token::Message: {
			std::vector<ASTNode *> children;
			children.push_back(previous_message);
			children.push_back(new ASTNode(tok));
			next_token();
			auto next_message =  parse_message_tail(new ASTNode(Token(Token::Send, filename, line_number, position), children));
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
		case Token::OpenParend: {
			next_token(); // (
			auto fn_call = new ASTNode(Token(Token::FnCall, filename, line_number, position));
			fn_call->get_children().push_back(previous_message);
			parse_parameters(fn_call);
			next_token(); // )
			return fn_call;
		}
		case Token::Eof: {
			if (request_line())
				return parse_message_tail(previous_message);
			return previous_message;
		}
		case Token::StatementEnd:
		case Token::Coma:
		case Token::SListEnd:
		case Token::CloseParend:
		case Token::SqBracketR:
			return previous_message;
		case Token::SListBegin:
		//	previous_message->get_children()[1]->get_children().push_back(parse_program());
			return previous_message;
		default:
			throw error_msg("Expected Object, value, message, (, ), ;, }, EOF or ,. Found: " + tok.token_readable() + ".");
	};
}

ASTNode *parse_use() {
	switch (tok.type) {
		case Token::Use: {
			next_token();
			while (tok.type == Token::Eof) {
				if (!request_line())
					throw "Unexpected end of file.";
			}
			if (tok.type != Token::Value)
				throw error_msg("Unexpected token when parsing use statement: " + tok.token_readable() + ".");

			auto saved_file = file;
			auto saved_filename = filename;
			auto saved_line_number = line_number;
			auto saved_raw_str = raw_str;
			auto saved_position = position;

			auto ast = generator->include_from(tok.value);

			file = saved_file;
			filename = saved_filename;
			line_number = saved_line_number;
			raw_str = saved_raw_str;
			position = saved_position;

			match(Token::StatementEnd);
			return ast;
		}
		default:
			throw error_msg("Unexpected token when parsing use statement: " + tok.token_readable() + ".");
	}
}

ASTNode *parse(std::string &fname, std::vector<std::string> &f, Generator *gen) {
	file = f;
	filename = fname;
	line_number = 0;
	generator = gen;

	if (f.size() != 0)
		raw_str = f[0];
	position = 0;

	return parse_program();
}