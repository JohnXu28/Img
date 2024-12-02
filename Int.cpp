// Int.cpp: implementation of the CInt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Int.h"
#include <Sysinfo/Config.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;
//#include "tiff_stl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CInt::CInt()
{
	Initial();
	hdr.resolution = 600;
	hdr.units = INCH;
	hdr.aspect = 0;
	hdr.reversed = 0;
	hdr.white = 0;
	m_Tiff2Int = 0;
	m_Int2Tiff = 0;
}

CInt::~CInt()
{

}

#define OpStr(Key, Default, lpBuf) ConGetStr2(Key, Default, lpBuf)

/*
const char* CInt::Name()
{
return typeid(*this).name();
return "CInt"
}
*/
int CInt::Initial()
{
	return 1;
}

int CInt::main()
{
	CBase::main();

	m_Int2Tiff = ConGetInt2("Int2Tiff", 0);
	if (m_Int2Tiff != 0)
	{
		ConGetStr2("InputInt", "input.int", InputFileName);
		ConGetStr2("OutputTiff", "input.int", OutputFileName);
		ReadFile(InputFileName);
		SaveTiff(OutputFileName);
	}

	m_Tiff2Int = ConfigGetInt2("CInt", "Tiff2Int", 0);
	if (m_Tiff2Int != 0)
	{
		ConGetStr2("InputTiff", "input.int", InputFileName);
		ConGetStr2("OutputInt", "input.int", OutputFileName);
		ReadTiff(InputFileName);
		SaveFile(OutputFileName);
	}
	cout << "*************** CInt end *************" << endl;
	return NoErr;
}

int CInt::SaveFile(LPCTSTR FileName)
{
	FILE* file;
	file = fopen(FileName, "wb+");
	if (file == NULL)
	{
		cout << "File Open Err : " << FileName;
		return -1;
	}
	put_int_hdr(file, &hdr, NULL);
	WriteIntImage(file);
	fclose(file);
	return 1;
}

int CInt::ReadFile(LPCTSTR FileName)
{
	FILE* file;
	file = fopen(FileName, "rb+");
	get_int_hdr(file, &hdr);
	m_Tiff.CreateNew(hdr.pixels, hdr.scanlines, 300, hdr.imageparts, hdr.bits_per_sample, 1);
	int ReadSize = hdr.pixels * hdr.scanlines * hdr.imageparts * hdr.bits_per_sample / 8;

	//Read Image
	ReadIntImage(file, hdr.pixels, hdr.scanlines, hdr.imageparts, hdr.bits_per_sample);
	if (hdr.photometry == cielab)
		m_Tiff.SetTag(PhotometricInterpretation, Short, 1, 8);

	fclose(file);

	return 1;
}

int CInt::SaveTiff(LPCTSTR FileName)
{
	m_Tiff.SaveFile(FileName);
	return 1;
}

int CInt::ReadTiff(LPCTSTR FileName)
{
	m_Tiff.ReadFile(FileName);
	TiffHeader2IntHeader();
	return 1;
}


