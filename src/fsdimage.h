#ifndef FSDIMAGE_H_
#define FSDIMAGE_H_

#include <string>
#include <utility>
#include <vector>

#include "types.h"


class FsdImage
{
public:

	explicit FsdImage(const char* filename);

	class Sector
	{
		friend class FsdImage;

	public:
		int getTrackId() const { return trackId; }
		int getHeadNumber() const { return headNumber; }
		int getSectorId() const { return sectorId; }
		int getSizeId() const { return sizeId; }
		int getSize() const { return 1 << (7 + sizeId); }
		int getRealSize() const { return 1 << (7 + realSize); }
		bool isDeletedData() const { return (errorCode == 0x20); }
		bool hasCrcError() const { return (errorCode == 0x0E); }
		const std::vector<byte>& getData() const { return data; }

	private:
		int trackId;
		int headNumber;
		int sectorId;
		int sizeId;
		int realSize;
		int errorCode;
		std::vector<byte> data;
	};

	class Track
	{
		friend class FsdImage;

	public:
		int getTrackNumber() const { return trackNumber; }
		bool isReadable() const { return readable; }
		const std::vector<Sector>& getSectors() const { return sectors; }
		int getNumSectors() const { return sectors.size(); }

	private:
		int trackNumber;
		bool readable;
		std::vector<Sector> sectors;
	};

	struct CreationData
	{
		int day;
		int month;
		int year;
		int creatorId;
		int releaseNum;
	};

	const CreationData& getCreationData() const { return creation; }
	const std::string& getTitle() const { return title; }
	const std::vector<Track>& getTracks() const { return tracks; }
	int getNumTracks() const { return tracks.size(); }

private:

	CreationData creation;
	std::string title;
	std::vector<Track> tracks;
};

#endif // ifndef FSDIMAGE_H_
