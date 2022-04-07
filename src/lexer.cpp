/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <iostream>
#include <cstring>
#include <cctype>

#include "lexer.h"
#include "token.h"
#include "parser.h"

using namespace std;

char token_image[MAX_TOKEN_LEN];

char scan_char(string &raw_string, long unsigned int *position) {
	return raw_string[++(*position)];
}

Token scan(string &raw_string, long unsigned int *position) {
	int c = raw_string[*position];
	int i = 0;

	while (isspace(c)) c = scan_char(raw_string, position);

	if (c == '\0') return Token({ type: TokEOF, value: "" });

	bool is_message = false;
	if (c == '.') {
		c = scan_char(raw_string, position);
		is_message = true;
	}

	switch (c) {
		case ';': scan_char(raw_string, position); return Token({ type: TokStatementEnd, value: ""});
		case '{': scan_char(raw_string, position); return Token({ type: TokSListBegin, value: ""});
		case '}': scan_char(raw_string, position); return Token({ type: TokSListEnd, value: ""});
		case '(': scan_char(raw_string, position); return Token({ type: TokOpenParend, value: ""});
		case ')': scan_char(raw_string, position); return Token({ type: TokCloseParend, value: ""});
		case ',': scan_char(raw_string, position); return Token({ type: TokComa, value: ""});
		case '^': scan_char(raw_string, position); return Token({ type: TokMessage, value: "clone"});
		case '!': scan_char(raw_string, position); scan_char(raw_string, position); return Token({ type: TokMessage, value: "!="});
		case '=': {
			c = scan_char(raw_string, position);

			if (c == '=') {
				scan_char(raw_string, position);
				return Token({ type: TokMessage, value: "==" });
			}

			return Token({ type: TokStore, value: "" });
		}
		default: {
			do {
				token_image[i++] = c;
				if (i >= MAX_TOKEN_LEN)
					throw error_msg("Maximum token length exceeded.");
				c = scan_char(raw_string, position);
			} while (isalpha(c) || isdigit(c) || c == '_');
			token_image[i] = '\0';

			if (token_image == string("fn")) return Token({ type: TokFn, value: "" });
			if (token_image[0] >= 65 && token_image[0] <= 90) return Token({ type: TokObject, value: token_image });
			return is_message ? Token({ type: TokMessage, value: token_image }) : Token({ type: TokValue, value: token_image });
		}
	}
}