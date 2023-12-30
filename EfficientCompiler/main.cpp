import JackParser;

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
    constexpr auto parser = get_jack_parser();
    auto contents = read_file("source.jack");

    const auto output = parser(contents);
    cout << parser << endl;
}