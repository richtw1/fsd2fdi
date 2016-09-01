#ifndef FDIIMAGE_H_
#define FDIIMAGE_H_

#include <fstream>
#include <vector>

#include "types.h"


class FdiImage
{
public:

	explicit FdiImage(const char* filename);

	class Track
	{
		friend class FdiImage;

	public:
		void addGap1AndSync(int size) { addGapAndSync(size); }
		void addSectorHeader(int trackId, int headId, int sectorId, int sizeId);
		void addGap2AndSync() { addGapAndSync(11); }
		void addSectorData(const std::vector<byte>& sectorData, bool deletedData, bool validCrc);
		void addGap3AndSync(int size) { addGapAndSync(size); }
		void addGap4();

	private:
		void addRLEBlock(byte val, int num);
		void addGap(int size);
		void addSync();
		void addGapAndSync(int size);

		// Descriptor values defined by the FDI format
		struct FdiFmDescriptors
		{
			static const byte sectorIdMark = 0x07;
			static const byte dataMark = 0x05;
			static const byte deletedDataMark = 0x02;
			static const byte fmDecodedData = 0x0C;
			static const byte fmDecodedData65536 = 0x0D;
			static const byte fmDecodedRleData = 0x09;
		};

		// Total data size in bytes of an FM track
		static const int trackSize = 3125;

		std::vector<byte> data;
	};

	void setComment(const char* text);
	Track& addTrack();
	void write();

private:

	void alignFrom(int currentPos, int alignment);

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