int CInt::get_int_hdr(FILE* fp, INT_HDR* hdr)
{
	enum class CLAUSE { BODY, ID, ENCODING, RASTER, PHOTOMETRY, ORIGIN };

	CLAUSE clause = CLAUSE::BODY;
	int x, rc;
	char buf[81], s1[81], s2[81], * p, * q;

	if (!hdr)  hdr = (INT_HDR*)calloc(1, sizeof(INT_HDR));
	//    assert( hdr);
	FOREVER
	{
		p = fgets(buf, 81, fp);
		if (NULL == p)
		{
			free(hdr);
			return NULL;
		}
		rc = sscanf(p, "%80s %80s", s1, s2);
		if (0 == rc)
			continue;
		if ('[' == *s1)
		{
			assert(1 == rc);
			if (!COMPARE("[Raster]", s1)) clause = CLAUSE::RASTER;
			elif(!COMPARE("[ID]", s1)) clause = CLAUSE::ID;
			elif(!COMPARE("[Photometry]",s1)) clause = CLAUSE::PHOTOMETRY;
			elif(!COMPARE("[End]", s1)) break;
			elif(!COMPARE("[Encoding]",s1)) clause = CLAUSE::ENCODING;
			elif(!COMPARE("[Origin]",s1)) clause = CLAUSE::ORIGIN;
			else
			{
				free(hdr);
				return NULL;
			}
			continue;
		}

		switch (clause)
		{
		case CLAUSE::BODY:
			free(hdr);
			return NULL;
		case CLAUSE::ENCODING:
			if (!(COMPARE("imageParts:",s1)))
				(*hdr).imageparts = atoi(s2);
			elif(!(COMPARE("interleave:",s1)));
			elif(!(COMPARE("bits/sample:",s1)))
				(*hdr).bits_per_sample = atoi(s2);
			elif(!(COMPARE("bytes/sl:", s1)))
				(*hdr).bytes_per_sl = atoi(s2);
			elif(!(COMPARE("name:",s1)))
			{
				if (COMPARE("packed",s2))
				{
					free(hdr);
					return NULL;
				}
			}
			else
			{
				free(hdr);
				return NULL;
			}
			break;
		case CLAUSE::ID:
			if (!COMPARE("format:", s1))
			{
				if (!COMPARE("internal", s2));
				else
				{
					free(hdr);
					return NULL;
				}
			}
			elif(!COMPARE("binary:",s1));
			elif(!COMPARE("version:",s1));
			else
			{
				free(hdr);
				return NULL;
			}
			break;
		case CLAUSE::RASTER:
			if (!COMPARE("pixels:", s1))
				(*hdr).pixels = atoi(s2);
			elif(!COMPARE("scanlines:", s1))
				(*hdr).scanlines = atoi(s2);
			elif(!COMPARE("scanDir:",s1))
			{
				if (!COMPARE("LR",s2))
					assert(!(*hdr).reversed);
				elif(!COMPARE("RL",s2))
					assert((*hdr).reversed);
				else
				{
					free(hdr);
					return NULL;
				}
				if (!COMPARE("TB",s2 + 2));
				else
				{
					free(hdr);
					return NULL;
				}
			}
			else
			{
				free(hdr);
				return NULL;
			}
			break;
		case CLAUSE::PHOTOMETRY:
			if (!(COMPARE("name:",s1)))
			{
				if (COMPARE("REV_",s2))
					q = s2;
				else
				{
					q = s2 + 4;
					(*hdr).reversed = TRUE;
				}
				(*hdr).photometry = resolve_ph(q);
			}
			elif(!COMPARE("white:",s1))
				(*hdr).white = atoi(s2);
			else
			{
				free(hdr);
				return NULL;
			}
			break;
		case CLAUSE::ORIGIN:
			if (!COMPARE("units:",s1))
			{
				if (!COMPARE("centimeters",s2))
					(*hdr).units = CENTIMETERS;
				elif(!COMPARE("inch",s2))
					(*hdr).units = INCH;
				elif(!COMPARE("unknown",s2))
					(*hdr).units = UNKNOWN;
				else
				{
					free(hdr);
					return NULL;
				}
			}
			elif(!COMPARE("resolution:",s1))
			{
				x = sscanf(s2, "%lf", &((*hdr).resolution));
				assert(1 == x);
			}
			elif(!COMPARE("aspect:",s1))
			{
				x = sscanf(s2, "%lf", &((*hdr).aspect));
				assert(1 == x);
			}
			else
			{
				free(hdr);
				return NULL;
			}
			break;
		default:
			free(hdr);
			return NULL;
		} //end switch
	} // end FOREEVER

	//   assert( (*hdr).bytes_per_sl == (((*hdr).pixels+1)*(*hdr).bits_per_sample-1)/(*hdr).bits_per_sample );

	assert((*hdr).bits_per_sample == ((*hdr).photometry == kLinear) ? 1 : 8);

	switch (hdr->photometry)
	{
	case kLinear:
	case grayLinear:
		assert(1 == (*hdr).imageparts);
		break;
	case rgbLinear:
	case faxlab:
	case cielab:
	case yccLinear:
		assert(3 == (*hdr).imageparts);
		break;
	case cmykLinear:
		assert(4 == (*hdr).imageparts);
		break;
	default:
		free(hdr);
		return NULL;
	}
	//    return hdr;
	return 1;
}

