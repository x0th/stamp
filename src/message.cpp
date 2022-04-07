/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>

#include "ast.h"
#include "message.h"

using namespace std;

Message::Message(string name, ASTNode *sender, Object *requester) : name(name), sender(sender), requester(requester) {}

ASTNode *Message::get_sender() { return sender; }

Object *Message::get_requester() { return requester; }

void Message::set_requester(Object *req) { requester = req; }