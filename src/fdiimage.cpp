#include "fdiimage.h"

#include <cassert>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>


FDIImage::FDIImage(const char* filename)
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


FDIImage::Header::Header()
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


void FDIImage::write()
{
}
