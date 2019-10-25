#include "stdafx.h"
#include <Sysinfo\SysInfo.h>
#include <Sysinfo\Config.h>
#include <Sysinfo\CTime.h>
#include "pgm.h"
#include <Tiff_STL3\Src\Tiff_STL3.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

/*	This Program is just for Maji Software Simulation.
PGM format
P1    1bit/pixel b&w image, ASCII encoded, pixel values are separated by space.
P2    8bit/pixel gray image, ASCII encoded, pixel values are separated by space.
P3    8bit/pixel RGB image, ASCII encoded, pixel values are separated by space.
P4    1bit/pixel b&w image, binary encoded, pixels are packed in 8 bits.
P5    8bit/pixel gray image, binary encoded.
P6    8bit/pixel RGB image, binary encoded.
P8    8bit/pixel CMYK image, binary encoded.//Extension???
//xujy
P7	  8bit/pixel CMYK defined.

The second line could be a comment line starting with '#', or contains
the X-size and Y-size of the image.
For 8bit pixel gray/RGB images, there is an extra line that contains the
maximum value of the pixel value,
which is usually 255. Then the raw image data follows. The entire header
looks like the following:
*/

#define CMYK_P7 //New Format.

CPGM::CPGM(void)
{
	m_lpTiff = NULL;
	m_format = PGM::P0;
}

CPGM::~CPGM(void)
{
	if (m_lpTiff != NULL)
		delete m_lpTiff;
}

int CPGM::ReadTiff(char *Input)
{
	if (m_lpTiff != NULL)
		delete m_lpTiff;
	m_lpTiff = new CTiff;
	if ((int)m_lpTiff->ReadFile(Input) != 0)
		return FileNotFound;

	if (m_lpTiff->GetTagValue(SamplesPerPixel) == 1)
	{
		if (m_lpTiff->GetTagValue(BitsPerSample) == 1)
			m_format = PGM::P4;//LineArt
		else
			m_format = PGM::P5;//Gray
	}
	else if (m_lpTiff->GetTagValue(SamplesPerPixel) == 3)
		m_format = PGM::P6; //RGB
	else if (m_lpTiff->GetTagValue(SamplesPerPixel) == 4)
#ifdef CMYK_P7
		m_format = PGM::P7; //CMYK
#else
		m_format = PGM::P8; //CMYK
#endif //CMYK_P7

	m_FileName = Input;
	return NoErr;
};

int CPGM::SaveTiff(char *Output)
{
	if (m_lpTiff != NULL)
		return (int)m_lpTiff->SaveFile(Output);

	return FileNotFound;
};


