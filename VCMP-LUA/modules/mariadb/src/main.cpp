#include "main.hpp"

int main() {
	using namespace mariadb;

	account_ref acc = account::create("127.0.0.1", "root", "", "test");
	connection_ref conn = connection::create(acc);
	result_set_ref result = conn->query("SELECT name FROM test_table WHERE id = 1;");
	result->next();
	std::cout << result->get_string("name") << '\n';
}