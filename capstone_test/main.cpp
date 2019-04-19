#include <setjmp.h>
#include <iostream>
#include "turbojpeg.h"
#include <fstream>
#include <string>
#include <windows.h>


#define PAD 2


struct IntBuffer
{
	unsigned int* buffer; //Data Buffer
	int size;              //Size of Buffer

	void init(long length)
	{
		//Create buffer of specified size
		buffer = new unsigned int[length];

		for (int i = 0; i < length; i++)
		{
			buffer[i] = 0;
		}

		//Store Size of Buffer
		size = length;
	}

	void destroy()
	{
		//Delete Buffer
		delete buffer;

		//Set Size to 0
		size = 0;
	}
};

struct CharBuffer
{
	unsigned char* buffer; //Data Buffer
	int size;              //Size of Buffer

	void init(long length)
	{
		//Create buffer of specified size
		buffer = new unsigned char[length];

		//Store Size of Buffer
		size = length;
	}

	void init(unsigned char *buf)
	{
		buffer = buf;
	}

	void destroy()
	{
		//Delete Buffer
		delete buffer;

		//Set Size to 0
		size = 0;
	}
};

struct ImageBuffer
{
	unsigned char* buffer; //Data Buffer
	int size;              //Size of Buffer

	int width;
	int height;
	int subSampling;
	int colorSpace;
	int pitch;

	void init(long length)
	{
		//Create buffer of specified size
		buffer = new unsigned char[length];

		//Store Size of Buffer
		size = length;
	}

	void init(unsigned char *buf)
	{
		buffer = buf;
	}

	void destroy()
	{
		//Delete Buffer
		delete buffer;

		//Set Size to 0
		size = 0;
	}
};


///////////////////////////
// readfile
// Inputs:
//        filename : *char
//
// Return:
//        *CharBuffer
///////////////////////////
CharBuffer* readFileJPG(char *filename)
{
	//Initialize file stream object
	std::ifstream infile;
	infile.open(filename, std::ios::binary);

	//If Infile has not opened return nullptr
	if (!infile)
		return nullptr;

	//Move to end of file
	infile.seekg(0L, std::ios::end);

	//Create new file buffer
	CharBuffer* buffer = new CharBuffer;

	//Initialize buffer with size of file
	buffer->init((long)infile.tellg());

	//Move to beggining of file
	infile.seekg(0L, std::ios::beg);

	//Read file into memory
	infile.read((char*)buffer->buffer, buffer->size);

	//Close file
	infile.close();

	//Return File buffer
	return buffer;
}


///////////////////////
// readFileBMP
// Inputs:
//        filename : char*
//
// Return:
//        CharBuffer*
///////////////////////
ImageBuffer* readFileBMP(char *filename)
{
	//Initialize file stream object
	std::ifstream infile;
	infile.open(filename, std::ios::binary);

	//If Infile has not opened return nullptr
	if (!infile)
		return nullptr;

	tagBITMAPFILEHEADER *header1 = new tagBITMAPFILEHEADER;
	tagBITMAPINFOHEADER *header2 = new tagBITMAPINFOHEADER;

	infile.read((char*)header1, sizeof(tagBITMAPFILEHEADER));
	infile.read((char*)header2, sizeof(tagBITMAPINFOHEADER));

	infile.close();

	ImageBuffer *buffer = new ImageBuffer;

	buffer->width = header2->biWidth;
	buffer->height = header2->biHeight;
	buffer->pitch = buffer->width * 3;

	int *pixelFormat = new int;
	*pixelFormat = TJPF_RGB;

	buffer->size = buffer->height * buffer->width * 3;

	buffer->init(tjLoadImage(filename, &buffer->width, 1, &buffer->height, pixelFormat, TJFLAG_STOPONWARNING));

	//std::cout << tjGetErrorStr();

	if (buffer->buffer == NULL)
		return nullptr;

	delete header1;
	delete header2;
	delete pixelFormat;

	return buffer;
}

