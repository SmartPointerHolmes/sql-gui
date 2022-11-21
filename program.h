
#include "sqlite/sqlite3.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include <functional>
#include <string>

struct sqlite3;

using OpenFileMethod = std::function <std::string()>;

class Program
{
public: 

	Program(OpenFileMethod InOpenFile);

	void Init();
	bool MainLoopUpdate();
	void Shutdown();

private:

	OpenFileMethod OpenFile;
	sqlite3* db;
	TextEditor editor;
	const char* query = "select * from sqlite_master";
	char** result = NULL;
	int result_rows = 0;
	int result_cols = 0;
};