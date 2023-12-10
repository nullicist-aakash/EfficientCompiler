import helpers.reflection;
import helpers.checks;
#include <iostream>
using namespace std;

enum X
{
	A,
	B,
	C
};

int main()
{
	ct_assert([]() {return ""; });
	cout << X::C << endl;
}