///////////////////////////////////
// decompressYUV
// Inputs:
//        fileBuffer : *CharBuffer
//
// Return:
//        *ImageBuffer
////////////////////////////////////
ImageBuffer* decompressYUV(CharBuffer *fileBuffer)
{
	//Initialize Decompression Object
	tjhandle handle = tjInitDecompress();

	//Create Image Buffer
	ImageBuffer *imageBuffer = new ImageBuffer;

	//Read Header from File
	tjDecompressHeader3(handle, fileBuffer->buffer, fileBuffer->size, &imageBuffer->width, &imageBuffer->height, &imageBuffer->subSampling, &imageBuffer->colorSpace);

	//Initialize image buffer with size of YUV
	imageBuffer->init(tjBufSizeYUV2(imageBuffer->width, PAD, imageBuffer->height, imageBuffer->subSampling));

	//Decompress Image data to image buffer
	tjDecompressToYUV2(handle, fileBuffer->buffer, fileBuffer->size, imageBuffer->buffer, imageBuffer->width, PAD, imageBuffer->height, TJFLAG_STOPONWARNING);

	//Destroy Decompressor
	tjDestroy(handle);

	//Delete fileBuffer
	fileBuffer->destroy();
	delete fileBuffer;

	//Return Image Buffer
	return imageBuffer;
}


//////////////////////////
// decompressRGB
// Inputs:
//        fileBuffer : *CharBuffer
//
// Return:
//        *ImageBuffer
//////////////////////////
ImageBuffer* decompressRGB(CharBuffer *fileBuffer)
{
	//Initialize Decompression Object
	tjhandle handle = tjInitDecompress();

	//Create Image Buffer
	ImageBuffer *imageBuffer = new ImageBuffer;

	//Read Header from File
	tjDecompressHeader3(handle, fileBuffer->buffer, fileBuffer->size, &imageBuffer->width, &imageBuffer->height, &imageBuffer->subSampling, &imageBuffer->colorSpace);

	//Create pitch
	int pitch = imageBuffer->width * tjPixelSize[TJPF_RGB];

	//Initialize image buffer
	imageBuffer->init(pitch * imageBuffer->height);

	//Decompress Image data to image buffer
	tjDecompress2(handle, fileBuffer->buffer, fileBuffer->size, imageBuffer->buffer, imageBuffer->width, pitch, imageBuffer->height, TJPF_RGB, TJFLAG_STOPONWARNING);

	//Destroy Decompressor
	tjDestroy(handle);

	//Delete fileBuffer
	fileBuffer->destroy();
	delete fileBuffer;

	//Return Image Buffer
	return imageBuffer;
}


////////////////////////////////
// generateHistogramYUV
// Inputs:
//        imageBuffer : *ImageBuffer
//
// Return:
//        *IntBuffer
////////////////////////////////
IntBuffer* generateHistogramYUV(ImageBuffer *imageBuffer)
{
	IntBuffer *CSVBuffer = new IntBuffer;

	CSVBuffer->init(256 * 3);

	//Loop through the 
	for (int i = 0; i < imageBuffer->size; i++)
	{
		//Add one to value stored within 
		CSVBuffer->buffer[(255 * ((i + 2) % 3)) + imageBuffer->buffer[i]]++;
	}

	return CSVBuffer;
}


////////////////////////////////////////
// generateHistogramRGB
// Inputs:
//        imageBuffer : *ImageBuffer
//
// Return:
//        *IntBuffer
///////////////////////////////////////
IntBuffer* generateHistogramRGB(ImageBuffer *imageBuffer)
{
	IntBuffer *CSVBuffer = new IntBuffer;

	CSVBuffer->init(256 * 4);

	//Loop through the 
	for (int i = 0; i < imageBuffer->size; i++)
	{
		//Add one to value stored within 
		CSVBuffer->buffer[(256 * ((i + 2) % 3)) + imageBuffer->buffer[i]]++;

		if ((i + 2) % 3 == 0)
		{
			int average = imageBuffer->buffer[i] + imageBuffer->buffer[i + 1] + imageBuffer->buffer[i + 2];

			average /= 3;

			CSVBuffer->buffer[(256 * 3) + average]++;
		}
	}

	return CSVBuffer;
}


