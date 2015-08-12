/*
		Name: SafeThread.h
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#pragma once
class CSection : public CRITICAL_SECTION
{
public:
	inline CSection()
	{
		::InitializeCriticalSection(this);
	}
	inline ~CSection()
	{
		::DeleteCriticalSection(this);
	}
};
class CSafeLock
{
private:
	CRITICAL_SECTION *m_pCs;
public:
	explicit inline CSafeLock(CRITICAL_SECTION &cs):m_pCs(&cs)
	{
		::EnterCriticalSection(m_pCs);
	}
	inline ~CSafeLock()
	{
		::LeaveCriticalSection(m_pCs);
	}
};