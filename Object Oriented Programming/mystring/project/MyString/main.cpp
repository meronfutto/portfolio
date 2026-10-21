#include <iostream>
#include "MyString.h"

int main() {
	// Create strings using each of the four implemented constructors.
	MyString empty;
	MyString text("Hello world!");
	MyString copy(text);
	std::string source_text = "Hello";
	MyString from_string(source_text);
	MyString part("Hello", 4);
	MyString repeated(5, '!');

	std::string standart_source = "Darenka";
	MyString custom_source("Krutaya");

	MyString standart_part(standart_source, 4);
	MyString custom_part(custom_source, 4);

	MyString clear_example("Hello");
	clear_example.clear();

	MyString assign_source("Kartoshka");
	MyString assign_target("Morkovka");
	assign_source = assign_target;
	std::cout << "\"" << assign_source.c_str() << "\" ("
		<< assign_source.size() << ", "
		<< assign_source.capacity() << ")\n";

	MyString test("bob");
	assign_source.insert(3, test);
	std::cout << "\"" << assign_source.c_str() << "\" ("
		<< assign_source.size() << ", "
		<< assign_source.capacity() << ")\n";


	assign_source.insert(5, " abobka");
	std::cout << "\"" << assign_source.c_str() << "\" ("
		<< assign_source.size() << ", "
		<< assign_source.capacity() << ")\n";

	assign_source.insert(3, 3, 'o');
	std::cout << "\"" << assign_source.c_str() << "\" ("
		<< assign_source.size() << ", "
		<< assign_source.capacity() << ")\n";


	char sym = '!';
	assign_source = sym;
	assign_source.shrink_to_fit();
	std::cout << "\"" << assign_source.c_str() << "\" ("
		<< assign_source.size() << ", "
		<< assign_source.capacity() << ")\n";

	// Display each string in the format: "contents" (size, capacity).

	// Expected: "" (0, 0)
	std::cout << "\"" << empty.c_str() << "\" ("
		<< empty.size() << ", "
		<< empty.capacity() << ")\n";

	// Expected: "Hello world!" (12, 13)
	std::cout << "\"" << text.c_str() << "\" ("
		<< text.size() << ", "
		<< text.capacity() << ")\n";

	// Expected: "Hell" (4, 5)
	std::cout << "\"" << part.c_str() << "\" ("
		<< part.size() << ", "
		<< part.capacity() << ")\n";

	// Expected: "!!!!!" (5, 6)
	std::cout << "\"" << repeated.c_str() << "\" ("
		<< repeated.size() << ", "
		<< repeated.capacity() << ")\n";

	
	std::cout << "\"" << copy.c_str() << "\" ("
		<< copy.size() << ", "
		<< copy.capacity() << ")\n";

	std::cout << "\"" << from_string.c_str() << "\" ("
		<< from_string.size() << ", "
		<< from_string.capacity() << ")\n";

	std::cout << "\"" << standart_part.c_str() << "\" ("
		<< standart_part.size() << ", "
		<< standart_part.capacity() << ")\n";

	std::cout << "\"" << custom_part.c_str() << "\" ("
		<< custom_part.size() << ", "
		<< custom_part.capacity() << ")\n";

	std::cout << "\"" << clear_example.c_str() << "\" ("
		<< clear_example.size() << ", "
		<< clear_example.capacity() << ")\n";

	clear_example.shrink_to_fit();

	std::cout << "\"" << clear_example.c_str() << "\" ("
		<< clear_example.size() << ", "
		<< clear_example.capacity() << ")\n";

	MyString partial_insert("hello");
	MyString kapusta("kapustaitochka");
	partial_insert.insert(1, kapusta, 2);

	std::cout << "\"" << partial_insert.c_str() << "\" ("
		<< partial_insert.size() << ", "
		<< partial_insert.capacity() << ")\n";

	MyString range_insert("hello");
	range_insert.insert(1, "ABCDE", 2, 2);

	std::cout << "\"" << range_insert.c_str() << "\" ("
		<< range_insert.size() << ", "
		<< range_insert.capacity() << ")\n";

	// Local objects are destroyed automatically when main exits.
	return 0;
} // The destructor frees memory automatically.