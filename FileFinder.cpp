#include "FileFinder.h"
#include <iostream>

std::vector < std::wstring> FileFinder::findFilesForMe(const std::wstring& directory) {
	std::vector<std::wstring> files;

	std::wstring searchPath = L"C:\\" + directory + L".*";

	

	std::wcout << "Searching Path: " << searchPath << std::endl;

	WIN32_FIND_DATAW fileData;

	HANDLE h = FindFirstFileW(searchPath.c_str(), &fileData);
	if (h == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Error: Unable to open directory " << directory << L". Error code: " << GetLastError() << std::endl;
		return files;
	}
	std::wstring newPath = searchPath + L"\\";
	do {
		if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			files.push_back(newPath + fileData.cFileName);
		}
	} while (FindNextFileW(h, &fileData));

	FindClose(h);
	return files;
}