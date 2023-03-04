#pragma once
#include "core.h"
#include "memory.h"

struct String;
struct StringView {
    Size length;
    char* data;
    
    StringView() = default;
    StringView(const char* cstr);
    StringView(String string);

    char& operator[](u64 idx);
    const char& operator[](u64 idx) const;
};

struct String {
    Allocator* allocator;
    Size length;
    char* data;
    
	void Resize(Size newSize);
	void Concat(StringView operand);
	void Update(StringView operand); // we might not need this at all
	void Insert(int at, StringView operand);
	void RemoveAt(int at);
};

String MakeString(Size length = 0, Allocator* allo = NULL); 
void FreeString(String* str);

String CopyString(StringView str, Allocator* allo = NULL);
String TCopyString(StringView str);
String TConcat(StringView a, StringView b);

String TempString(Size length = 0);

StringView GetStringView(String str);

// ~Hack this doesnt have a null terminator
StringView CharStr(char* ch);

// StringView Str(const char* cstr);