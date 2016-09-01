#ifndef FDIIMAGE_H_
#define FDIIMAGE_H_

#include <fstream>
#include <vector>

#include "types.h"


class FDIImage
{
public:

	explicit FDIImage(const char* filename);

	void write();

	class Sector
	{
	};

	class Track
	{
		std::vector<Sector> sectors;
	};

private:

	struct Header
	{
		Header();

		static const std::size_t signatureSize = 27;
		static const std::size_t creatorSize = 30;
		static const std::size_t commentSize = 80;

		char signature[signatureSize];
		char creator[creatorSize];
		char cr;
		char lf;
		char comment[commentSize];
		char eof;
		byte versionHi;
		byte versionLo;
		byte lastTrackHi;
		byte lastTrackLo;
		byte lastHead;
		byte type;
		byte rotSpeed;
		byte flags;
		byte tpi;
		byte headWidth;
		byte reserved1;
		byte reserved2;
	};

	std::ofstream file;
	Header header;
	std::vector<Track> tracks;
};

#endif // ifndef FDIIMAGE_H_
