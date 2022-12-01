#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

class BinaryReader;
class DataTable;
class TypedDataTable;

using DataTablePtr = std::shared_ptr<DataTable>;
using TypedDataTablePtr = std::shared_ptr<TypedDataTable>;

class DataTable 
{
public:

	explicit DataTable();

	const std::wstring*const GetCell(size_t Row, size_t Column) const;
	size_t GetNumColumns() const { return GetNumRows() > 0 ? mTable[0].size() : 0; }
	size_t GetNumRows() const { return mTable.size(); }

	void SetSize(size_t NumRows, size_t NumColumns);
	void SetRow(size_t Row, const std::vector<std::wstring>& Values);
	void SetCell(size_t Row, size_t Column, std::wstring& Value);

	size_t GetSizeInBytesAsCSV() const;

	static DataTablePtr CreateFromCSV(BinaryReader& Reader);

private:

	std::vector<std::vector<std::wstring>> mTable;
		
};

using ProgressReporter = std::function<void(float)>;

class TypedDataTable
{
public:

	explicit TypedDataTable();

	size_t GetNumColumns() const { return mColumnHeaders.size(); }
	size_t GetNumRows() const { return mNumRows; }

	const std::vector<std::wstring>& GetColumnHeaders() { return mColumnHeaders; }

	const std::wstring* GetColumnHeader(size_t ColumnIndex);
	const std::vector<int32_t>* GetColumnInteger(size_t ColumnIndex) const;
	const std::vector<float>* GetColumnFloat(size_t ColumnIndex) const;
	const std::vector<std::wstring>* GetColumnString(size_t ColumnIndex) const;


	static TypedDataTablePtr CreateFromCSV(BinaryReader& Reader, uint32_t RowDataStarts, uint32_t RowForColumns, ProgressReporter ReportProgress);

private:

	void SerialiseCell(const std::wstring& Data, size_t Column, size_t Row);

	std::vector<std::wstring> mColumnHeaders;
	std::vector<std::unique_ptr<std::vector<int32_t>>> mIntegerValues;
	std::vector<std::unique_ptr<std::vector<float>>> mFloatValues;
	std::vector<std::unique_ptr<std::vector<std::wstring>>> mStringValues;
	size_t mNumRows;
};
