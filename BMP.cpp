#include "StdAfx.h"
#include <Sysinfo\SysInfo.h>
#include <Sysinfo\Config.h>
#include <Sysinfo\CTime.h>
#include "BMP.h"
#include <Tiff_STL3\Tiff_STL3.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

BMP::BMP(void)
{
	m_Header.bfType = 0x4d42;//"BM"
	m_Header.bfSize = 0;    
    m_Header.bfReserved1 = 0;
    m_Header.bfReserved2 = 0;
    m_Header.bfOffBits = 0;

	m_Info.biSize = 0;
    m_Info.biWidth = 0;
    m_Info.biHeight = 0;
    m_Info.biPlanes = 1;//default, Don't change it.
    m_Info.biBitCount = 24;//1 , 4, 8, 24
    m_Info.biCompression = 0;//No Compression.
    m_Info.biSizeImage = 0;
    m_Info.biXPelsPerMeter = 2952;//72 dpi.
    m_Info.biYPelsPerMeter = 2952;
    m_Info.biClrUsed = 0;
    m_Info.biClrImportant = 0;
	m_lpTiff = NULL;
}

BMP::~BMP(void)
{
	if(m_lpTiff != NULL)
		delete m_lpTiff;
}

int BMP::ReadTiff(char *Input)
{
	if(m_lpTiff != NULL)
		delete m_lpTiff;

	m_lpTiff = new CTiff;

	if((int)m_lpTiff->ReadFile(Input) != 0)
		return FileNotFound;

	//Initial BMP Info
	int Width = m_lpTiff->GetTagValue(ImageWidth);
	int Length = m_lpTiff->GetTagValue(ImageLength);
	int Samples = m_lpTiff->GetTagValue(SamplesPerPixel);
	int Bits = m_lpTiff->GetTagValue(BitsPerSample);
	int Resolution = m_lpTiff->GetTagValue(XResolution);

	int SizeImage = (Width * Length * Samples * Bits) / 8 ;
	//Initial Header, Info.
	m_Info.biSize = sizeof(BITMAPINFOHEADER);//default 0x28;
	m_Info.biWidth = Width;
	m_Info.biHeight = Length;
	m_Info.biSizeImage = SizeImage;
	m_Info.biXPelsPerMeter = (LONG)(Resolution * 100 /2.54);
	m_Info.biYPelsPerMeter = (LONG)(Resolution * 100 /2.54);

	if(Samples == 3)
	{//Color
		int HeaderSize = sizeof(m_Header);
		int InfoSize = sizeof(m_Info);
		m_Header.bfSize  = SizeImage +  HeaderSize + InfoSize;
		m_Header.bfOffBits = 0x36;
		m_Info.biBitCount = 24;
		
	}
	else
	{//Gray
		m_Header.bfSize  = SizeImage + sizeof(m_Header) + sizeof(m_Info) + 4 * 256;//Color Table
		m_Header.bfOffBits = 0x436;	
		m_Info.biBitCount = 8;
	}

	return NoErr;
};

int BMP::SaveTiff(char *Output)
{
	if(m_lpTiff != NULL)
		return (int)m_lpTiff->SaveFile(Output);

	return FileNotFound;
};


int BMP::ReadFile(char *Input)
{
	FILE *file = fopen(Input, "rb");
	if(file == NULL)
		return FileNotFound;

	fread(&m_Header, sizeof(m_Header), 1, file);
	fread(&m_Info, sizeof(m_Info), 1, file);

	int Width = m_Info.biWidth;
	int Length = m_Info.biHeight;
	int Sample, Bits;
	switch(m_Info.biBitCount)
	{
	case 1: Sample = 1;Bits = 1;break;
	case 8: Sample = 1;Bits = 8;break;
	case 24:
	default: Sample = 3;Bits = 8;break;
	}
	
	if(m_lpTiff != NULL)
		delete m_lpTiff;
	m_lpTiff = new CTiff;	
	
	m_lpTiff->CreateNew(Width, Length, (int)(m_Info.biXPelsPerMeter*2.54/100), Sample, Bits);
	
	int BytesPerLine = Width * Sample * Bits / 8;
	if(Sample == 3)
	{
		LPBYTE lpBuf1 = new BYTE[BytesPerLine];
		LPBYTE lpBuf2 = new BYTE[BytesPerLine];
		for(int i = Length - 1; i >= 0; i--)
		{
			fread(lpBuf1, 1, BytesPerLine, file);
			BGR2RGB(lpBuf1, lpBuf2, Width);
			m_lpTiff->PutRow(lpBuf2, i);
		}
		delete []lpBuf1;
		delete []lpBuf2;
	}
	else
	{
		LPBYTE lpBuf = new BYTE[BytesPerLine];
		fseek(file, m_Header.bfOffBits, SEEK_SET);		
		for(int i = Length - 1; i >= 0; i--)
		{
			fread(lpBuf, 1, BytesPerLine, file);			
			m_lpTiff->PutRow(lpBuf, i);
		}
		delete []lpBuf;	
	}

	fclose(file);

	return NoErr;
}

