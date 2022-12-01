
#include "BinaryReader.h"
#include <algorithm>

namespace
{
	template <typename T>
	T ReadPOD(const uint8_t * Data, size_t DataLength, size_t& ReadPosition)
	{
		if (ReadPosition + sizeof(T) <= DataLength)
		{
			const auto* Ptr = reinterpret_cast<const T*>(&Data[ReadPosition]);
			ReadPosition += sizeof(T);
			return *Ptr;
		}
		return 0;
	}
}

BinaryReader::BinaryReader(const uint8_t * Data, size_t DataLength)
	: mData(Data)
	, mDataLength(DataLength)
	, mReadPosition(0)
{
}
uint8_t BinaryReader::ReadUInt8()
{
	return ReadPOD<uint8_t>(mData, mDataLength, mReadPosition);
}
uint32_t BinaryReader::ReadUInt32()
{
	return ReadPOD<uint32_t>(mData, mDataLength, mReadPosition);
}
int16_t BinaryReader::ReadInt16()
{
	return ReadPOD<int16_t>(mData, mDataLength, mReadPosition);
}
uint16_t BinaryReader::ReadUInt16()
{
	return ReadPOD<uint16_t>(mData, mDataLength, mReadPosition);
}
int32_t BinaryReader::ReadInt32()
{
	return ReadPOD<int32_t>(mData, mDataLength, mReadPosition);
}
uint64_t BinaryReader::ReadUInt64()
{
	return ReadPOD<uint64_t>(mData, mDataLength, mReadPosition);
}
int64_t BinaryReader::ReadInt64()
{
	return ReadPOD<int64_t>(mData, mDataLength, mReadPosition);
}
float BinaryReader::ReadFloat32()
{
	return ReadPOD<float>(mData, mDataLength, mReadPosition);
}
void BinaryReader::ReadBlob(void * DataDestination, size_t DataSize)
{
	if (mReadPosition + DataSize <= mDataLength)
	{
		memcpy(DataDestination, mData + mReadPosition, DataSize);
		mReadPosition += DataSize;
	}
}
size_t BinaryReader::GetReadPosition() const
{
	return mReadPosition;
}
size_t BinaryReader::GetDataLength() const
{
	return mDataLength;
}
void BinaryReader::Seek(size_t NewPosition)
{
	if (NewPosition < mDataLength)
	{
		mReadPosition = NewPosition;
	}
}
void BinaryReader::Advance(size_t AmountToSkip)
{
	mReadPosition = std::clamp(mReadPosition + AmountToSkip, mReadPosition, mDataLength);
}