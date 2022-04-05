/*
 * Copyright (c) 2022, Pavlo Pastaryev <p.pastaryev@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>

#include "ast.h"
#include "message.h"

using namespace std;

Message::Message(string name, ASTNode *sender) : name(name), sender(sender) {}

ASTNode *Message::get_sender() { return sender; }