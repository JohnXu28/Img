#pragma once
#include <Tiff_STL3\Src\Tiff_STL3.h>

enum PGM_format
{
	PGM_UnKnown,
	P1,//1bit/pixel b&w image, ASCII encoded, pixel values are separated by space
	P2,//8bit/pixel gray image, ASCII encoded, pixel values are separated by space
	P3,//8bit/pixel RGB image, ASCII encoded, pixel values are separated by space
	P4,//1bit/pixel b&w image, binary encoded, pixels are packed in 8 bits
	P5,//8bit/pixel gray image, binary encoded
	P6,//8bit/pixel RGB image, binary encoded
	P7,//8bit or 1bit/pixel CMYK image, binary encoded *** Defined, 
	P8 //8bit or 1bit/pixel CMYK image, binary encoded *** Undfined, old format.
};

class CPGM
{
public:
	CPGM(void);
	~CPGM(void);
	int ReadTiff(char *Input);
	int SaveTiff(char *Output);
	int ReadFile(char *Inupt);
	int SaveFile(char *FileName);
	int SaveFile_ASCII(char *FileName);
	int Tiff2PPM(char *Input, char*Output);
	int Bin2PGM(char *InputFile, int Width, int Length, char *OutputFile);
	CTiff *m_lpTiff;	
private:

	PGM_format m_format;
	string m_FileName;
};
