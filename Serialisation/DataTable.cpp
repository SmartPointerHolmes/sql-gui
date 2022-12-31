
#include "DataTable.h"
#include "BinaryReader.h"
#include <stdexcept>
#include <sstream>

DataTable::DataTable()
{
}

const std::string *const  DataTable::GetCell(size_t Row, size_t Column)const
{
	if (GetNumRows() > Row && GetNumColumns() > Column)
	{
		return &mTable[Row][Column];
	}
	return nullptr;
}

void DataTable::SetSize(size_t NumRows, size_t NumColumns)
{
	mTable.resize(NumRows);
	for (auto&& Row : mTable)
	{
		Row.resize(NumColumns);
	}
}

void DataTable::SetCell(size_t RowIndex, size_t ColumnIndex, std::string& Value)
{
	if (RowIndex < mTable.size())
	{
		auto& Column = mTable[RowIndex];
		if (ColumnIndex < Column.size())
		{
			Column[ColumnIndex] = Value;
		}
	}
}
void DataTable::SetRow(size_t RowIndex, const std::vector<std::string>& Values)
{
	if (RowIndex < mTable.size())
	{
		auto& Row = mTable[RowIndex];
		if (Values.size() == Row.size())
		{
			Row = Values;
		}
		else if (Values.size() < Row.size())
		{
			for (size_t ValueIndex = 0; ValueIndex < Values.size(); ++ValueIndex)
			{
				Row[ValueIndex] = Values[ValueIndex];
			}
		}
	}
}
size_t DataTable::GetSizeInBytesAsCSV() const
{
	size_t DataSize = 0;
	for (auto& Row : mTable)
	{
		for (auto& Column : Row)
		{
			DataSize += Column.size() * sizeof(wchar_t);
		}
	}

	const auto NumRows = GetNumRows();
	const auto NumColumns = GetNumColumns();
	DataSize += sizeof(wchar_t) * NumRows * NumColumns; // commas
	DataSize += sizeof(wchar_t) * NumRows; // carriage returns

	return DataSize;
}
	
DataTablePtr DataTable::CreateFromCSV(BinaryReader & Reader)
{
	constexpr char CarriageReturn = '\n';
	constexpr char Comma = ',';
	constexpr char DoubleQuote = '\"';
	size_t StartOfBuffer = Reader.GetReadPosition();
	size_t NumColumns = 0;
	size_t NumRows = 0;
	size_t MaxLengthRow = 0;
	size_t CurrentLengthRow = 0;
	char Character = (char)Reader.ReadUInt8();

	NumRows++;
	NumColumns++;
	while (Character != CarriageReturn && Character != DoubleQuote && Reader.GetReadPosition() < Reader.GetDataLength())
	{
		CurrentLengthRow++;
		if (Character == Comma)
		{
			MaxLengthRow = std::max(CurrentLengthRow, MaxLengthRow);
			CurrentLengthRow = 0;
			NumColumns++;
		}
		Character = (char)Reader.ReadUInt8();
	}
	NumRows++;
	while (Reader.GetReadPosition() < Reader.GetDataLength())
	{
		Character = (char)Reader.ReadUInt8();
		CurrentLengthRow++;

		if (Character == CarriageReturn)
		{
			NumRows++;
			MaxLengthRow = std::max(CurrentLengthRow, MaxLengthRow);
			CurrentLengthRow = 0;
		}
	}

	DataTablePtr NewTable = std::make_shared<DataTable>();
	NewTable->mTable.resize(NumRows);
	for (auto& Column : NewTable->mTable)
	{
		Column.resize(NumColumns);
	}

	Reader.Seek(StartOfBuffer);

	bool CaptureWholeStream = false;
	size_t RowIndex = 0;
	size_t ColumnIndex = 0;
	std::string TempData;
	TempData.reserve(MaxLengthRow);
	while (Reader.GetReadPosition() < Reader.GetDataLength())
	{
		Character = (char)Reader.ReadUInt8();
		if (Character == DoubleQuote)
		{
			CaptureWholeStream = !CaptureWholeStream;
		}
		if (!CaptureWholeStream)
		{
			if (Character == Comma)
			{
				NewTable->mTable[RowIndex][ColumnIndex] = TempData.data();
				ColumnIndex++;
				TempData.clear();
				continue;
			}
			else if (Character == CarriageReturn)
			{
				NewTable->mTable[RowIndex][ColumnIndex] = TempData.data();
				TempData.clear();
				ColumnIndex = 0;
				RowIndex++;
				continue;
			}
		}
		
		TempData.push_back(Character);
	}
	NewTable->mTable[RowIndex][ColumnIndex] = TempData.data();
	return NewTable;
}