//Save as Binary file.
int BMP::SaveFile(char *FileName)
{//Only for CMYK 8 bits and RGB 8 bits
	FILE *file = fopen(FileName, "wb+");
	fwrite(&m_Header, sizeof(m_Header), 1, file);
	fwrite(&m_Info, sizeof(m_Info), 1, file);
	
	int Width = m_lpTiff->GetTagValue(ImageWidth);
	int Length = m_lpTiff->GetTagValue(ImageLength);
	int Samples = m_lpTiff->GetTagValue(SamplesPerPixel);
	int Bits = m_lpTiff->GetTagValue(BitsPerSample);
	int BytesPerLine = (Width * Samples * Bits) / 8;

	LPBYTE lpBuf = new BYTE[BytesPerLine];
	
	BYTE temp = 0;
	if(Samples == 3)
	{
		LPBYTE lpOut = new BYTE[BytesPerLine];
		for(int i = Length - 1; i >= 0; i--)
		{
			m_lpTiff->GetRow(lpBuf, i);	
			RGB2BGR(lpBuf, lpOut, Width);
			fwrite(lpOut, 1, BytesPerLine, file);
		}
		delete []lpOut;
	}
	else
	{
		BYTE Table[256*4];
		LPBYTE lpTemp = Table;
		for(int i = 0; i < 256; i++)
		{
			*(lpTemp++) = i;
			*(lpTemp++) = i;
			*(lpTemp++) = i;
			*(lpTemp++) = 0;
		}
		fwrite(Table, 1, 256*4, file);			
		for(int i = Length - 1; i >= 0; i--)
		{
			m_lpTiff->GetRow(lpBuf, i);	
			fwrite(lpBuf, 1, BytesPerLine, file);
		}	
	}

	fclose(file);
	delete []lpBuf;
	
	return NoErr;
}

//Same as RGB2BGR.
void BMP::BGR2RGB(LPBYTE lpIn, LPBYTE lpOut, int Size)
{
	RGB2BGR(lpIn, lpOut, Size);
}
void BMP::RGB2BGR(LPBYTE lpIn, LPBYTE lpOut, int Size)
{	
	for(int i = 0; i < Size; i++)
	{
		*(lpOut) = *(lpIn+2);
		*(lpOut+1) = *(lpIn+1);
		*(lpOut+2) = *(lpIn);
		lpIn += 3;
		lpOut += 3;
	}
}
//Save as ASCII file.
int BMP::SaveFile_ASCII(char *FileName)
{//Only for CMYK 8 bits and RGB 8 bits


	return NoErr;

}

int BMP::Tiff2BMP(char *Input, char*Output)
{
	return NoErr;
}

int BMP::Bin2BMP(char *InputFile, int Width, int Length, char *OutputFile)
{//Only Gray 8 bits.
	int size = Width * Length;


	return NoErr;
}

#ifdef BMP_TEST

BMP_Test::BMP_Test()
{				 
	Initial();

}

BMP_Test::~BMP_Test()
{

}

int BMP_Test::Initial()
{	return 1;}

int BMP_Test::main()
{	
	char BMP_In[128];
	char BMP_Out[128];
	char Tiff_Out[128];
	char Tiff_In[128];

	CBase::main();	
	ConGetStr2("BMP_In",   "In.bmp", BMP_In);
	ConGetStr2("BMP_Out",  "Out.bmp", BMP_Out);
	ConGetStr2("Tiff_Out", "Out.tif", Tiff_Out);
	ConGetStr2("Tiff_In",  "In.tif", Tiff_In);

	BMP m_Bmp;	
	m_Bmp.ReadFile(BMP_In);
	m_Bmp.SaveTiff(Tiff_Out);
	m_Bmp.ReadTiff(Tiff_In);
	m_Bmp.SaveFile(BMP_Out);

	cout << "*************** BMP Test end *************" << endl;

	return NoErr;
}

#endif //BMP_TEST

