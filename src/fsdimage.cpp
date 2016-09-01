#include "fsdimage.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>


FSDImage::FSDImage(const char* filename)
{
	// Open FSD file
	std::ifstream file(filename, std::ios_base::binary);
	
	// Throw an exception if it couldn't be opened (e.g. because it doesn't exist)
	if (file.fail())
	{
		std::ostringstream message;
		message << "Cannot open file '" << filename << "'.";
		throw std::runtime_error(message.str());
	}

	// Automatically throw exceptions from now on
	file.exceptions(std::ios_base::eofbit | std::ios_base::badbit);
	
	// Check that header is correct
	char fsdId[3];
	file.read(fsdId, 3);
	if (fsdId[0] != 'F' || fsdId[1] != 'S' || fsdId[2] != 'D')
	{
		std::ostringstream message;
		message << "File '" << filename << "' is not a valid FSD file.";
		throw std::runtime_error(message.str());
	}

	// Read creator data
	byte data[5];
	file.read(reinterpret_cast<char*>(data), 5);

	creation.day = (data[0] >> 3);
	creation.month = (data[2] & 0x0F);
	creation.year = ((data[0] & 0x07) << 8) | data[1];
	creation.creatorId = (data[2] >> 4);
	creation.releaseNum = ((data[4] >> 6) << 8) | data[3];	// @todo: seems wrong

	// Read title
	std::getline(file, title, '\0');

	// Read number of tracks
	int numTracks = file.get();

	// Read tracks one-by-one
	for (int t = 0; t < numTracks; t++)
	{
		Track track;
		track.trackNumber = file.get();
		int numSectors = file.get();
		track.readable = (file.get() == 0xFF);

		// Read sectors for this track
		for (int s = 0; s < numSectors; s++)
		{
			Sector sector;
			sector.trackId = file.get();
			sector.headNumber = file.get();
			sector.sectorId = file.get();
			sector.sizeId = file.get();

			if (track.readable)
			{
				// These fields only exist for readable tracks
				sector.realSize = file.get();
				sector.errorCode = file.get();
				sector.data.resize(sector.getRealSize());
				file.read(reinterpret_cast<char*>(&sector.data.front()), sector.getRealSize());
			}

			track.sectors.push_back(std::move(sector));
		}

		tracks.push_back(std::move(track));
	}

	// @todo: some kind of heuristic which detects a 'normal' disk image and performs a track-by-track sector skew
	// (as the FSD format loses this information by rotating sectors into position so that logical sector 0 always appears first)
}


bool FSDImage::Sector::isEmpty() const
{
	// 8271 initializes newly formatted sectors to E5 bytes.
	// If it is in this state, we can choose to run-length encode it to save space.
	return std::all_of(std::begin(data), std::end(data), [](char c) { return c == 0xE5; });
}