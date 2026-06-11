#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include "nlohmann/json.h"
#include "sqllite/sqlite3.h"
#include <iostream>
#include <fstream>
namespace fs = std::filesystem;
struct FileLogs {
	std::string filename;
	std::string fullpath;
	std::string extension;
};

struct FileSaxHandler : public nlohmann::json_sax<nlohmann::json> {
	std::vector<FileLogs> files;

	FileLogs currentFile;
	std::string currentKey;

	bool inObject = false;
	bool null() override {
		return true;
	}

	// called when a boolean is parsed; value is passed
	bool boolean(bool val) override{
		return true;
	}

	// called when a signed or unsigned integer number is parsed; value is passed
	bool number_integer(number_integer_t val) override{
		return true;
	}
	bool number_unsigned(number_unsigned_t val) override {
		return true;
	}

	// called when a floating-point number is parsed; value and original string is passed
	bool number_float(number_float_t val, const string_t& s) override {
		return true;
	}

	// called when a string is parsed; value is passed and can be safely moved away
	bool string(string_t& val) override {
		if (inObject)
		{
			if (currentKey == "filename")
				currentFile.filename = val;
			else if (currentKey == "fullpath")
				currentFile.fullpath = val;
			else if (currentKey == "extension")
				currentFile.extension = val;
		}

		return true;
	}
	// called when a binary value is parsed; value is passed and can be safely moved away
	bool binary(binary_t& val) override {
		return true;
	}

	// called when an object or array begins or ends, resp. The number of elements is passed (or -1 if not known)
	bool start_object(std::size_t elements) override {
		currentFile = {};
		inObject = true;
		return true;
	}
	bool end_object() override{
		files.push_back(currentFile);
		inObject = false;
		return true;
	}
	bool start_array(std::size_t elements) override {
		return true;
	}
	bool end_array() override {
		return true;
	}
	// called when an object key is parsed; value is passed and can be safely moved away
	bool key(string_t& val) {
		currentKey = val;
		return true;
	}

	// called when a parse error occurs; byte position, the last token, and an exception is passed
	bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::json::exception& ex) {
		std::cerr << "Parse error at "
			<< position
			<< " token: "
			<< last_token
			<< "\n"
			<< ex.what()
			<< '\n';

		return false;
	}
};

class FileEntry
{
public:
	

	void IndexDirectory(const fs::path& root);
	void StartIndexing();
	void SaveIndex(const std::string& file);
	void SaveInDB(const std::string& file);
	std::vector<FileLogs> g_FileIndex;
	void LoadIndex(const std::string& filename);
	void MigrateJsonToDb();
	std::vector<FileLogs> SearchFiles(const std::string& prefix);
};

