#pragma once
#include <windows.h>
#include <string>
#include <vector>

class FileFinder {
public:
	void findFilesForMe(const std::wstring& startDir, const std::wstring& target);
};