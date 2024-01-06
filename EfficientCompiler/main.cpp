import RegexParser;

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{
	auto ast = RegexParser::get_ast(R"(abc|[def-z]*)");
	cout << ast.errors << endl;
	cout << ast.logs << endl;
	if (ast.root)
		cout << *ast.root << endl;
}