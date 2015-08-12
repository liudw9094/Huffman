/*
		Name: Util.cpp
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#include "stdafx.h"

const wchar_t* GetFileName(const wchar_t *szFilePath)
{
	wchar_t* ptr = wcsrchr((wchar_t*)szFilePath, '\\');
	wchar_t* ptr1 = wcsrchr((wchar_t*)szFilePath, '/');
	ptr = ptr> ptr1 ? ptr : ptr1;
	return ++ptr;
}
const wchar_t* GetFileTypeName(const wchar_t *szFilePath)
{
	wchar_t* ptr = wcsrchr((wchar_t*)szFilePath, '.');
	return ++ptr;
}