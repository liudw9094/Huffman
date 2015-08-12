/*
		Name: Safe.h
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#pragma once

#include "SafeThread.h"
template<typename T> inline void SAFE_DELETE(T* &p)
{
	if(p)
	{
		delete p;
		p = NULL;
	} 
}
template<typename T> inline void SAFE_DELETE_ARRAY(T* &p)
{
	if(p)
	{
		delete [] p;
		p = NULL;
	} 
}