///////////////////
// writeCSV
// Inputs:
//        buffer   : *IntBuffer
//        filename : *char
//
// Return:
//        void
///////////////////
void writeCSV(IntBuffer *buffer, char *filename)
{
	//Create file stream
	std::ofstream outfile;
	outfile.open(filename, std::ios::trunc);

	for (int i = 0; i < buffer->size; i++)
	{
		std::string data;

		data = std::to_string(buffer->buffer[i]);
		data += ',';

		outfile << data;
	}

	outfile.close();
}


//////////////////////////////
// generateDifferenceMap
// Inputs:
//        imageBuffer1 : *ImageBuffer
//        imageBuffer2 : *ImageBuffer
// 
// Return:
//        *ImageBuffer
//////////////////////////////
ImageBuffer* generateDifferenceMap(ImageBuffer *imageBuffer1, ImageBuffer *imageBuffer2)
{
	ImageBuffer *bmp = new ImageBuffer;

	bmp->width = imageBuffer1->width;
	bmp->height = imageBuffer1->height;
	bmp->pitch = imageBuffer1->width;
	bmp->colorSpace = TJPF_GRAY;

	bmp->init((imageBuffer1->size / 3 > imageBuffer2->size / 3) ? (imageBuffer2->size / 3) : (imageBuffer1->size / 3));

	for(int i = 0; i < imageBuffer1->size && i < imageBuffer2->size; i += 3)
	{
		int average1 = (imageBuffer1->buffer[i] + imageBuffer1->buffer[i + 1] + imageBuffer1->buffer[i + 2]) / 3;

		int average2 = (imageBuffer2->buffer[i] + imageBuffer2->buffer[i + 1] + imageBuffer2->buffer[i + 2]) / 3;
	
		bmp->buffer[i / 3] = abs(average1 - average2);
	}

	imageBuffer1->destroy();
	delete imageBuffer1;

	tjFree(imageBuffer2->buffer);
	delete imageBuffer2;

	return bmp;
}


///////////////////
// writeBMP
// Inputs:
//        buffer   : *ImageBuffer
//        filename : *char
//
// Return:
//        void
///////////////////
void writeBMP(ImageBuffer *buffer, char *filename)
{
	tjSaveImage(filename, buffer->buffer, buffer->width, buffer->pitch, buffer->height, buffer->colorSpace, TJFLAG_STOPONWARNING);
}


///////////////////////////////////////
// main
// Inputs:
//        argc : int
//        argv : *char
//                    base command       [0]
//                    input  jpeg   path [1]
//                    input  bitmap path [2]
//                    output csv1   path [3]
//                    output csv2   path [4]
//                    output bitmap path [5]
//
// Return:
//       int
int main(int argc, char* argv[])
{
	//If there are six arguments
	if (argc == 6)
	{
		//Read in and decompress jpeg file
		ImageBuffer *jpegImage = decompressRGB(readFileJPG(argv[1]));

		//Create a histogram of the jpeg and write to csv
		writeCSV(generateHistogramRGB(jpegImage), argv[3]);

		//Read in Bitmap file
		ImageBuffer *bitmapImage = readFileBMP(argv[2]);

		//Create a histogram of the bitmap and write to csv
		writeCSV(generateHistogramRGB(bitmapImage), argv[4]);

		//Generate difference map from jpeg and bitmap image
		writeBMP(generateDifferenceMap(jpegImage, bitmapImage), argv[5]);
	}
	//If there are fewer then six arguments
	else if (argc < 6)
		//return error code one
		return 1;

	//Else there are more than six arguments
	else
		//return error code one
		return 1;

	return 0;
}