TypedDataTable::TypedDataTable()
{
}


const std::string* TypedDataTable::GetColumnHeader(size_t ColumnIndex)
{
	if (ColumnIndex < mColumnHeaders.size())
	{
		return &mColumnHeaders[ColumnIndex];
	}

	return nullptr;
}
ColumnDataType TypedDataTable::GetColumnDataType(size_t ColumnIndex)
{
	if (ColumnIndex < mColumnDataTypes.size())
	{
		return mColumnDataTypes[ColumnIndex];
	}

	return ColumnDataType::Unknown;
}
const std::vector<int32_t>* TypedDataTable::GetColumnInteger(size_t ColumnIndex) const
{
	if (ColumnIndex < mColumnHeaders.size())
	{
		return mIntegerValues[ColumnIndex] != nullptr ? mIntegerValues[ColumnIndex].get() : nullptr;
	}

	return nullptr;
}
const std::vector<float>* TypedDataTable::GetColumnFloat(size_t ColumnIndex) const
{
	if (ColumnIndex < mColumnHeaders.size())
	{
		return mFloatValues[ColumnIndex] != nullptr ? mFloatValues[ColumnIndex].get() : nullptr;
	}

	return nullptr;
}
const std::vector<std::string>* TypedDataTable::GetColumnString(size_t ColumnIndex) const
{
	if (ColumnIndex < mColumnHeaders.size())
	{
		return mStringValues[ColumnIndex] != nullptr ? mStringValues[ColumnIndex].get() : nullptr;
	}

	return nullptr;
}

const std::string*const TypedDataTable::GetCellAsString(size_t Row, size_t Column) const
{
	if (Column < mStringValues.size())
	{
		if (mStringValues[Column])
		{
			if (Row < mStringValues[Column]->size())
			{
				return &((*mStringValues[Column])[Row]);
			}
		}
	}
	return nullptr;
}

namespace {
	template<typename T>
	void AddToColumn(std::unique_ptr<std::vector<T>>& ColumnValues, const T& Value, size_t TotalRows, size_t RowIndex)
	{
		if (!ColumnValues)
		{
			ColumnValues.reset(new std::vector<T>());
			ColumnValues->resize(TotalRows);
			memset(ColumnValues->data(), 0, sizeof(T)* TotalRows);
		}

		(*ColumnValues)[RowIndex] = Value;
	}

	void AddToColumn(std::unique_ptr<std::vector<std::string>>& ColumnValues, const std::string& Value, size_t TotalRows, size_t RowIndex)
	{
		if (!ColumnValues)
		{
			ColumnValues.reset(new std::vector<std::string>());
			ColumnValues->resize(TotalRows);
		}

		(*ColumnValues)[RowIndex] = Value;
	}
}

