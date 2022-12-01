#pragma once

#include <stdint.h>

class BinaryReader final
{
public:

	explicit BinaryReader(const uint8_t* Data, size_t DataLength);

	uint8_t ReadUInt8();
	uint32_t ReadUInt32();
	int16_t ReadInt16();
	uint16_t ReadUInt16();
	int32_t ReadInt32();
	uint64_t ReadUInt64();
	int64_t ReadInt64();

	float ReadFloat32();

	void ReadBlob(void* DataDestination, size_t DataSize);

	size_t GetReadPosition() const;
	size_t GetDataLength() const;
	void Seek(size_t NewPosition);
	void Advance(size_t AmountToSkip);

private:

	const uint8_t* mData;
	size_t mDataLength;
	size_t mReadPosition;
};