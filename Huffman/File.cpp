/*
		Name: File.cpp
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#include "stdafx.h"
#include "File.h"


const GUID CSimpleFile::m_guid = // {B1039B3F-6A8D-4177-AF2A-15BD3B888368}
{ 0xb1039b3f, 0x6a8d, 0x4177, { 0xaf, 0x2a, 0x15, 0xbd, 0x3b, 0x88, 0x83, 0x68 } };

const GUID CHuffmanFile::m_guid = // {6EDF3A54-A47C-40C6-BF9C-C108D320DA8E}
{ 0x6edf3a54, 0xa47c, 0x40c6, { 0xbf, 0x9c, 0xc1, 0x8, 0xd3, 0x20, 0xda, 0x8e } };

const GUID CCombinedHufFile::m_guid = // {B448C1B5-7253-44CB-9343-4B9027EA7B85}
{ 0xb448c1b5, 0x7253, 0x44cb, { 0x93, 0x43, 0x4b, 0x90, 0x27, 0xea, 0x7b, 0x85 } };

CFileBase::CFileBase() : m_pByteArray(NULL), m_dwCount(0), m_dwError(0)
{
	m_szOpenedFilePath[0] = '\0';
}
CFileBase::CFileBase(CFileBase&)
{
	assert(0);
}
CFileBase::~CFileBase()
{
	Release();
}
bool CFileBase::ReadFile(const wchar_t* szPath)
{
	Release();
	FILE *file;
	if(_wfopen_s(&file, szPath, L"rb"))
		return false;
	bool result = ReadFile(file);
	fclose(file);
	if(!result)
	{
		Release();
		return false;
	}
	wcscpy_s(m_szOpenedFilePath, szPath);
	return true;
}
bool CFileBase::ReadFile(FILE* pFile)
{
	Release();
	if(!pFile)
		return false;
	fseek(pFile, 0, SEEK_END);
	m_dwCount = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	if(!m_dwCount)
		return false;
	m_pByteArray = new BYTE[m_dwCount];
	if( !fread_s(m_pByteArray, sizeof(BYTE)*m_dwCount,
			sizeof(BYTE), m_dwCount, pFile) )
		return false;
	return true;
}
bool CFileBase::WriteFile(const wchar_t* szPath) const
{
	FILE *file;
	if(_wfopen_s(&file, szPath, L"wb"))
		return false;
	bool result = WriteFile(file);
	fclose(file);
	if( result )
		wcscpy_s(m_szOpenedFilePath, szPath);
	return result;
}
bool CFileBase::WriteFile(FILE* pFile) const
{
	if(!m_dwCount)
		return false;
	return !!fwrite(m_pByteArray, sizeof(BYTE), m_dwCount, pFile);
}
DWORD CFileBase::GetBytes() const
{
	return m_dwCount;
}
const BYTE* CFileBase::GetData() const
{
	return m_pByteArray;
}
DWORD CFileBase::GetError() const
{
	return m_dwError;
}
const wchar_t* CFileBase::GetFilePath() const
{
	return m_szOpenedFilePath;
}
void CFileBase::Release()
{
	SAFE_DELETE_ARRAY(m_pByteArray);
	m_dwCount = 0;
	m_szOpenedFilePath[0] = '\0';
}
CFileBase& CFileBase::operator = (const CFileBase &file)
{
	Release();
	m_dwCount = file.m_dwCount;
	if(m_dwCount)
	{
		m_pByteArray = new BYTE[m_dwCount];
		if(!m_pByteArray)
			throw(0);
		memcpy(m_pByteArray, file.m_pByteArray, m_dwCount);
	}
	return *this;
}


CHuffmanFile::CHuffmanFile() : m_pHufCode(NULL)
{
	m_szFileName[0] = '\0';
}
CHuffmanFile::~CHuffmanFile()
{
	Release();
}
bool CHuffmanFile::ReadFile(FILE* pFile)
{
	if(!pFile)
		return false;
	m_dwError = 0;
	Release();
	SHuffman_File_Head head;
	fread(&head, sizeof(head), 1, pFile);
	fseek(pFile,-(int)sizeof(head),SEEK_CUR);
	if(strcmp(head.szMagicText, HUFFMAN_FILE_HEAD) ||
		!head.dwSourceFileSize || !head.dwCodeKeyCount ||
		!head.dwCodeSize)
	{
		m_dwError = HUFFMAN_ERROR_BADHEAD;
		Release();
		return false;
	}
	m_dwCount = head.dwCodeSize;
	if(!m_dwCount)
		return false;
	m_pByteArray = new BYTE[m_dwCount];
	if( !fread_s(m_pByteArray, sizeof(BYTE)*m_dwCount,
			sizeof(BYTE), m_dwCount, pFile) )
		return false;
	if(wcscpy_s(m_szFileName, (wchar_t *)(m_pByteArray + sizeof(head))))
	{
		Release();
		return false;
	}
	return true;
}
bool CHuffmanFile::Encode(const CFile &file)
{
	m_dwError = 0;
	if(!file.m_pByteArray)
	{
		m_dwError = HUFFMAN_ERROR_NOFILE;
		return false;
	}
	Release();

	DWORD count = 0;
	DWORD count1 = 0;
	DWORD count2 = 0;
	DWORD dwCodeCounts[256];
	// 统计每个char编码的出现次数
	memset(&dwCodeCounts, 0, sizeof(dwCodeCounts));
	for(DWORD i = 0; i < file.m_dwCount; ++i)
		++dwCodeCounts[file.m_pByteArray[i]];
	CHuffmanTree huf[256];

	// Head of Huffman File.
	SHuffman_File_Head hfHead;
	// Size of the head.
	count += sizeof(hfHead);

	for(size_t i = 0; i <256; i++)
	{
		huf[i].SetValue((char) i);
		huf[i].SetWeight(dwCodeCounts[i]);
	}
	SAFE_DELETE(m_pHufCode);
	m_pHufCode = new CHuffmanCode(huf, 256);
	
	
	CHuffmanCode::Code *pCode = m_pHufCode->GetCodeArray();
	size_t codeKeyCount = m_pHufCode->GetCodeArrayCount();
	for(size_t i = 0; i <codeKeyCount; i++)
	{
		// Size of Huffman code 霍夫曼编码所占字节
		count += BIT_COUNT_TO_BYTE_COUNT(pCode[i].count) + sizeof(char) + sizeof(size_t);
		// Bits of file after it compressed.
		count1 +=  huf[(BYTE)pCode[i].ANSI].GetWeight() * pCode[i].count ;
	}
	// 文件压缩后所占的字节
	count1 = count1 % 8 ?
			count1 / 8 + 1 :
			count1 / 8;
	wcscpy_s(m_szFileName, ::GetFileName(file.m_szOpenedFilePath));
	count2 = wcslen(m_szFileName) * sizeof(wchar_t) + sizeof(wchar_t);
	// 得到霍夫曼压缩文件字节数
	hfHead.dwCodeSize = m_dwCount = count + count1 + count2;
	// 分配内存空间
	m_pByteArray = new BYTE[m_dwCount];
	// 清零，避免BitToByte中或位运算出错
	memset(m_pByteArray, 0, m_dwCount * sizeof(BYTE));
	BYTE *pCurrent = m_pByteArray;
	// 配置文件头
	hfHead.dwCodeKeyCount = codeKeyCount;
	hfHead.dwSourceFileSize = file.m_dwCount;
	// 向内存中写入文件头
	memcpy(pCurrent, &hfHead, sizeof(hfHead));
	pCurrent += sizeof(hfHead);
	// 写入文件名
	wcscpy_s((wchar_t*)pCurrent, count2, m_szFileName);
	pCurrent += count2;
	// 向内存中写入霍夫曼编码
	for(size_t i = 0; i <codeKeyCount; i++)
	{
		memcpy(pCurrent, &pCode[i].ANSI, sizeof(char));
		pCurrent += sizeof(char);
		memcpy(pCurrent, &pCode[i].count, sizeof(size_t));
		pCurrent += sizeof(size_t);
		DWORD tmpByteSize = BIT_COUNT_TO_BYTE_COUNT(pCode[i].count);
		BYTE *tmpByte = new BYTE[tmpByteSize];
		// 清零，避免BitToByte中或位运算出错
		memset(tmpByte, 0, sizeof(BYTE) * tmpByteSize);
		BYTE *tmpByteCurrent = tmpByte;
		m_cPush = 0;
		for(size_t j = 0; j <pCode[i].count; j++)
			BitToByte(tmpByteCurrent, pCode[i].pBinaryCode[j]);
		memcpy(pCurrent, tmpByte, tmpByteSize);
		pCurrent += tmpByteSize;
		SAFE_DELETE_ARRAY(tmpByte);
	}
	// 写入压缩内容
	m_cPush = 0;
	*pCurrent = 0;
	for(size_t i = 0; i <file.m_dwCount; i++)
	{
		size_t len; bool *pc;
		m_pHufCode->GetHuffmanCode(file.m_pByteArray[i], pc, len);
		for(size_t j = 0; j <len; j++)
			BitToByte(pCurrent, pc[j]);
	}
	return true;
}
bool CHuffmanFile::Decode(CFile& File) const
{
	m_dwError = 0;
	BYTE *pReadCurrent = m_pByteArray;
	DWORD dwHFSize = m_dwCount;
	if(!pReadCurrent || !dwHFSize || m_szFileName[0] == '\0')
	{
		m_dwError = HUFFMAN_ERROR_BADHUFFILE;
		return false;
	}
	// 读入HuffmanFile文件头
	SHuffman_File_Head hfHead;
	memcpy(&hfHead, pReadCurrent, sizeof(hfHead));
	pReadCurrent += sizeof(hfHead);
	dwHFSize -= sizeof(hfHead);
	if(strcmp(hfHead.szMagicText, HUFFMAN_FILE_HEAD) ||
		!hfHead.dwSourceFileSize || !hfHead.dwCodeKeyCount)
	{
		m_dwError = HUFFMAN_ERROR_BADHEAD;
		return false;
	}
	// 跳过文件名
	pReadCurrent += wcslen(m_szFileName) * sizeof(wchar_t) + sizeof(wchar_t);
	// 读入并创建霍夫曼编码树
	CHuffmanTree HufRoot;
	for(DWORD i = 0; i <hfHead.dwCodeKeyCount; ++i)
	{
		char ansi = *pReadCurrent;
		pReadCurrent++;
		size_t len;
		memcpy(&len, pReadCurrent, sizeof(size_t));
		pReadCurrent += sizeof(size_t);
		bool *pCodeKeys = new bool[len];
		DWORD tmpByteSize = BIT_COUNT_TO_BYTE_COUNT(len);
		BYTE *tmpByte = new BYTE[tmpByteSize];
		memcpy(tmpByte, pReadCurrent, tmpByteSize);
		pReadCurrent += tmpByteSize;
		BYTE *tmpByteCurrent = tmpByte;
		m_cPop = 0;
		for(size_t j = 0; j <len; j++)
			ByteToBit(tmpByteCurrent, pCodeKeys[j]);
		HufRoot.CreateHuffmanFromCode(ansi, pCodeKeys, len);
		SAFE_DELETE_ARRAY(tmpByte);
		SAFE_DELETE_ARRAY(pCodeKeys);
	}
	File.Release();
	// 为写入的文件开辟内存空间
	File.m_dwCount = hfHead.dwSourceFileSize;
	File.m_pByteArray = new BYTE[File.m_dwCount];
	// 解码并把内容写入内存中
	m_cPop = 0;
	char ansi = '\0';
	for(DWORD i = 0; i <File.m_dwCount;)
	{
		bool bBinary = false;
		ByteToBit(pReadCurrent, bBinary);
		if(HufRoot.SearchAnsiFromNextCode(ansi, bBinary))
		{
			File.m_pByteArray[i] = (BYTE)ansi;
			i++;
		}
	}
	return true;
}
GUID CHuffmanFile::GetGUID() const
{
	return m_guid;
}
const wchar_t* CHuffmanFile::GetFileName() const
{
	return m_szFileName;
}
void CHuffmanFile::Release()
{
	SAFE_DELETE(m_pHufCode);
	CFileBase::Release();
}
CFileBase& CHuffmanFile::operator =(const CFileBase &file)
{
	if(file.GetGUID() == m_guid)
		CFileBase::operator=(file);
	else
		if( !Encode((CFile&)file) )
			throw(0);
	return *this;
}
bool CHuffmanFile::TestFile(wchar_t *szFilePath)
{
	FILE *pFile;
	if(_wfopen_s(&pFile, szFilePath, L"rb"))
		return false;
	if(!pFile)
		return false;
	SHuffman_File_Head Head;
	fread_s(&Head, sizeof(Head), sizeof(Head), 1, pFile);
	if(strcmp(Head.szMagicText, HUFFMAN_FILE_HEAD)
		|| !Head.dwSourceFileSize || !Head.dwCodeKeyCount)
	{
		fclose(pFile);
		return false;
	}
	fclose(pFile);
	return true;
}
void CHuffmanFile::BitToByte(BYTE* &pWriteCurrent, bool bBinary) const
{
	if(m_cPush>= 8)
	{
		pWriteCurrent++;
		*pWriteCurrent = 0;
		m_cPush = 0;
	}
	char b = !!bBinary;
	*pWriteCurrent |= b<<(7 - m_cPush);
	m_cPush++;
}
void CHuffmanFile::ByteToBit(BYTE* &pReadCurrent, bool &bBinary) const
{
	if(m_cPop>= 8)
	{
		pReadCurrent++;
		m_cPop = 0;
	}
	bBinary = ( *pReadCurrent>>(7 - m_cPop) ) & 0x01;
	m_cPop++;
}




CFileBase& CFile::operator=(const CFileBase& file)
{
	GUID guid = file.GetGUID();
	if(guid == CHuffmanFile::m_guid)
	{
		if(!((CHuffmanFile&)file).Decode(*this))
			throw(0);
	}
	else
		CFileBase::operator=(file);
	return *this;
}




GUID CSimpleFile::GetGUID() const
{
	return m_guid;
}
CFileBase& CSimpleFile::operator=(const CFileBase& file)
{
	return CFile::operator=(file);
}






CCombinedHufFile::~CCombinedHufFile()
{
	Release();
}
bool CCombinedHufFile::ReadFile(const wchar_t *szPath)
{
	return CFile::ReadFile(szPath);
}
bool CCombinedHufFile::ReadFile(FILE* pFile)
{
	Release();
	SCombinHuf_File_Head head;
	fread(&head, sizeof(head), 1, pFile);
	if( strcmp(head.szMagicText, COMBINEHUF_FILE_HEAD) )
		return false;
	for(DWORD i = 0; i < head.dwFileCount; ++i)
	{
		CHuffmanFile *pHuf = new CHuffmanFile;
		pHuf->ReadFile(pFile);
		try
		{
			m_vHufFiles.push_back(pHuf);
		}
		catch(...)
		{
			return false;
		}
	}
	CFileBase::Release();
	return true;
}
bool CCombinedHufFile::WriteFile(const wchar_t *szPath) const
{
	return CFile::WriteFile(szPath);
}
bool CCombinedHufFile::WriteFile(FILE* pFile) const
{
	SCombinHuf_File_Head head;
	head.dwFileCount = m_vHufFiles.size();
	strcpy_s(head.szMagicText, COMBINEHUF_FILE_HEAD);
	if(!fwrite(&head, sizeof(head), 1, pFile))
		return false;
	for(DWORD i = 0; i < head.dwFileCount; ++i)
		if( !m_vHufFiles[i]->WriteFile(pFile) )
			return false;
	return true;
}
bool CCombinedHufFile::UpdateFile()
{
	return WriteFile(m_szOpenedFilePath);
}
GUID CCombinedHufFile::GetGUID() const
{
	return m_guid;
}
DWORD CCombinedHufFile::GetFileIndex(const wchar_t *szFileName) const
{
	DWORD dwCount = m_vHufFiles.size();
	if(!dwCount)
		return -1;
	for(DWORD i = 0; i < dwCount; ++i)
		if(m_vHufFiles[i])
			if(!wcscmp(m_vHufFiles[i]->GetFileName(), szFileName))
				return i;
	return -1;
}
const wchar_t* CCombinedHufFile::GetFileName(DWORD dwIndex) const
{
	if(dwIndex >= m_vHufFiles.size())
		return nullptr;
	return m_vHufFiles[dwIndex]->GetFileName();
}
const CHuffmanFile* CCombinedHufFile::GetFile(DWORD dwIndex) const
{
	if(dwIndex >= m_vHufFiles.size())
		return nullptr;
	return m_vHufFiles[dwIndex];
}
const CHuffmanFile* CCombinedHufFile::GetFile(const wchar_t *szFileName) const
{
	return GetFile(GetFileIndex(szFileName));
}
DWORD CCombinedHufFile::GetFileCount() const
{
	return m_vHufFiles.size();
}
bool CCombinedHufFile::DecodeFile(DWORD dwIndex, CSimpleFile &file) const
{
	try
	{
		file.Release();
		file = *m_vHufFiles[dwIndex];
	}
	catch(...)
	{
		return false;
	}
	return true;
}
DWORD CCombinedHufFile::AddFile(const CFileBase& file)
{
	CHuffmanFile *pHuf;
	try
	{
		if(GetFileIndex(::GetFileName(file.GetFilePath())) != -1)
			return false;
		pHuf = new CHuffmanFile;
		if( !pHuf )
			return false;
		*pHuf = file;
		m_vHufFiles.push_back(pHuf);
	}
	catch(...)
	{
		delete pHuf;
		return false;
	}
	return true;
}
bool CCombinedHufFile::DeleteFile(DWORD dwIndex)
{
	try
	{
		if( dwIndex > m_vHufFiles.size() )
			return false;
		delete m_vHufFiles[dwIndex];
		m_vHufFiles.erase(m_vHufFiles.begin() + dwIndex);
	}
	catch(...)
	{
		return false;
	}
	return true;
}
void CCombinedHufFile::Release()
{
	for(auto i = m_vHufFiles.begin(); i != m_vHufFiles.end(); ++i)
		delete *i;
	m_vHufFiles.clear();
	CFile::Release();
}
bool CCombinedHufFile::TestFile(wchar_t *szFilePath)
{
	FILE *pFile;
	if(_wfopen_s(&pFile, szFilePath, L"rb"))
		return false;
	if(!pFile)
		return false;
	SCombinHuf_File_Head Head;
	fread_s(&Head, sizeof(Head), sizeof(Head), 1, pFile);
	if( strcmp(Head.szMagicText, COMBINEHUF_FILE_HEAD) )
	{
		fclose(pFile);
		return false;
	}
	fclose(pFile);
	return true;
}
