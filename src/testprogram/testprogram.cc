

#include <udjat/defs.h>
#include <udjat/tools/http.h>
#include <iostream>

using namespace std;
using namespace Udjat;

int main(int argc, const char **argv) {

	HTTP::Client client("http://localhost");


	try {
		cout << "---" << endl;
		cout << client.get() << endl;
		cout << "---" << endl;
	} catch(const std::exception &e) {
		cerr << endl << e.what() << endl;
	}

	return 0;
}
