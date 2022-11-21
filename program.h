
#include "sqlite/sqlite3.h"
#include "ImGuiColorTextEdit/TextEditor.h"

struct sqlite3;

class Program
{
public: 

	Program();

	void Init(const char* db_path);
	void MainLoopUpdate();
	void Shutdown();

private:

	sqlite3* db;
	TextEditor editor;
	const char* query = "select * from sqlite_master";
	char** result = NULL;
	int result_rows = 0;
	int result_cols = 0;
};