TypedDataTablePtr TypedDataTable::CreateFromCSV(BinaryReader& Reader, uint32_t RowDataStarts, uint32_t RowForColumns, ProgressReporter ReportProgress)
{
	constexpr char CarriageReturn = '\n';
	constexpr char Comma = ',';
	constexpr char DoubleQuote = '\"';

	size_t StartOfBuffer = Reader.GetReadPosition();
	size_t NumColumns = 0;
	size_t NumRows = 0;
	size_t MaxLengthRow = 0;
	size_t CurrentLengthRow = 0;
	char Character = (char)Reader.ReadUInt8();

	NumRows++;
	NumColumns++;
	while (Character != CarriageReturn && Character != DoubleQuote && Reader.GetReadPosition() < Reader.GetDataLength())
	{
		CurrentLengthRow++;
		if (Character == Comma)
		{
			MaxLengthRow = std::max(CurrentLengthRow, MaxLengthRow);
			CurrentLengthRow = 0;
			NumColumns++;
		}
		Character = (char)Reader.ReadUInt8();
	}
	NumRows++;
	while (Reader.GetReadPosition() < Reader.GetDataLength())
	{
		Character = (char)Reader.ReadUInt8();
		CurrentLengthRow++;

		if (Character == CarriageReturn)
		{
			NumRows++;
			MaxLengthRow = std::max(CurrentLengthRow, MaxLengthRow);
			CurrentLengthRow = 0;
		}
	}

	TypedDataTablePtr NewTable = std::make_shared<TypedDataTable>();
	NewTable->mNumRows = NumRows;
	NewTable->mColumnHeaders.resize(NumColumns);
	NewTable->mColumnDataTypes.resize(NumColumns);
	NewTable->mIntegerValues.resize(NumColumns);
	NewTable->mFloatValues.resize(NumColumns);
	NewTable->mStringValues.resize(NumColumns);

	Reader.Seek(StartOfBuffer);

	bool CaptureWholeStream = false;
	size_t RowIndex = 0;
	size_t ColumnIndex = 0;
	std::string TempData;
	TempData.reserve(MaxLengthRow);
	while (Reader.GetReadPosition() < Reader.GetDataLength())
	{
		Character = (char)Reader.ReadUInt8();
		if (Character == DoubleQuote)
		{
			CaptureWholeStream = !CaptureWholeStream;
		}
		if (!CaptureWholeStream && (Character == Comma || Character == CarriageReturn))
		{
			if (RowIndex >= RowDataStarts)
			{
				NewTable->SerialiseCell(TempData, ColumnIndex, RowIndex - RowDataStarts);
			}
			else if (RowIndex == RowForColumns)
			{
				NewTable->mColumnHeaders[ColumnIndex] = TempData;
			}
			TempData.clear();

			if (Character == Comma)
			{
				ColumnIndex++;
			}
			else if (Character == CarriageReturn)
			{
				ColumnIndex = 0;
				RowIndex++;
				if (ReportProgress)
				{
					ReportProgress(static_cast<float>(static_cast<double>(RowIndex) / static_cast<double>(NumRows)));
				}
			}
		}
		else
		{
			TempData.push_back(Character);
		}
	}

	NewTable->SerialiseCell(TempData, ColumnIndex, RowIndex);

		
	for (auto Column = 0; Column < NumColumns; ++Column)
	{
		auto& FloatValues = NewTable->mFloatValues[Column];
		auto& IntegerValues = NewTable->mIntegerValues[Column];
		if (IntegerValues && FloatValues)
		{
			for (auto ValueIndex = 0; ValueIndex < IntegerValues->size(); ++ValueIndex)
			{
				(*FloatValues)[ValueIndex] = (*FloatValues)[ValueIndex] == 0.0f ? (float)(*IntegerValues)[ValueIndex] : (*FloatValues)[ValueIndex];
			}
			IntegerValues.reset();
		}
	}

	for (auto ColumnIndex = 0; ColumnIndex < NumColumns; ++ColumnIndex)
	{
		const auto& FloatValues = NewTable->mFloatValues[ColumnIndex];
		const auto& StringValues = NewTable->mStringValues[ColumnIndex];
		const auto& IntValues = NewTable->mIntegerValues[ColumnIndex];

		int DataTypeMask = static_cast<int>(ColumnDataType::Unknown);
		if (FloatValues)
		{
			DataTypeMask |= static_cast<int>(ColumnDataType::Real);
		}
		if (IntValues)
		{
			DataTypeMask |= static_cast<int>(ColumnDataType::Integer);
		}

		if (StringValues && DataTypeMask == static_cast<int>(ColumnDataType::Unknown))
		{
			DataTypeMask = static_cast<int>(ColumnDataType::Text);
		}

		NewTable->mColumnDataTypes[ColumnIndex] = static_cast<ColumnDataType>(DataTypeMask);
		
	}


	return NewTable;
}

