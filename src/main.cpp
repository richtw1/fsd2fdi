#include "fdiimage.h"
#include "fsdimage.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <string>


void convert(const char* fsdFilename, const char* fdiFilename, bool verbose)
{
	// Open FSD file
	FsdImage fsd(fsdFilename);

	if (verbose)
	{
		std::cout << "Title: " << fsd.getTitle() << std::endl;
		std::cout << "Day: " << fsd.getCreationData().day << std::endl;
		std::cout << "Month: " << fsd.getCreationData().month << std::endl;
		std::cout << "Year: " << fsd.getCreationData().year << std::endl;
		std::cout << "Id: " << fsd.getCreationData().creatorId << std::endl;
		std::cout << "Release: " << fsd.getCreationData().releaseNum << std::endl;
		std::cout << "Num tracks: " << fsd.getNumTracks() << std::endl;
		std::cout << std::endl;
	}

	// Create FDI file
	FdiImage fdi(fdiFilename);

	fdi.setComment(fsd.getTitle().c_str());

	// @todo: calculate best gap sizes based on total data size
	// For now these are the recommended defaults for a regular DFS disc
	const int gap1Size = 16;
	const int gap3Size = 21;

	for (const auto& track : fsd.getTracks())
	{
		// @todo: support unformatted / unreadable tracks

		FdiImage::Track& fdiTrack = fdi.addTrack();
		fdiTrack.addGap1AndSync(gap1Size);

		int sectorCount = 0;
		int numSectors = track.getNumSectors();
		for (const auto& sector : track.getSectors())
		{
			fdiTrack.addSectorHeader(sector.getTrackId(), sector.getHeadNumber(), sector.getSectorId(), sector.getSizeId());
			fdiTrack.addGap2AndSync();
			fdiTrack.addSectorData(sector.getData(), sector.isDeletedData(), sector.hasCrcError());
			sectorCount++;
			if (sectorCount < numSectors)
			{
				// Don't add Gap 3 after the final sector
				fdiTrack.addGap3AndSync(gap3Size);
			}
		}

		fdiTrack.addGap4();
	}

	fdi.write();
}


int main(int argc, char* argv[])
{
	// Check parameters correct
	if (argc != 2 && argc != 3)
	{
		std::cerr << "Syntax: fsd2fdi <fsd filename> [<fdi filename>]" << std::endl;
		return 1;
	}

	try
	{
		if (argc == 3)
		{
			// Supplied an explicit output filename
			convert(argv[1], argv[2], false);
		}
		else
		{
			// Build output filename from input filename
			static const char fsdExtension[] = ".fsd";
			static const char fdiExtension[] = ".fdi";

			std::string fdiFilename = argv[1];
			auto it = std::find_end(std::begin(fdiFilename), std::end(fdiFilename), fsdExtension, fsdExtension + 4, [](char a, char b) { return std::tolower(a) == std::tolower(b); });
			if (it != fdiFilename.end())
			{
				// Replace extension in-place
				fdiFilename.replace(it, it + 4, fdiExtension);
			}
			else
			{
				// Append extension to end of input filename as .fsd wasn't found in the string
				fdiFilename += fdiExtension;
			}

			convert(argv[1], fdiFilename.c_str(), false);
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
