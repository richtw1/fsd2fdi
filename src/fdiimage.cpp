#include "fdiimage.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <utility>


FdiImage::FdiImage(const char* filename)
	: file(filename, std::ios_base::binary)
{
	// Throw an exception if it couldn't be opened
	if (file.fail())
	{
		std::ostringstream message;
		message << "Cannot open file '" << filename << "'.";
		throw std::runtime_error(message.str());
	}

	// Automatically throw exceptions from now on
	file.exceptions(std::ios_base::eofbit | std::ios_base::badbit);

}


FdiImage::Header::Header()
{
	// Initialize fixed fields of the header
	const char signatureText[] = "Formatted Disk Image file\r\n";
	assert(sizeof signatureText == signatureSize + 1);
	std::memcpy(signature, signatureText, signatureSize);

	const char creatorText[] = "Created by fsd2fdi version 0.1";
	std::memset(creator, 0x20, creatorSize);
	std::memcpy(creator, creatorText, sizeof creatorText - 1);
	cr = 0x0D;
	lf = 0x0A;

	std::memset(comment, 0x1A, commentSize);
	eof = 0x1A;

	versionHi = 2;
	versionLo = 2;
	lastTrackHi = 0;
	lastTrackLo = 0;
	lastHead = 0;
	type = 1;			// 5.25" disk
	rotSpeed = 232;		// it was set to that in an FDI file made by Disk2FDI
	flags = 0;
	tpi = 0;			// 48 tpi
	headWidth = 2;		// 96 tpi
	reserved1 = 0;
	reserved2 = 0;
}


FdiImage::Track& FdiImage::addTrack()
{
	tracks.push_back(Track());
	return tracks.back();
}


void FdiImage::Track::addRLEBlock(byte val, int num)
{
	// FDI file format maximum RLE block size is 256, so add as many as necessary
	while (num > 0)
	{
		int size = std::min(num, 256);
		data.push_back(FdiFmDescriptors::fmDecodedRleData);
		data.push_back((size == 256) ? 0 : size);
		data.push_back(val);
		num -= size;
	}
}


void FdiImage::Track::addGap(int size)
{
	// Add block of 'size' FFs
	addRLEBlock(0xFF, size);
}


void FdiImage::Track::addSync()
{
	// Add 6 sync bytes (0x00)
	addRLEBlock(0x00, 6);
}


void FdiImage::Track::addGapAndSync(int size)
{
	addGap(size);
	addSync();
}


void FdiImage::Track::addGap4()
{
	// Pad out the rest of the track to maximum capacity
	int remaining = trackSize - data.size();
	addRLEBlock(0xFF, remaining);
}


static word addToCrc(word crc, byte b)
{
#if 0
	// From: http://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
	byte x = (crc >> 8) ^ b;
	x ^= (x >> 4);
	return (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
#endif

	for (int i = 0; i < 8; i++)
	{
		crc = (crc << 1) ^ ((((crc >> 8) ^ (b << i)) & 0x0080) ? 0x1021 : 0);
	}

	return crc;
}


void FdiImage::Track::addSectorHeader(int trackId, int headId, int sectorId, int sizeId)
{
	// Calculate the header CRC, including the ID mark as a data byte in the CRC
	word crc = 0xFFFF;
	crc = addToCrc(crc, 0xFE);
	crc = addToCrc(crc, trackId);
	crc = addToCrc(crc, headId);
	crc = addToCrc(crc, sectorId);
	crc = addToCrc(crc, sizeId);

	// Add sector ID address mark
	data.push_back(FdiFmDescriptors::sectorIdMark);

	// Add 4 bytes sector IDs, plus CRC
	data.push_back(FdiFmDescriptors::fmDecodedData);
	static const int numBits = 6 * 8;
	data.push_back(numBits >> 8);
	data.push_back(numBits & 0xFF);
	data.push_back(trackId);
	data.push_back(headId);
	data.push_back(sectorId);
	data.push_back(sizeId);
	data.push_back(crc >> 8);
	data.push_back(crc & 0xFF);
}


void FdiImage::Track::addSectorData(const std::vector<byte>& sectorData, bool deletedData, bool validCrc)
{
	// Calculate the data CRC, including the data mark
	word crc = 0xFFFF;
	crc = addToCrc(crc, deletedData ? 0xF8 : 0xFB);
	crc = std::accumulate(std::begin(sectorData), std::end(sectorData), crc, [](word crc, byte b) { return addToCrc(crc, b); });

	if (!validCrc)
	{
		// ruin the CRC to deliberately cause a sector CRC error in the image.
		// we can never know what the actual disc's CRC was, but has the same result.
		crc ^= 0x1234;
	}

	// Add data address mark
	data.push_back(deletedData ? FdiFmDescriptors::deletedDataMark : FdiFmDescriptors::dataMark);

	bool isEmpty = std::all_of(std::begin(sectorData), std::end(sectorData), [](byte c) { return c == 0xE5; });
	if (isEmpty)
	{
		// Empty (unused) sector full of E5s. Size optimize by storing as many RLE blocks as required.
		addRLEBlock(0xE5, sectorData.size());
	}
	else
	{
		// Add sector data
		const int numBits = sectorData.size() * 8;
		if (numBits < 0x10000)
		{
			data.push_back(FdiFmDescriptors::fmDecodedData);
			data.push_back(numBits >> 8);
			data.push_back(numBits & 0xFF);
		}
		else
		{
			data.push_back(FdiFmDescriptors::fmDecodedData65536);
			data.push_back((numBits - 0x10000) >> 8);
			data.push_back((numBits - 0x10000) & 0xFF);
		}

		data.insert(std::end(data), std::begin(sectorData), std::end(sectorData));
	}

	// Write data CRC
	data.push_back(crc >> 8);
	data.push_back(crc & 0xFF);
}


void FdiImage::alignFrom(int currentPos, int alignment)
{
	// Aligns file pointer to the next alignment boundary from the given position
	int num = ((currentPos + (alignment - 1)) & ~(alignment - 1)) - currentPos;
	while (num--)
	{
		file.put(0);
	}
}


void FdiImage::write()
{
	// Update header to contain correct number of tracks
	header.lastTrackHi = static_cast<byte>((tracks.size() - 1) >> 8);
	header.lastTrackLo = static_cast<byte>((tracks.size() - 1) & 0xFF);

	// Write out header to file
	file.write(reinterpret_cast<char*>(&header), sizeof header);

	// Write out tracks description table
	for (const auto& track : tracks)
	{
		file.put(0xCFu);
		file.put(static_cast<char>((track.data.size() + 0xFF) >> 8));
	}

	alignFrom(sizeof header + 2 * tracks.size(), 512);

	for (const auto& track : tracks)
	{
		file.write(reinterpret_cast<const char*>(&track.data.front()), track.data.size());
		alignFrom(track.data.size(), 256);
	}
}