PHOTOMETRY CInt::resolve_ph(char* p)
{
	PHOTOMETRY photometry;

	/*---------------------------------------------------------------
	| E.g., simplify "REV_kLinear" to "kLinear".
	+--------------------------------------------------------------*/
	if (!COMPARE("REV_", p))  p += 4;

	if (!COMPARE("kLinear", p))   photometry = kLinear;
	elif(!COMPARE("cmykLinear", p)) photometry = cmykLinear;
	elif(!COMPARE("grayLinear", p)) photometry = grayLinear;
	elif(!COMPARE("rgbLinear", p))  photometry = rgbLinear;
	elif(!COMPARE("faxlab", p))     photometry = faxlab;
	elif(!COMPARE("cielab", p))     photometry = cielab;
	elif(!COMPARE("yccLinear", p))  photometry = yccLinear;
	elif(!COMPARE("YCbCr", p))      photometry = yccLinear;
	else { return unknownint; }
	return photometry;
}

int CInt::TiffHeader2IntHeader()
{
	//Tiff Header 2 Int Header	
	hdr.pixels = m_Tiff.GetTagValue(ImageWidth);
	hdr.scanlines = m_Tiff.GetTagValue(ImageLength);
	hdr.bits_per_sample = m_Tiff.GetTagValue(BitsPerSample);
	hdr.bytes_per_sl = hdr.pixels * hdr.bits_per_sample / 8;
	hdr.imageparts = m_Tiff.GetTagValue(SamplesPerPixel);
	hdr.resolution = m_Tiff.GetTagValue(XResolution);
	hdr.reversed = 0;

	switch (m_Tiff.GetTagValue(ResolutionUnit))
	{
	case 1:hdr.units = UNKNOWN; break;
	case 3:hdr.units = CENTIMETERS; break;
	case 2:
	default:hdr.units = INCH; break;
	}

	hdr.white = 0;
	switch (m_Tiff.GetTagValue(PhotometricInterpretation))
	{
	case 0:hdr.photometry = kLinear; break;
	case 2:hdr.photometry = rgbLinear; break;
	case 5:hdr.photometry = cmykLinear; break;
	case 8:hdr.photometry = cielab; break;
	default:hdr.photometry = rgbLinear; break;
	}

	return 1;
}

int CInt::IntHeader2TiffHeader()
{
	m_Tiff.SetTag(ImageWidth, Short, 1, hdr.pixels);
	m_Tiff.SetTag(ImageLength, Short, 1, hdr.scanlines);
	m_Tiff.SetTag(BitsPerSample, Short, 1, hdr.bits_per_sample);
	m_Tiff.SetTag(SamplesPerPixel, Short, 1, hdr.imageparts);
	m_Tiff.SetTag(XResolution, Short, 1, (DWORD)hdr.resolution);
	m_Tiff.SetTag(YResolution, Short, 1, (DWORD)hdr.resolution);

	switch (hdr.units)
	{
	case UNKNOWN:m_Tiff.SetTag(ResolutionUnit, Short, 1, 1); break;
	case CENTIMETERS:m_Tiff.SetTag(ResolutionUnit, Short, 1, 3); break;
	case INCH:
	default:m_Tiff.SetTag(ResolutionUnit, Short, 1, 2); break;
	}

	switch (hdr.photometry)
	{
	case kLinear:		m_Tiff.SetTag(PhotometricInterpretation, Short, 1, 0); break;
	case rgbLinear:		m_Tiff.SetTag(PhotometricInterpretation, Short, 1, 2); break;
	case cmykLinear:	m_Tiff.SetTag(PhotometricInterpretation, Short, 1, 5); break;
	case cielab:		m_Tiff.SetTag(PhotometricInterpretation, Short, 1, 8); break;
	default:			m_Tiff.SetTag(PhotometricInterpretation, Short, 1, 2); break;
	}
	return 1;
}

