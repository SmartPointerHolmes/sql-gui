
#include "sqlite/sqlite3.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include <functional>
#include <string>
#include <memory>

struct sqlite3;

using OpenFileMethod = std::function <std::string()>;

class TableHandle final
{
public:
	
	static std::shared_ptr<TableHandle> BuildTable(const char* Query, sqlite3* Database);

	TableHandle() = default;
	~TableHandle();

	TableHandle(const TableHandle& copy) = delete;
	TableHandle(const TableHandle&& Rhs) = delete;
	TableHandle& operator=(const TableHandle& Rhs) = delete;
	TableHandle& operator=(const TableHandle&& Rhs) = delete;

	bool IsValid() const { return mResult != nullptr; }
	const char* GetErrorMessage() { return mErrorMessage ? mErrorMessage : "Success"; }

	char** GetTable() const { return mResult; }
	int GetRows() const { return mRows; }
	int GetColumns() const { return mColumns; }

private:

	char** mResult = nullptr;
	char* mErrorMessage = nullptr;
	int mRows= 0;
	int mColumns= 0;
};

class Program
{
public: 

	Program(OpenFileMethod InOpenFile, OpenFileMethod InNewFile);

	void Init();
	bool MainLoopUpdate();
	void Shutdown();

private:

	void DrawSQLQueryView();
	void DrawTablesView();
	void DrawRecordsView();

	void DrawAllTablesCombo();

	OpenFileMethod OpenFile;
	OpenFileMethod NewFile;
	sqlite3* ActiveDatabase;
	TextEditor editor;
	std::shared_ptr<TableHandle> mSQLTableHandle;
	std::shared_ptr<TableHandle> mAllTablesHandle;
	std::shared_ptr<TableHandle> mCurrentTableFullContents;
	int mSelectedTableIndex = 0;

};