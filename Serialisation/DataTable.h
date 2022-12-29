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

	const std::string*const GetCell(size_t Row, size_t Column) const;
	size_t GetNumColumns() const { return GetNumRows() > 0 ? mTable[0].size() : 0; }
	size_t GetNumRows() const { return mTable.size(); }

	void SetSize(size_t NumRows, size_t NumColumns);
	void SetRow(size_t Row, const std::vector<std::string>& Values);
	void SetCell(size_t Row, size_t Column, std::string& Value);

	size_t GetSizeInBytesAsCSV() const;

	static DataTablePtr CreateFromCSV(BinaryReader& Reader);

private:

	std::vector<std::vector<std::string>> mTable;
		
};

using ProgressReporter = std::function<void(float)>;

enum class ColumnDataType
{
	Unknown = 0,
	Integer = 1,
	Real = 2,
	Text = 4,
};

class TypedDataTable
{
public:

	static const char* GetColumnDataTypeName(ColumnDataType Type)
	{
		switch (Type)
		{
		case ColumnDataType::Unknown:
			return "NULL";
			break;
		case ColumnDataType::Integer:
			return "INTEGER";
			break;
		case ColumnDataType::Real:
			return "REAL";
			break;
		case ColumnDataType::Text:
			return "TEXT";
			break;
		default:
			return "Mixed";
		}
	}

	explicit TypedDataTable();

	size_t GetNumColumns() const { return mColumnHeaders.size(); }
	size_t GetNumRows() const { return mNumRows; }

	const std::vector<std::string>& GetColumnHeaders() { return mColumnHeaders; }

	const std::vector<ColumnDataType>& GetColumnDataTypes() { return mColumnDataTypes; }
	const std::string* GetColumnHeader(size_t ColumnIndex);
	ColumnDataType GetColumnDataType(size_t ColumnIndex);
	
	const std::vector<int32_t>* GetColumnInteger(size_t ColumnIndex) const;
	const std::vector<float>* GetColumnFloat(size_t ColumnIndex) const;
	const std::vector<std::string>* GetColumnString(size_t ColumnIndex) const;

	const std::string*const GetCellAsString(size_t Row, size_t Column) const;


	static TypedDataTablePtr CreateFromCSV(BinaryReader& Reader, uint32_t RowDataStarts, uint32_t RowForColumns, ProgressReporter ReportProgress);

private:

	void SerialiseCell(const std::string& Data, size_t Column, size_t Row);

	std::vector<std::string> mColumnHeaders;
	std::vector<ColumnDataType> mColumnDataTypes;
	std::vector<std::unique_ptr<std::vector<int32_t>>> mIntegerValues;
	std::vector<std::unique_ptr<std::vector<float>>> mFloatValues;
	std::vector<std::unique_ptr<std::vector<std::string>>> mStringValues;
	size_t mNumRows;
};