int CPGM::ReadFile(char *Input)
{
	int samplesPerPixel = 0;
	int bitsPerSample = 8;
	m_FileName = Input;
	ifstream fin(Input, std::ios::binary);
	if (fin.bad())
		return FileNotFound;

	string tempstr;

	//Format
	getline(fin, tempstr, (char)0x0A);

	if (tempstr.compare("P1") == 0)
	{//1bit/pixel b&w image, ASCII encoded
		samplesPerPixel = 1;//Gray 8 bits
		m_format = PGM::P1;
	}
	else if (tempstr.compare("P2") == 0)
	{//8bit/pixel gray image, ASCII encoded
		samplesPerPixel = 1;//Gray 8 bits
		m_format = PGM::P2;
	}
	else if (tempstr.compare("P3") == 0)
	{//8bit/pixel RGB image, ASCII encoded
		samplesPerPixel = 3;//Gray 8 bits
		m_format = PGM::P3;
	}
	else if (tempstr.compare("P4") == 0)
	{//1bit/pixel b&w image, binary encoded
		samplesPerPixel = 1;
		m_format = PGM::P4;
	}
	else if (tempstr.compare("P5") == 0)
	{//Gray 8 bits
		samplesPerPixel = 1;
		m_format = PGM::P5;
	}
	else if (tempstr.compare("P6") == 0)
	{//RGB  8 bits
		samplesPerPixel = 3;
		m_format = PGM::P6;
	}
	else if (tempstr.compare("P7") == 0)
	{//CMYK 8 bits
		samplesPerPixel = 4;
		m_format = PGM::P7;
	}
	else if (tempstr.compare("P8") == 0)
	{//CMYK 8 bits
		samplesPerPixel = 4;
		m_format = PGM::P8;
	}
	else
		m_format = PGM::P0;

	while (1)
	{
		getline(fin, tempstr, (char)0x0A);//# Some Info, don't care about it.
		if (tempstr[0] != '#')
			break;
	}

	size_t index = tempstr.find(' ');
	string strWidth;
	strWidth.assign(tempstr, 0, index);
	int Width = atoi(strWidth.c_str());

	string strLength = tempstr.substr(index);
	int Length = atoi(strLength.c_str());

	getline(fin, tempstr, (char)0x0A);
	int Range = atoi(tempstr.c_str());

	if (this->m_lpTiff != NULL)
		delete this->m_lpTiff;
	this->m_lpTiff = new CTiff;
	this->m_lpTiff->CreateNew(Width, Length, 600, samplesPerPixel, bitsPerSample);

	int TotalBytes = (Width * Length * samplesPerPixel * bitsPerSample) >> 3;
	if ((m_format == PGM::P1) || (m_format == PGM::P2) || (m_format == PGM::P3))
	{//ASCII
		LPBYTE lpTemp = m_lpTiff->GetImageBuf();
		TimeCount time;
		time.Start();

		if ((m_format == PGM::P2) && (Range == 1))
		{//LineArt 
			int BufSize = Width * 2;
			LPBYTE lpBuf = new BYTE[Width];
			for (int i = 0; i < Length; i++)
			{
				fin.read((char*)lpBuf, BufSize);
				LPWORD lwTemp = (LPWORD)lpBuf;
				for (int j = 0; j < Width; j++)
					if (*(lwTemp++) & 0x1)
						*(lpTemp++) = 0;
					else
						*(lpTemp++) = 255;
			}
			delete[]lpBuf;
		}
		else
		{
			for (int i = 0; i < TotalBytes; i++)
			{
#if 1
				getline(fin, tempstr, (char)0x20);
				*(lpTemp++) = atoi(tempstr.c_str());

#else
				fin >> data;
				*(lpTemp++) = data;
#endif
			}
		}

		time.Stop();
	}
	else
	{//Bin P4, P5, P6, P7, P8
		if (Range == 255)
		{
			fin.read((char*)this->m_lpTiff->GetImageBuf(), TotalBytes);
		}
		else if (Range == 1)
		{
			LPBYTE lpBuf = new BYTE[TotalBytes];
			fin.read((char*)lpBuf, TotalBytes);
			LPBYTE lpTemp1 = lpBuf;
			LPBYTE lpTemp2 = this->m_lpTiff->GetImageBuf();
			for (int i = 0; i < TotalBytes; i++)
				if (*(lpTemp1++) == 1)
					*(lpTemp2++) = 255;
				else
					*(lpTemp2++) = 0;
			delete[]lpBuf;
		}
	}
	fin.close();

	return NoErr;
}

