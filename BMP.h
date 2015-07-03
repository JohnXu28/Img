#pragma once
#include <Tiff_STL3\Tiff_STL3.h>
#include <straw\Process\Process.h>

#include <WinGDI.h>
class BMP 
{
public:
	BMP(void);
	~BMP(void);
	int ReadTiff(char *Input);
	int SaveTiff(char *Output);
	int ReadFile(char *Inupt);
	int SaveFile(char *FileName);
	int SaveFile_ASCII(char *FileName);
	int Tiff2BMP(char *Input, char*Output);
	int Bin2BMP(char *InputFile, int Width, int Length, char *OutputFile);
	CTiff *m_lpTiff;	
private:
	void BGR2RGB(LPBYTE lpIn, LPBYTE lpOut, int Size);
	void RGB2BGR(LPBYTE lpIn, LPBYTE lpOut, int Size);
	string m_FileName;
	BITMAPFILEHEADER m_Header;
	BITMAPINFOHEADER m_Info;
};

#define BMP_TEST
#ifdef BMP_TEST

class BMP_Test:public CBase
{
public:
	BMP_Test();
	virtual ~BMP_Test();
	virtual int Initial();
	virtual int main();
	virtual char* Name(){return "BMP_Test";};
private:
};
#endif //BMP_TEST

