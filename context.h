#pragma once

#include <map>
#include <string>

using namespace std;

class Object;

void initialize_global_context();

class Context {
public:
	Context() {}
	void add(Object *obj, string name);
	Object *get(string obj);
	void dump();
private:
	map<string, Object *> context;
};

extern Context *global_context;