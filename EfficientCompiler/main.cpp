import RegexParser;
import compiler;
import helpers;

import std;

using namespace std;
using namespace RegexParser;


int main()
{
	auto ast = get_ast(R"(abc|[d-f]*)");
	cout << ast.errors << endl;
	cout << ast.logs << endl;
	if (ast.root)
	{
		cout << *ast.root << endl;
		auto nfa = get_nfa(ast.root.get());
		cout << nfa << endl;
	}

	return 0;
}