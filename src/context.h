#pragma once

#include <map>
#include <string>

using namespace std;

class Store;

void initialize_global_context();

class Context {
public:
	Context() {}
	void add(Store *store, string name);
	Store *get(string name);
	void dump();
private:
	map<string, Store*> context;
};

extern Context *global_context;