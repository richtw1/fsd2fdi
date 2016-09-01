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


void FdiImage::Track::addGap(int size)
{
	assert(size > 0 && size <= 256);

	// Add block of 'size' FFs
	data.push_back(FdiFmDescriptors::fmDecodedRleData);
	data.push_back((size == 256) ? 0 : size);
	data.push_back(0xFF);
}


void FdiImage::Track::addSync()
{
	// Add 6 sync bytes (0x00)
	data.push_back(FdiFmDescriptors::fmDecodedRleData);
	data.push_back(6);
	data.push_back(0x00);
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
	while (remaining > 0)
	{
		int size = std::min(remaining, 256);
		addGap(size);
		remaining -= size;
	}
}


static word addToCrc(word crc, byte b)
{
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
	data.push_back(crc >> 8);
	data.push_back(crc & 0xFF);
}


void FdiImage::write()
{
}
