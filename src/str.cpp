#include "str.h"

#include "allocators.h"
#include "util.h"

#include <string.h>
#include <stdlib.h>

// ~Todo: 
// if we need custom allocators on the string we'll do that, 
// currently dont see a need yet.

String MakeString(size_t length, Allocator* allo) {
	String str;
    str.allocator = allo;
    str.data = cast(char*, AceAlloc(length + 1, allo));
	str.length = length;
	str.data[str.length] = 0;
	return str;
}

void FreeString(String* str) {
	AceFree(str->data, str->allocator);
	str->data = NULL;
	str->length = 0;
}


String TempString(Size length) {
    // ~Hack: multithreading doesnt work with this
    // ~Todo: fix with maybe some Jai-like context struct we pass around
    return MakeString(length, Frame_Arena);
}


// ~Test
String TConcat(StringView a, StringView b)
{
	String str = TempString(a.length + b.length);
	Memcpy(str.data, a.data, a.length);
	Memcpy(str.data + a.length, b.data, b.length);
	return str;
}



void String::Resize(Size newSize) {
	this->data = cast(char*, AceRealloc(data, this->length + 1, newSize + 1, this->allocator)); // Deal with allocation failures
	this->length = newSize;
	this->data[length] = 0;
}

void String::Concat(StringView operand) {
    Size oldLength = this->length;
    this->Resize(this->length + operand.length);
    Memcpy(this->data + oldLength, operand.data, operand.length);
}

void String::Update(StringView operand)
{
    this->Resize(operand.length);
    Memcpy(this->data, operand.data, operand.length);
}

void String::Insert(int at, StringView operand)
{
	Assert(at >= 0 && at <= this->length);

	Size oldLength = this->length;
    this->Resize(this->length + operand.length);
	for (int i = oldLength - 1; i >= at; i--)
	 	this->data[i + operand.length] = this->data[i];
    Memcpy(this->data + at, operand.data, operand.length);
}

void String::RemoveAt(int at)
{
	Assert(at >= 0 && at < this->length);

	Memcpy(this->data + at, this->data + at + 1, this->length - at - 1);
	this->Resize(this->length - 1);
}

char& StringView::operator[](u64 idx)
{
	return this->data[idx];
}

const char& StringView::operator[](u64 idx) const
{
	return this->data[idx];
}



StringView GetStringView(String str) {
    // ~Note: C++ made me turn this from 
    // StringView { str.length, str.data } 
    // into this because i added a conversion constructor,
	StringView view;
    view.length = str.length;
    view.data = str.data;
    return view;
}

StringView::StringView(const char* cstr) {
    length = strlen(cstr);
    data = (char*)cstr;
}

StringView::StringView(String str) {
    *this = GetStringView(str);
}

StringView CharStr(char* ch) {
	StringView sv;
	sv.length = 1;
	sv.data = ch;
	return sv;
}

String CopyString(StringView str, Allocator* allo) {
	String nstr = MakeString(str.length, allo);
	Memcpy(nstr.data, str.data, str.length + 1);
	return nstr;
}

String TCopyString(StringView str) {
	return CopyString(str, Frame_Arena);
}




/*
StringView Str(const char* cstr) {
        return StringView { strlen(cstr), cast(char*, cstr) };
}

*/