int CInt::ReadIntImage(FILE* file, int Width, int Length, int samplePerPixel, int bitsPerSample)
{
	LPBYTE lpTempR, lpTempG, lpTempB, lpTempK;
	int ReadSize = Width * bitsPerSample / 8;
	LPBYTE lpTemp = m_Tiff.GetImageBuf();
	if (samplePerPixel == 3)
	{//3 channel
		LPBYTE lpImageBufR = new BYTE[ReadSize];
		LPBYTE lpImageBufG = new BYTE[ReadSize];
		LPBYTE lpImageBufB = new BYTE[ReadSize];

		for (int i = 0; i < Length; i++)
		{
			fread(lpImageBufR, sizeof(char), ReadSize, file);
			fread(lpImageBufG, sizeof(char), ReadSize, file);
			fread(lpImageBufB, sizeof(char), ReadSize, file);
			lpTempR = lpImageBufR;
			lpTempG = lpImageBufG;
			lpTempB = lpImageBufB;
			if (hdr.photometry == cielab)
			{
				for (int k = 0; k < Width; k++)
				{
					*(lpTemp++) = *(lpTempR++);
					*(lpTemp++) = *(lpTempG++) ^ 0x80;
					*(lpTemp++) = *(lpTempB++) ^ 0x80;
				}
			}
			else if (hdr.photometry == yccLinear)
			{
				//YCC2RGB
				//1.0000         0    1.4020
				//1.0000   -0.3441   -0.7141
				//1.0000    1.7720         0
				int y, cb, cr;
				int R, G, B;
				for (int k = 0; k < Width; k++)
				{//Need change
					y = *(lpTempR++);
					cb = *(lpTempG++) - 128;
					cr = *(lpTempB++) - 128;

					R = (y * 1024 + cb * 0 + cr * 1435) >> 10;
					G = (y * 1024 + cb * -352 + cr * -731) >> 10;
					B = (y * 1024 + cb * 1814 + cr * 0) >> 10;

					if (R > 255) R = 255;
					if (R < 0)	R = 0;
					if (G > 255) G = 255;
					if (G < 0)	G = 0;
					if (B > 255) B = 255;
					if (B < 0)	B = 0;

					*(lpTemp++) = R;
					*(lpTemp++) = G;
					*(lpTemp++) = B;
				}
			}
			else
			{
				for (int k = 0; k < Width; k++)
				{
					*(lpTemp++) = *(lpTempR++);
					*(lpTemp++) = *(lpTempG++);
					*(lpTemp++) = *(lpTempB++);
				}
			}
		}
		delete[]lpImageBufR;
		delete[]lpImageBufG;
		delete[]lpImageBufB;
	}
	else if (samplePerPixel == 4)
	{//4 channel
		LPBYTE lpImageBufR = new BYTE[ReadSize];
		LPBYTE lpImageBufG = new BYTE[ReadSize];
		LPBYTE lpImageBufB = new BYTE[ReadSize];
		LPBYTE lpImageBufK = new BYTE[ReadSize];

		for (int i = 0; i < Length; i++)
		{
			fread(lpImageBufR, sizeof(char), ReadSize, file);
			fread(lpImageBufG, sizeof(char), ReadSize, file);
			fread(lpImageBufB, sizeof(char), ReadSize, file);
			fread(lpImageBufK, sizeof(char), ReadSize, file);
			lpTempR = lpImageBufR;
			lpTempG = lpImageBufG;
			lpTempB = lpImageBufB;
			lpTempK = lpImageBufK;
			for (int k = 0; k < Width; k++)
			{
				*(lpTemp++) = *(lpTempR++);
				*(lpTemp++) = *(lpTempG++);
				*(lpTemp++) = *(lpTempB++);
				*(lpTemp++) = *(lpTempK++);
			}
		}
		delete[]lpImageBufR;
		delete[]lpImageBufG;
		delete[]lpImageBufB;
		delete[]lpImageBufK;
	}
	else
	{//1 channel
		for (int i = 0; i < Length; i++)
		{
			fread(lpTemp, sizeof(char), ReadSize, file);
			lpTemp += ReadSize;
		}
	}

	return 1;
}

