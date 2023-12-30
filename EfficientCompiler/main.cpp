import JackParser;
import ArithmeticParser;

#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>

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
	constexpr auto aparser = get_arithmetic_parser();
	const auto output2 = aparser("1 + 2 + 4");
	cout << aparser << endl;
	cout << output2.logs << endl;
	cout << output2.errors << endl;
	cout << *output2.root << endl;
}