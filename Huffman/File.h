/*
		Name: File.h
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#pragma once
#include "Safe.h"
#include "Huffman.h"
#include "Util.h"

inline DWORD BIT_COUNT_TO_BYTE_COUNT(DWORD BitCount)
{
	if(BitCount % 8)
		return BitCount / 8 + 1;
	return BitCount / 8;
}

#define HUFFMAN_FILE_HEAD "HF"
#define COMBINEHUF_FILE_HEAD "CHF"

//Error
#define HUFFMAN_ERROR_BADHEAD -1 
#define HUFFMAN_ERROR_BADHUFFILE -2
#define HUFFMAN_ERROR_NOFILE -3
/*
	Huffman file structure:
		|
 		+- Head(struct SHuffman_File_Head)
 		+- FileName
 		+- Decode Keys
 		+- Codes
*/
struct SHuffman_File_Head
{
	char	szMagicText[5];
	DWORD	dwSourceFileSize;
	DWORD	dwCodeKeyCount;
	DWORD	dwCodeSize;
	SHuffman_File_Head()
	{
		strcpy_s(szMagicText, 5, HUFFMAN_FILE_HEAD);
		dwCodeSize = dwSourceFileSize = 0;
		dwCodeKeyCount = 0;
	}
};
/*
	Combine Huffman file structure
		|
		+- Head(struct SCombinHuf_File_Head)
		+- Huffman file offsets
		+- Huffman files
*/
struct SCombinHuf_File_Head
{
	char	szMagicText[5];
	DWORD	dwFileCount;
	SCombinHuf_File_Head()
	{
		strcpy_s(szMagicText, 5, COMBINEHUF_FILE_HEAD);
		dwFileCount = 0;
	}
};
class CFileBase abstract
{
protected:
	BYTE			*m_pByteArray;
	DWORD			m_dwCount;
	mutable DWORD	m_dwError;
	mutable wchar_t	m_szOpenedFilePath[MAX_PATH];
public:
	CFileBase();
	virtual ~CFileBase();
	virtual bool ReadFile(const wchar_t *szPath);
	virtual bool ReadFile(FILE *pFile);
	virtual bool WriteFile(const wchar_t *szPath) const;
	virtual bool WriteFile(FILE *pFile) const;
	virtual DWORD GetBytes() const;
	virtual const BYTE* GetData() const;
	virtual DWORD GetError() const;
	virtual const wchar_t* GetFilePath() const;
	virtual void Release();
	virtual GUID GetGUID() const = 0;
	virtual CFileBase& operator=(const CFileBase& file);
private:
	CFileBase(CFileBase&);
};

class CFile;

class CHuffmanFile : public CFileBase
{
	friend class CFile;
private:
	static const GUID	m_guid;
protected:
	CHuffmanCode		*m_pHufCode;
	mutable char		m_cPush;
	mutable char		m_cPop;
	wchar_t				m_szFileName[MAX_PATH];
public:
	CHuffmanFile();
	virtual ~CHuffmanFile();
public:
	virtual bool ReadFile(FILE *pFile);
	virtual bool Encode(const CFile &file);
	virtual bool Decode(CFile &file) const;
	virtual GUID GetGUID() const;
	virtual const wchar_t* GetFileName() const;
	virtual void Release();
public:
	virtual CFileBase& operator=(const CFileBase& file);
public:
	static bool	TestFile(wchar_t *szFilePath);
protected:
	void BitToByte(BYTE* &pWriteCurrent, bool bBinary) const;
	void ByteToBit(BYTE* &pReadCurrent, bool &bBinary) const;
};
class CFile abstract: public CFileBase
{
	friend class CHuffmanFile;
public:
	virtual CFileBase& operator = (const CFileBase& file);
};
class CSimpleFile : public CFile
{
private:
	static const GUID	m_guid;
public:
	virtual GUID GetGUID() const;
	virtual CFileBase& operator = (const CFileBase& file);
};

class CCombinedHufFile : public CFile
{
private:
	static const GUID			m_guid;
	std::vector<CHuffmanFile*>	m_vHufFiles;
public:
	~CCombinedHufFile();
public:
	virtual bool				ReadFile(const wchar_t *szPath);
	virtual bool				ReadFile(FILE* pFile);
	virtual bool				WriteFile(const wchar_t *szPath) const;
	virtual bool				WriteFile(FILE* pFile) const;
	virtual bool				UpdateFile();
	virtual GUID				GetGUID() const;
	virtual DWORD				GetFileIndex(const wchar_t *szFileName) const;
	virtual const wchar_t*		GetFileName(DWORD dwIndex) const;
	virtual const CHuffmanFile*	GetFile( DWORD dwIndex ) const;
	virtual const CHuffmanFile*	GetFile( const wchar_t *szFileName ) const;
	virtual	DWORD				GetFileCount() const;
	virtual bool				DecodeFile(DWORD dwIndex, CSimpleFile &file) const;
	virtual DWORD				AddFile( const CFileBase& file );
	virtual bool				DeleteFile(DWORD dwIndex);
	virtual void				Release();
public:
	static bool					TestFile(wchar_t *szFilePath);
protected:
};