bool StringToInteger(const std::string& String, int32_t& Output)
{
	const char* nptr = String.c_str();                     /* string to read               */
	char* endptr = NULL;                            /* pointer to additional chars  */
	int base = 10;    /* numeric base (default 10)    */

	/* reset errno to 0 before call */
	errno = 0;

	/* call to strtol assigning return to number */
	Output = strtol(nptr, &endptr, base);

	/* output original string of characters considered */
	printf("\n string : %s\n endptr : %s\n\n", nptr, endptr);

	/* test return to number and errno values */
	if (nptr == endptr)
		printf(" number : %lu  invalid  (no digits found, 0 returned)\n", Output);
	else if (errno == ERANGE && Output == LONG_MIN)
		printf(" number : %lu  invalid  (underflow occurred)\n", Output);
	else if (errno == ERANGE && Output == LONG_MAX)
		printf(" number : %lu  invalid  (overflow occurred)\n", Output);
	else if (errno == EINVAL)  /* not in all c99 implementations - gcc OK */
		printf(" number : %lu  invalid  (base contains unsupported value)\n", Output);
	else if (errno != 0 && Output == 0)
		printf(" number : %lu  invalid  (unspecified error occurred)\n", Output);
	else if (errno == 0 && nptr && !*endptr)
		printf(" number : %lu    valid  (and represents all characters read)\n", Output);
	else if (errno == 0 && nptr && *endptr != 0)
		printf(" number : %lu    valid  (but additional characters remain)\n", Output);
	else
		return true;

	return false;
}

bool StringToFloat(const std::string& String, float& Output)
{
	const char* nptr = String.c_str();                     /* string to read               */
	char* endptr = NULL;                            /* pointer to additional chars  */

	/* reset errno to 0 before call */
	errno = 0;

	/* call to strtol assigning return to number */
	Output = strtof(nptr, &endptr);

	/* output original string of characters considered */
	printf("\n string : %s\n endptr : %s\n\n", nptr,  endptr);

	/* test return to number and errno values */
	if (nptr == endptr)
		printf(" number : %f  invalid  (no digits found, 0 returned)\n", Output);
	else if (errno == ERANGE && Output == FLT_MIN)
		printf(" number : %f  invalid  (underflow occurred)\n", Output);
	else if (errno == ERANGE && Output == FLT_MAX)
		printf(" number : %f  invalid  (overflow occurred)\n", Output);
	else if (errno == EINVAL)  /* not in all c99 implementations - gcc OK */
		printf(" number : %f  invalid  (base contains unsupported value)\n", Output);
	else if (errno != 0 && Output == 0)
		printf(" number : %f  invalid  (unspecified error occurred)\n", Output);
	else if (errno == 0 && nptr && !*endptr)
		printf(" number : %f    valid  (and represents all characters read)\n", Output);
	else if (errno == 0 && nptr && *endptr != 0)
		printf(" number : %f    valid  (but additional characters remain)\n", Output);
	else
		return true;

	return false;
}

void TypedDataTable::SerialiseCell(const std::string& TempData, size_t ColumnIndex, size_t RowIndex)
{
	if (TempData.length() > 0)
	{
		int32_t IntegerResult = 0;
		float FloatResult = 0.0f;
		if (StringToInteger(TempData, IntegerResult))
		{
			AddToColumn(mIntegerValues[ColumnIndex], IntegerResult, mNumRows, RowIndex);
		}
		else if (StringToFloat(TempData, FloatResult))
		{
			AddToColumn(mFloatValues[ColumnIndex], FloatResult, mNumRows, RowIndex);
		}

		AddToColumn(mStringValues[ColumnIndex], TempData, mNumRows, RowIndex);
	}

}