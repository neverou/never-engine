#pragma once

// sys.h
// this file is for platform specific functions
// like filesystem and querying system time, etc


// TODO(...); Replace all of C runtime library with our own

#include "core.h"
#include "str.h"
#include "array.h"

// ~Todo reconsider file modes and the matrix of possibilities on different platforms
enum FileMode {
	FILE_READ     = 0x1,
	FILE_WRITE    = 0x2,
	FILE_CREATE	  = 0x4,
	FILE_TRUNCATE = 0x8,
	FILE_APPEND   = 0x10,
};

// ~Todo figure out if we need this to be the same size across platforms?
// i mean we shouldn't be serializing files regardless but still
struct File {
	u32 mode;
	bool error;
	
	#if defined(BUILD_DEBUG)
		char _filename[64];
	#endif

	#if defined(PLATFORM_LINUX)
		void* fileHandle;
		// s32 fileDescriptor;
    #elif defined(PLATFORM_MACOS)
        void* fileHandle;
    #elif defined(PLATFORM_WINDOWS)
		void* fileHandle;
	#endif
};

bool Open(File* file, StringView str, u32 mode);
void Close(File* file);
Size Read(File* file, void* buffer, Size count);
Size Write(File* file, void* buffer, Size count);
Size FileLength(File* file);

// ~Refactor make a better way to see if the file exists or not, should be used by @FileExists
bool FileExists(StringView path);

Array<String> ListAllDirs(StringView path);
Array<String> ListAllFiles(StringView path);

//

// because windows is weird with its whole funcname_s thing, this is helpful for portability
void Memset(void* data, Size size, u8 value);
void Memcpy(void* dst, void* src, Size size);


//

u64 GetTicks();