int CInt::put_int_hdr(FILE* fp, INT_HDR* int_hdr, char* ph_str)
{
	int    i;
	char* p, buf[512];

	p = buf;
	p += sprintf(p, "[ID]\n");
	p += sprintf(p, "format: internal\n");
	p += sprintf(p, "binary: %c%c%c%c\n", 0, 0, 0, 0);
	p += sprintf(p, "version: 2.0\n");
	p += sprintf(p, "\n");
	p += sprintf(p, "[Raster]\n");
	p += sprintf(p, "pixels: %d\n", int_hdr->pixels);
	p += sprintf(p, "scanlines: %d\n", int_hdr->scanlines);
	p += sprintf(p, "\n");
	p += sprintf(p, "[Encoding]\n");
	p += sprintf(p, "interleave: scanline\n");
	p += sprintf(p, "imageParts: %d\n", int_hdr->imageparts);

	p += sprintf(p, "bits/sample:");
	for (i = 0; i < int_hdr->imageparts; i++)
		p += sprintf(p, " %d", int_hdr->bits_per_sample);
	p += sprintf(p, "\n");

	p += sprintf(p, "bytes/sl: %d\n", int_hdr->bytes_per_sl);
	p += sprintf(p, "\n");
	p += sprintf(p, "[Photometry]\n");
	p += sprintf(p, "name: ");
	switch (int_hdr->photometry)
	{
	case kLinear:
		p += sprintf(p, "kLinear");
		break;
	case grayLinear:
		p += sprintf(p, "grayLinear");
		break;
	case rgbLinear:
		p += sprintf(p, "rgbLinear");
		break;
	case cmykLinear:
		p += sprintf(p, "cmykLinear");
		break;
	case faxlab:
		p += sprintf(p, "faxlab");
		break;
	case yccLinear:
		p += sprintf(p, "yccLinear");
		break;
	case cielab:
		p += sprintf(p, "cielab");
		break;
	default:
		break;
	}

	p += sprintf(p, "\n");
	p += sprintf(p, "\n");
	p += sprintf(p, "[End]\x0c\n");
	fwrite(buf, 1, p - buf, fp);
	return 1;
}

