#if defined(PLATFORM_WINDOWS)

#include <stdio.h> // TODO(voidless): replace CRT
#include <Windows.h>

#include "sys.h"


bool Open(File* file, StringView str, u32 mode) {
	FILE* f;

	const char* openMode = "";
	// ~FixMe
	if (mode & FILE_READ)
	{
		if (mode & FILE_WRITE)
		{
			Assert((mode & FILE_APPEND) != (mode & FILE_TRUNCATE));
			if (mode & FILE_APPEND)
			{
				openMode = "a+b";
			}
			else if (mode & FILE_TRUNCATE)
			{
				openMode = "w+b";
			}
		}
		else
		{
			Assert(!(mode & FILE_APPEND));
			Assert(!(mode & FILE_TRUNCATE));
			openMode = "rb";
		}
	}
	else if (mode & FILE_WRITE)
	{
		Assert((mode & FILE_APPEND) != (mode & FILE_TRUNCATE));
		if (mode & FILE_APPEND)
		{
			openMode = "ab";
		}
		else if (mode & FILE_TRUNCATE)
		{
			openMode = "wb";
		}
	}

	u32 status = fopen_s(&f, str.data, openMode);

	if (status != 0)
		return false;

	file->fileHandle = f;
	file->mode = mode;
	
	return true;
}

void Close(File* file) {
	fclose((FILE*)file->fileHandle);
}

Size Read(File* file, void* buffer, Size count) {
	return fread(buffer, 1, count, (FILE*)file->fileHandle);
}

Size Write(File* file, void* buffer, Size count) {
	return fwrite(buffer, 1, count, (FILE*)file->fileHandle);
}

Size FileLength(File* file) {
	FILE* f = (FILE*)file->fileHandle;
	Size offset = ftell(f);
	fseek(f, 0, SEEK_END);
	Size len = ftell(f);
	fseek(f, offset, SEEK_SET);
	return len;
}


#include "allocators.h"
#include "util.h"

Array<String> ListAllDirs(StringView path)
{
	DynArray<String> dirs = MakeDynArray<String>(0, Frame_Arena);

	String adjustedPath = TCopyString(path);

	if (path.data[path.length - 1] != '/')
		adjustedPath.Concat("/");
	adjustedPath.Concat("*");

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(adjustedPath.data, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		LogWarn("[sys] Failed to list directories in directory (%s)", path.data);
	}

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ArrayAdd(&dirs, TCopyString(ffd.cFileName));
		}
	}
	while (FindNextFileA(hFind, &ffd) != 0);

	return dirs;
}

Array<String> ListAllFiles(StringView path)
{
	DynArray<String> files = MakeDynArray<String>(0, Frame_Arena);

	String adjustedPath = TCopyString(path);

	if (path.data[path.length - 1] != '/')
		adjustedPath.Concat("/");
	adjustedPath.Concat("*");

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(adjustedPath.data, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		LogWarn("[sys] Failed to list files in directory (%s)", path.data);
	}

	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			ArrayAdd(&files, TCopyString(ffd.cFileName));
		}
	}
	while (FindNextFileA(hFind, &ffd) != 0);

	return files;
}






void Memset(void* data, Size size, u8 value) {
	memset(data, value, size);
}

void Memcpy(void* dst, void* src, Size size) {
	memcpy(dst, src, size);
}


//



// ~Todo SDL_GetTicks64 or replace with custom platform function
#include <SDL/SDL.h> 
u64 GetTicks() { return SDL_GetTicks(); }


#endif
