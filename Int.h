// Int.h: interface for the CInt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INT_H__EE7D3C70_42A9_4CEC_9BFC_003FAE4A4611__INCLUDED_)
#define AFX_INT_H__EE7D3C70_42A9_4CEC_9BFC_003FAE4A4611__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <Tiff_STL3\Src\Tiff_STL3.h>
#include <Straw\Process\Process.h>
#include <assert.h>
//using namespace AV_Tiff_STL2;

#define FOREVER     for(;;)
#define until       while
#define COMPARE( x, y)  strncmp( x, y, strlen(x))
#define elif else if

enum class PHOTOMETRY{
	ekLinear	, 
	egrayLinear, 
	ergbLinear, 
	ecmykLinear, 
	efaxlab, 
	eyccLinear, 
	ecielab,
	eunknownint		= 0xffff
};

#define kLinear			PHOTOMETRY::ekLinear
#define grayLinear		PHOTOMETRY::egrayLinear
#define rgbLinear		PHOTOMETRY::ergbLinear
#define cmykLinear		PHOTOMETRY::ecmykLinear
#define faxlab			PHOTOMETRY::efaxlab
#define yccLinear		PHOTOMETRY::eyccLinear
#define cielab			PHOTOMETRY::ecielab
#define unknownint		PHOTOMETRY::eunknownint

enum UNITS{
	UNKNOWN, 
	CENTIMETERS, 
	INCH
};

struct _int_hdr {
	int     bits_per_sample;    /*bits per pixel per separation*/    
	int     bytes_per_sl;       /*bytes per scan line per separation*/    
	int     pixels;             /*pixels per scan line*/    
	int     scanlines;          /*scan lines per separation*/    
	int     imageparts;         /*number of input separations*/    
	int     white;              /*value of white, default 0*/    
	int     reversed;           /*scan reversal*/    
	PHOTOMETRY      photometry;
	UNITS   units;
#ifdef __GNUC__
	int		res1;
	int		res2;
	int		aspect1;
	int		aspect2;
#else
	double  resolution,         /**/
		aspect;             /**/
#endif

}   ;


//typedef struct _int_hdr INT_HDR;
using  INT_HDR = struct _int_hdr;

class CInt:public CBase  //Xerox format
{
public:
	virtual int Initial();
	virtual int main();
	virtual char* Name(){return (char*)"CInt";};
	int ReadTiff(LPCTSTR FileName);
	int SaveTiff(LPCTSTR FileName);
	int ReadFile(LPCTSTR FileName);
	int SaveFile(LPCTSTR FileName);
	int Read_PGM(LPCTSTR Inupt, LPCTSTR Output);
	int PGM2Tiff(LPCTSTR Inupt, LPCTSTR Output);
	CInt();
	virtual ~CInt();
private:
	int ReadIntImage(FILE *file, int Width, int Length, int samplePerPixel, int bitsPerSample);
	int WriteIntImage(FILE *file);
	int get_int_hdr( FILE *fp, INT_HDR *hdr);
	PHOTOMETRY resolve_ph( char* p);
	int IntHeader2TiffHeader();
	int TiffHeader2IntHeader();
	int put_int_hdr( FILE *fp, INT_HDR* int_hdr, char* ph_str);

	char InputFileName[MAX_PATH];
	char OutputFileName[MAX_PATH];
	int m_Tiff2Int, m_Int2Tiff;
	CTiff	m_Tiff;
	INT_HDR hdr;
};

#endif // !defined(AFX_INT_H__EE7D3C70_42A9_4CEC_9BFC_003FAE4A4611__INCLUDED_)