int CInt::WriteIntImage(FILE* file)
{
	LPBYTE lpTemp, lpTempR, lpTempG, lpTempB, lpTempK;
	LPBYTE lpImageBuf = m_Tiff.GetImageBuf();
	int Width = hdr.pixels;
	int Length = hdr.scanlines;

	if (hdr.imageparts == 3)
	{
		LPBYTE lpImageBufR = new BYTE[Width];
		LPBYTE lpImageBufG = new BYTE[Width];
		LPBYTE lpImageBufB = new BYTE[Width];
		lpTemp = lpImageBuf;

		for (int i = 0; i < Length; i++)
		{
			lpTempR = lpImageBufR;
			lpTempG = lpImageBufG;
			lpTempB = lpImageBufB;
			if (hdr.photometry == cielab)
			{
				for (int k = 0; k < Width; k++)
				{
					*(lpTempR++) = *(lpTemp++);
					*(lpTempG++) = *(lpTemp++) ^ 0x80;
					*(lpTempB++) = *(lpTemp++) ^ 0x80;
				}
			}
			else
			{
				for (int k = 0; k < Width; k++)
				{
					*(lpTempR++) = *(lpTemp++);
					*(lpTempG++) = *(lpTemp++);
					*(lpTempB++) = *(lpTemp++);
				}
			}
			fwrite(lpImageBufR, sizeof(char), Width, file);
			fwrite(lpImageBufG, sizeof(char), Width, file);
			fwrite(lpImageBufB, sizeof(char), Width, file);
		}
		delete[]lpImageBufR;
		delete[]lpImageBufG;
		delete[]lpImageBufB;
	}
	else
	{
		if (hdr.bits_per_sample == 8)
		{
			LPBYTE lpImageBufR = new BYTE[Width];
			LPBYTE lpImageBufG = new BYTE[Width];
			LPBYTE lpImageBufB = new BYTE[Width];
			LPBYTE lpImageBufK = new BYTE[Width];
			lpTemp = lpImageBuf;
			for (int i = 0; i < Length; i++)
			{
				lpTempR = lpImageBufR;
				lpTempG = lpImageBufG;
				lpTempB = lpImageBufB;
				lpTempK = lpImageBufK;
				for (int k = 0; k < Width; k++)
				{
					*(lpTempR++) = *(lpTemp++);
					*(lpTempG++) = *(lpTemp++);
					*(lpTempB++) = *(lpTemp++);
					*(lpTempK++) = *(lpTemp++);
				}
				fwrite(lpImageBufR, sizeof(char), Width, file);
				fwrite(lpImageBufG, sizeof(char), Width, file);
				fwrite(lpImageBufB, sizeof(char), Width, file);
				fwrite(lpImageBufK, sizeof(char), Width, file);
			}
			delete[]lpImageBufR;
			delete[]lpImageBufG;
			delete[]lpImageBufB;
			delete[]lpImageBufK;
		}
		else
		{//CMYK 1bits, has been packed
			int BufSize = Width * Length * 4 / 8;
			fwrite(lpImageBuf, sizeof(char), BufSize, file);
		}
	}
	return 1;
}

int CInt::Read_PGM(LPCTSTR Inupt, LPCTSTR Output)
{
	ifstream fin(Inupt);
	string tempstr;
	getline(fin, tempstr, (char)0x0A);//P8
	getline(fin, tempstr, (char)0x0A);//#
	getline(fin, tempstr, (char)0x20);//#
	int Width = atoi(tempstr.c_str());

	getline(fin, tempstr, (char)0x0A);
	int Length = atoi(tempstr.c_str());

	getline(fin, tempstr, (char)0x0A);
	// 		cout << tempstr;

	int BufSize = Width * Length * 4;
	char* lpBuf = new char[BufSize];
	fin.read(lpBuf, BufSize);
	fin.close();

	m_Tiff.CreateNew(Width, Length, 600, 4, 8);
	TiffHeader2IntHeader();
	memcpy(m_Tiff.GetImageBuf(), lpBuf, BufSize);

	delete[]lpBuf;
	SaveFile(Output);
	return 1;
}




int CInt::PGM2Tiff(LPCTSTR Inupt, LPCTSTR Output)
{
	ifstream fin(Inupt);
	string tempstr;
	getline(fin, tempstr, (char)0x0A);//P8
	getline(fin, tempstr, (char)0x0A);//#
	getline(fin, tempstr, (char)0x20);//#
	int Width = atoi(tempstr.c_str());

	getline(fin, tempstr, (char)0x0A);
	int Length = atoi(tempstr.c_str());

	getline(fin, tempstr, (char)0x0A);
	// 		cout << tempstr;

	int BufSize = Width * Length * 4;
	char* lpBuf = new char[BufSize];
	fin.read(lpBuf, BufSize);
	fin.close();

	m_Tiff.CreateNew(Width, Length, 600, 4, 8);
	memcpy(m_Tiff.GetImageBuf(), lpBuf, BufSize);
	m_Tiff.SaveFile(Output);
	delete[]lpBuf;

	return 1;
}

