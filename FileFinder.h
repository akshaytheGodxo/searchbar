#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "IndexFiles.h"
class FileFinder {
public:
	void findFilesForMe(const std::wstring& startDir, const std::wstring& target);

	void BuildIndexAtStartup(const std::wstring& rootDir,
		IndexFiles& indexer);
};