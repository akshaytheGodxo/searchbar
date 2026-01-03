#pragma once
#include <windows.h>
#include <string>
#include <vector>

class FileFinder {
public:
	static std::vector<std::wstring> findFilesForMe(const std::wstring& directory);
};