//Save as Binary file.
int CPGM::SaveFile(char *FileName)
{//Only for CMYK 8 bits and RGB 8 bits
	if (this->m_lpTiff == NULL)
		return FileNotFound;

	int Width = m_lpTiff->GetTagValue(ImageWidth);
	int Length = m_lpTiff->GetTagValue(ImageLength);
	int samplesPerPixel = m_lpTiff->GetTagValue(SamplesPerPixel);
	int bitsPerSample = m_lpTiff->GetTagValue(BitsPerSample);

	FILE *file = fopen(FileName, "wb+");

	char *Format[9] = { "UnKnown\n","P1\n", "P2\n", "P3\n", "P4\n", "P5\n", "P6\n", "P7\n", "P8\n" };

	fprintf(file, Format[(int)m_format]);
	fprintf(file, "#Orignal Image : %s\n", m_FileName.c_str());

	if (m_format == PGM::P7)
	{
		fprintf(file, "WIDTH %d\n", Width);
		fprintf(file, "HEIGHT %d\n", Length);
		fprintf(file, "DEPTH 4\n");
		fprintf(file, "MAXVAL 255\n");
		fprintf(file, "TUPLTYPE CMYK\n");
		fprintf(file, "ENDHDR\n");
	}
	else
	{
		fprintf(file, "%d %d\n", Width, Length);

		if (bitsPerSample == 8)
			fprintf(file, "255\n");
		else
			fprintf(file, "1\n");
	}

	int TotalBytes = (Width*Length*samplesPerPixel*bitsPerSample) >> 3;
	fwrite(m_lpTiff->GetImageBuf(), 1, TotalBytes, file);

	fclose(file);

	return NoErr;
}

//Save as ASCII file.
int CPGM::SaveFile_ASCII(char *FileName)
{//Only for CMYK 8 bits and RGB 8 bits
	if (this->m_lpTiff == NULL)
		return FileNotFound;

	int Width = m_lpTiff->GetTagValue(ImageWidth);
	int Length = m_lpTiff->GetTagValue(ImageLength);
	int samplesPerPixel = m_lpTiff->GetTagValue(SamplesPerPixel);
	int bitsPerSample = m_lpTiff->GetTagValue(BitsPerSample);

	char *Format[9] = { "UnKnown\n","P1\n", "P2\n", "P3\n", "P4\n", "P5\n", "P6\n", "P7\n", "P8\n" };

	//Color RGB
	if (m_format == PGM::P4)
		m_format = PGM::P1;

	//Gray
	if (m_format == PGM::P5)
		m_format = PGM::P2;

	//Color RGB
	if (m_format == PGM::P6)
		m_format = PGM::P3;

	FILE *file = fopen(FileName, "wb+");
	fprintf(file, Format[(int)m_format]);
	fprintf(file, "#Orignal Image : %s\n", m_FileName.c_str());
	fprintf(file, "%d %d\n", Width, Length);
	if (bitsPerSample == 8)
		fprintf(file, "255\n");
	else
		fprintf(file, "1\n");

	int BytesPerLine = (Width * samplesPerPixel * bitsPerSample) >> 3;
	LPBYTE lpTemp = m_lpTiff->GetImageBuf();

	for (int i = 0; i < Length; ++i)
	{
		for (int j = 0; j < BytesPerLine; ++j)
			fprintf(file, "%4d", (unsigned char)(*lpTemp++));

		fprintf(file, "\n");
	}
	fclose(file);

	return NoErr;

}

int CPGM::Tiff2PPM(char *Input, char*Output)
{
	this->ReadTiff(Input);
	this->SaveFile(Output);
	return NoErr;
}

int CPGM::Bin2PGM(char *InputFile, int Width, int Length, char *OutputFile)
{//Only Gray 8 bits.
	int size = Width * Length;
	char *lpBuf = new char[size];
	m_FileName = InputFile;
	FILE *In = fopen(InputFile, "rb");
	fread(lpBuf, 1, size, In);
	FILE *Out = fopen(OutputFile, "wb");
	fprintf(Out, "P2\n%d %d\n255\n", Width, Length);

	char *lpTemp = lpBuf;
	for (int i = 0; i < Length; i++)
	{
		//fprintf(Out, "%3d", (unsigned char)(*lpTemp++));
		for (int j = 0; j < Width; j++)
			fprintf(Out, "%4d", (unsigned char)(*lpTemp++));

		fprintf(Out, "\n");
	}
	delete[]lpBuf;
	fclose(In);
	fclose(Out);

	return NoErr;
}

