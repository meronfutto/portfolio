#include "MyString.h"
#include <stdexcept>
#include <limits>

MyString::MyString()
	: str_(nullptr), length_(0), capacity_(0) {
}

MyString::MyString(const char* source)
	: str_(nullptr), length_(0), capacity_(0) {

	if (source == nullptr) {
		throw std::invalid_argument("Source must be not null!");
	}

	// Determine the source length without counting the null terminator.
	while (source[length_] != '\0') {
		++length_;
	}

	// Allocate space for the characters and the null terminator.
	capacity_ = length_ + 1;
	str_ = new char[capacity_];

	// Copy the entire source, including its null terminator.
	for (int i = 0; i <= length_; i++) {
		str_[i] = source[i];
	}
}

MyString::MyString(const MyString& source)
	: MyString(source.c_str()) {
}

MyString::MyString(const std::string& source)
	: MyString(source.c_str()) {
}

MyString::MyString(const char* source, int count)
	: str_(nullptr), length_(0), capacity_(0) {
	if (source == nullptr) {
		throw std::invalid_argument("Source must be not null!");
	}

	if (count < 0) {
		throw std::invalid_argument("Count must be not negative!");
	}

	int source_length = 0;

	while (source[source_length] != '\0') {
		++source_length;
	}

	// Prevent reading beyond the source string.
	if (count > source_length) {
		throw std::out_of_range("Count exceeds source length!");
	}

	length_ = count;
	capacity_ = length_ + 1;
	str_ = new char[capacity_];

	// Copy only the requested characters.
	for (int i = 0;i < length_;i++) {
		str_[i] = source[i];
	}

	// The selected portion must have its own null terminator.
	str_[length_] = '\0';
}

MyString::MyString(const std::string& source, int count)
	: MyString(source.c_str(), count) {
}

MyString::MyString(const MyString& source, int count)
	: MyString(source.c_str(), count) {
}

MyString::MyString(int count, char ch)
	: str_(nullptr), length_(0), capacity_(0) {
	
	if (count < 0) {
		throw std::invalid_argument("Count must not be negative!");
	}

	length_ = count;
	capacity_ = length_ + 1;
	str_ = new char[capacity_];

	// Fill the string with the requested character.
	for (int i = 0;i < length_;i++) {
		str_[i] = ch;
	}

	str_[length_] = '\0';
}

MyString::~MyString() {
	// Free the character buffer.
	delete[] str_;
}

const char* MyString::c_str() const {
	// Provide a valid empty C string when no buffer is allocated.
	if (str_ == nullptr) {
		return "";
	}

	return str_;
}

int MyString::size() const {
	return length_;
}

int MyString::capacity() const {
	return capacity_;
}

void MyString::clear(){
	if (str_ != nullptr) {
		str_[0] = '\0';
	}

	length_ = 0;
}

void MyString::shrink_to_fit() {
	int new_capacity = length_ + 1;

	if (capacity_ <= new_capacity) {
		return;
	}

	// Allocate a smaller buffer and copy the string, including its terminator.
	char* new_str = new char[new_capacity];

	for (int i = 0; i <= length_;i++) {
		new_str[i] = str_[i];
	}

	// Replace the old buffer with the smaller one.
	delete[] str_;
	str_ = new_str;
	capacity_ = new_capacity;
}

bool MyString::empty() const{
	return length_ == 0;
}

void MyString::insert(int index, int count, char ch) {
	if (index<0 || index > length_) {
		throw std::out_of_range("Index is out of range!");
	}

	if (count < 0) {
		throw std::invalid_argument("Count must not be negative!");
	}

	if (count == 0) {
		return;
	}

	if (count > std::numeric_limits<int>::max() - length_ - 1) {
		throw std::length_error("Resulting string is too large!");
	}

	int new_length = length_ + count;
	if (new_length + 1 > capacity_) {
		// Allocate a larger buffer and preserve the current text.
		char* new_str = new char[new_length + 1];

		for (int i = 0; i < length_;++i) {
			new_str[i] = str_[i];
		}
		new_str[length_] = '\0';

		delete[] str_;
		str_ = new_str;
		capacity_ = new_length + 1;
	}

	// Shift characters to the right, including the null terminator.
	for (int i = length_; i >= index;--i) {
		str_[i + count] = str_[i];
	}

	for (int i = 0;i < count;++i) {
		str_[index + i] = ch;
	}

	length_ = new_length;
}

void MyString::insert(int index, const char* source) {
	// Keep a separate copy of the source text.
	MyString source_copy(source);
	// Insert spaces before copying the text.
	insert(index, source_copy.size(), ' ');

	// Replace the spaces with the source characters.
	for (int i = 0; i < source_copy.size();++i) {
		str_[index + i] = source_copy.c_str()[i];
	}
}

void MyString::insert(int index, const std::string& source) {
	insert(index, source.c_str());
}

void MyString::insert(int index, const MyString& source) {
	insert(index, source.c_str());
}

void MyString::insert(int index, const char* source, int count) {
	MyString source_part(source, count);
	insert(index, source_part);
}

void MyString::insert(int index, const std::string& source, int count) {
	insert(index, source.c_str(), count);
}

void MyString::insert(int index, const MyString& source, int count) {
	insert(index, source.c_str(), count);
}


void MyString::insert(int index, const char* source, int s_index, int count) {
	if (source == nullptr) {
		throw std::invalid_argument("Source must not be null!");
	}

	int source_length = 0;
	while (source[source_length] != '\0') {
		++source_length;
	}

	if (s_index < 0 || s_index > source_length) {
		throw std::out_of_range("Source index is out of range!");
	}

	// Pass the source starting position and count to the existing insert.
	insert(index, source + s_index, count);
}

void MyString::insert(int index, const std::string& source, int s_index, int count) {
	insert(index, source.c_str(), s_index, count);
}

void MyString::insert(int index, const MyString& source, int s_index, int count) {
	insert(index, source.c_str(), s_index, count);
}



MyString& MyString::operator=(const char* source) {
	if (source == nullptr) {
		throw std::invalid_argument("Source must not be null!");
	}

	int source_length = 0;

	while (source[source_length] != '\0') {
		++source_length;
	}

	if (source_length + 1 > capacity_) {
		// Allocate the larger buffer and copy the text
		char* new_str = new char[source_length + 1];

		for (int i = 0;i <= source_length;++i) {
			new_str[i] = source[i];
		}

		delete[] str_;
		str_ = new_str;
		capacity_ = source_length + 1;
	}
	else {
		// Copy the text into the existing buffer.
		for (int i = 0; i <= source_length; ++i) {
			str_[i] = source[i];
		}
	}
	length_ = source_length;

	return *this;
}

MyString& MyString::operator=(const MyString& source) {
	return operator=(source.c_str());
}

MyString& MyString::operator=(const std::string& source) {
	return operator=(source.c_str());
}

MyString& MyString::operator= (char ch) {
	char source[2] = { ch, '\0' };
	return operator=(source);
}