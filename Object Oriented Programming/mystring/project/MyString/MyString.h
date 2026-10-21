#include <string>
#ifndef _MYSTRING_H_
#define _MYSTRING_H_

// A string class that manages its own dynamically allocated character buffer.
class MyString{

public:
	// Creates an empty string without allocating a buffer.
	MyString();

	// Creates a copy of a null-terminated source string.
	MyString(const char* source);

	// Creates an independent copy of another string.
	MyString(const MyString& source);

	// Creates a copy of an std::string.
	MyString(const std::string& source);

	// Copies the first count characters of the source string.
	MyString(const char* source, int count);

	// Copies the first count characters of an std::string.
	MyString(const std::string& source, int count);

	// Copies the first count characters of another MyString.
	MyString(const MyString& source, int count);

	// Creates a string containing count copies of ch.
	MyString(int count, char ch);

	// Assigns the contents of "source" string.
	MyString& operator= (const char* source);

	// Copies text from another MyString.
	MyString& operator= (const MyString& source);

	// Copies text from an std::string.
	MyString& operator= (const std::string& source);

	// Assigns the single character
	MyString& operator= (char ch);

	// Releases the memory owned by the string.
	~MyString();

	// Returns a read-only pointer to a null-terminated string.
	const char* c_str() const;

	// Returns the number of characters, excluding the null terminator.
	int size() const;

	// Returns the allocated buffer size, including space for the null terminator.
	int capacity() const;
	
	// Clears the string without releasing its buffer.
	void clear();

	// Releases unused buffer space.
	void shrink_to_fit();

	// Returns true if the string is empty.
	bool empty() const;

	// Inserts count copies of ch at index.
	void insert(int index, int count, char ch);

	// Inserts text at index.
	void insert(int index, const char* source);

	// Inserts text from an std::string.
	void insert(int index, const std::string& source);

	// Inserts text from another MyString.
	void insert(int index, const MyString& source);

	// Inserts the first count characters from source.
	void insert(int index, const char* source, int count);
	void insert(int index, const std::string& source, int count);
	void insert(int index, const MyString& source, int count);

	// Inserts count characters from source, starting at s_index.
	void insert(int index, const char* source, int s_index, int count);
	void insert(int index, const std::string& source, int s_index, int count);
	void insert(int index, const MyString& source, int s_index, int count);

private:
	char* str_;      // Pointer to the character buffer
	int length_;     // Number of characters, excluding the null terminator.
	int capacity_;   // Allocated buffer size, including space for the null terminator
};

#endif

