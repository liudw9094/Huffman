/*
		Name: Huffman.cpp
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#include "stdafx.h"
#include "Huffman.h"


CHuffmanTree::CHuffmanTree(char Value, DWORD dwWeight):
		m_cValue(Value), m_dwWeight(dwWeight)
{
	m_paChildren[0] = m_paChildren[1] = NULL;
	m_pCurrent = this;
}
CHuffmanTree::~CHuffmanTree()
{
	ReleaseChildren();
}
bool CHuffmanTree::SetChildPointer(LR lr, CHuffmanTree *pTree)
{
	int i = (int)lr;
	if(m_paChildren[i])
		return false;
	m_paChildren[i] = pTree;
	return true;
}
bool CHuffmanTree::AddChild(LR lr, char Value)
{
	int i = (int)lr;
	if(m_paChildren[i])
		return false;
	m_paChildren[i] = new CHuffmanTree(Value);
	return true;
}
void CHuffmanTree::SetValue(char Value)
{
	m_cValue = Value;
}
void CHuffmanTree::SetWeight(DWORD dwWeight)
{
	m_dwWeight = dwWeight;
}
char CHuffmanTree::GetValue() const
{
	return m_cValue;
}
DWORD CHuffmanTree::GetWeight() const
{
	return m_dwWeight;
}
void CHuffmanTree::ReleaseChild(LR lrChild)
{
	int i = (int)lrChild;
	SAFE_DELETE(m_paChildren[i]);
}
void CHuffmanTree::ReleaseChildren()
{
	for(int i = 0; i <2; i++)
		ReleaseChild((LR)i);
}
size_t	CHuffmanTree::GetDepth(size_t base)
{
	base++;
	size_t t1 = 0, t2 = 0;
	if(m_paChildren[0])
		t1 = m_paChildren[0]->GetDepth(base);
	if(m_paChildren[1])
		t2 = m_paChildren[1]->GetDepth(base);
	if(!m_paChildren[0] && !m_paChildren[1])
		return base;
	return t1> t2 ? t1 : t2;
}
bool CHuffmanTree::CreateHuffmanFromCode(char ANSI, bool *pCode, DWORD dwLength)
{
	m_dwWeight = 0;
	if(!dwLength)
	{
		ReleaseChildren();
		m_cValue = ANSI;
		return true;
	}
	char i = !!*pCode;
	if(!m_paChildren[i])
	{
		m_paChildren[i] = new CHuffmanTree;
		if(!m_paChildren[i])
			return false;
	}
	return m_paChildren[i]->CreateHuffmanFromCode(ANSI, ++pCode, --dwLength);
}
bool CHuffmanTree::SearchAnsiFromNextCode(char &ANSI, bool bBinary)
{

	if(!m_pCurrent)
		return false;
	
	m_pCurrent = m_pCurrent->m_paChildren[!!bBinary];
	if(!m_pCurrent)
	{
		SearchReset();
		return false;
	}
	if(!m_pCurrent->m_paChildren[0] &&
		!m_pCurrent->m_paChildren[1])
	{
		ANSI = m_pCurrent->m_cValue;
		SearchReset();
		return true;
	}
	
	return false;
}
void CHuffmanTree::SearchReset()
{
	m_pCurrent = this;
}
void CHuffmanTree::_ClearChildPointers()
{
	m_paChildren[0] = m_paChildren[1] = NULL;
}
void CHuffmanTree::_SetChildPointer(LR lr, CHuffmanTree *pTree)
{
	int i = (int)lr;
	m_paChildren[i] = pTree;
}
CHuffmanCode::CHuffmanCode(CHuffmanTree *pArray, size_t count)
{
	m_pRoot = NULL;
	m_CodeCount = 0;
	m_MaxLength = 0;
	m_pCodeArray = NULL;
	_CompileTree(pArray, count);
	_CompileCode();
}
CHuffmanCode::~CHuffmanCode()
{
	SAFE_DELETE(m_pRoot);
	SAFE_DELETE_ARRAY(m_pCodeArray);
}
bool CHuffmanCode::GetHuffmanCode(char Byte, bool* &pCode, size_t &Length)
{
	pCode = false;
	Length = 0;
	for(size_t i = 0; i <m_CodeCount; i++)
	{
		if(m_pCodeArray[i].ANSI == Byte)
		{
			pCode = m_pCodeArray[i].pBinaryCode;
			Length = m_pCodeArray[i].count;
			return true;
		}
	}
	return false;
}
void CHuffmanCode::_GetMin(CHuffmanTree **ppArray, size_t count, 
							size_t &k1, size_t &k2)
{
	DWORD dwMin = -1;
	k1 = k2 = -1;
	if(!ppArray)
		return;
	for(size_t i = 0; i <count - 1; i++)
	{
		if(!ppArray[i])
			continue;
		for(size_t j = i + 1; j <count; j++)
		{
			if(!ppArray[j])
				continue;
			DWORD value = ppArray[i]->m_dwWeight + ppArray[j]->m_dwWeight;
			if(value <dwMin)
			{
				dwMin = value;
				k1 = i;
				k2 = j;
			}
		}
	}
}
void CHuffmanCode::_CompileTree(const CHuffmanTree *pArray, size_t count)
{
	if(!pArray || !count)
		return;
	m_CodeCount = count;
	CHuffmanTree **ppa = new CHuffmanTree*[count];
	for(size_t i = 0; i <count; i++)
	{
		ppa[i] = NULL;
		DWORD weight = pArray[i].GetWeight();
		if(!weight)
		{
			m_CodeCount--;
			continue;
		}
		ppa[i] = new CHuffmanTree(pArray[i].GetValue(), weight);
	}
	CHuffmanTree *tmp = NULL;
	for(;;)
	{
		size_t k1 = 0, k2 = 0;
		_GetMin(ppa, count, k1, k2);
		if(k1 == -1 || k2 == -1)
			break;
		DWORD dwWeight = ppa[k1]->GetWeight() + ppa[k2]->GetWeight();
		tmp = new CHuffmanTree(0, dwWeight);
		tmp->SetChildPointer(CHuffmanTree::LEFT, ppa[k1]);
		tmp->SetChildPointer(CHuffmanTree::RIGHT, ppa[k2]);
		ppa[k1] = tmp;
		ppa[k2] = NULL;
	}
	m_pRoot = tmp;
}
void CHuffmanCode::_CompileCode()
{
	m_MaxLength = m_pRoot->GetDepth() - 1;
	SAFE_DELETE(m_pCodeArray);
	m_pCodeArray = new Code[m_CodeCount];
	Code *pCodeArray = m_pCodeArray;
	bool *pLeftRightStack = new bool[m_MaxLength];
	_GetBinary(m_pRoot, 0, pCodeArray, pLeftRightStack);
	SAFE_DELETE_ARRAY(pLeftRightStack);
}
void CHuffmanCode::_GetBinary(CHuffmanTree *pTree, size_t depth,
							  Code* &pCodeArray, bool* pLeftRightStack)
{
	if(!pTree)
		return;
	CHuffmanTree **Children = pTree->m_paChildren;
	if(!Children[0] && !Children[1])
	{
		pCodeArray->ANSI = pTree->GetValue();
		pCodeArray->count = depth;
		pCodeArray->pBinaryCode = new bool[depth];
		memcpy(pCodeArray->pBinaryCode, pLeftRightStack, sizeof(bool) * depth);
		pCodeArray++;
		return;
	}
	depth++;
	pLeftRightStack[depth - 1] = 0;
	_GetBinary(pTree->m_paChildren[0], depth, pCodeArray, pLeftRightStack);
	pLeftRightStack[depth - 1] = 1;
	_GetBinary(pTree->m_paChildren[1], depth, pCodeArray, pLeftRightStack);
}