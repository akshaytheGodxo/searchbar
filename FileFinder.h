#pragma once
#include <windows.h>
#include <string>
#include <vector>

class FileFinder
{
public:
	explicit FileFinder(const std::wstring& directoryPath);
	std::vector<std::wstring> findFiles() const;

private:
	std::wstring path;
};

