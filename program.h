
#include "sqlite/sqlite3.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include <functional>
#include <string>
#include <memory>

struct sqlite3;

using OpenFileMethod = std::function <std::string(const char*)>;
class DatabaseHandle;

class TableHandle final
{
public:
	
	static std::shared_ptr<TableHandle> BuildTable(const char* Query, const std::shared_ptr<DatabaseHandle>& Database);

	TableHandle(std::shared_ptr<DatabaseHandle> Database);
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

	std::shared_ptr<DatabaseHandle> mSourceDatabase;
	char** mResult = nullptr;
	char* mErrorMessage = nullptr;
	int mRows= 0;
	int mColumns= 0;
};

class DatabaseHandle final : public std::enable_shared_from_this<DatabaseHandle>
{
public:

	static std::shared_ptr<DatabaseHandle> CreateDatabase(const std::string& FilePath);

	DatabaseHandle(sqlite3& Database);
	~DatabaseHandle();

	DatabaseHandle(const DatabaseHandle& copy) = delete;
	DatabaseHandle(const DatabaseHandle&& Rhs) = delete;
	DatabaseHandle& operator=(const DatabaseHandle& Rhs) = delete;
	DatabaseHandle& operator=(const DatabaseHandle&& Rhs) = delete;

	std::shared_ptr<TableHandle> BuildTable(const char* Query);
	const char* RunQuery(const char* Query);

	sqlite3& GetImpl() const { return mDatabase; }

private:

	sqlite3& mDatabase;
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
	TextEditor editor;

	std::shared_ptr<DatabaseHandle> mActiveDatabase;
	std::shared_ptr<TableHandle> mSQLTableHandle;
	std::shared_ptr<TableHandle> mAllTablesHandle;
	std::shared_ptr<TableHandle> mCurrentTableFullContents;
	int mSelectedTableIndex = 0;
	char mTableName[_MAX_PATH] = { 0 };
};