/*
		Name: Huffman.h
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#pragma once
#include "Safe.h"

class CHuffmanTree
{
public:
	friend class CHuffmanCode;
	typedef enum{ LEFT = 0, RIGHT = 1 } LR;
private:
	CHuffmanTree	*m_paChildren[2];
	char			m_cValue;
	DWORD			m_dwWeight;
	CHuffmanTree	*m_pCurrent;//游标节点指针(用于霍夫曼编码->ANSI码(或八位二进制码)的转换
private:
	inline void _ClearChildPointers();
	inline void _SetChildPointer(LR lr, CHuffmanTree *pTree);
public:
	explicit CHuffmanTree(char Value = 0, DWORD dwWeight = 0);
	~CHuffmanTree();
	bool	SetChildPointer(LR lr, CHuffmanTree *pTree);
	bool	AddChild(LR lr, char Value = 0);
	void	SetValue(char Value);
	void	SetWeight(DWORD dwWeight);
	char	GetValue() const;
	DWORD	GetWeight() const;
	void	ReleaseChild(LR lrChild);
	void	ReleaseChildren();
	size_t	GetDepth(size_t base = 0);
	bool	CreateHuffmanFromCode(char ANSI, bool *pCode, DWORD dwLength);
	bool	SearchAnsiFromNextCode(char &ANSI, bool bBinary);
	void	SearchReset();
};

class CHuffmanCode
{
public:
	struct Code
	{
		char	ANSI;
		bool	*pBinaryCode;
		size_t	count;
		Code(): ANSI('\0'), pBinaryCode(NULL){};
		~Code(){ SAFE_DELETE_ARRAY(pBinaryCode); };
	};
private:
	CHuffmanTree	*m_pRoot;
	size_t			m_MaxLength;
	Code			*m_pCodeArray;
	size_t			m_CodeCount;
private:
	void _CompileTree(const CHuffmanTree *pArray, size_t count);
	void _CompileCode();
	static void _GetBinary(CHuffmanTree *pRootTree, size_t depth,
							Code* &pCodeArray, bool* pLeftRightStack);
	inline void _GetMin(CHuffmanTree **ppArray, size_t count, 
					size_t &k1, size_t &k2);
public:
	CHuffmanCode(CHuffmanTree *pArray, size_t count);
	~CHuffmanCode();
	bool GetHuffmanCode(char Byte, bool* &pCode, size_t &Length);

	inline CHuffmanTree* GetRoot() const{ return m_pRoot; };
	inline Code* GetCodeArray() const{ return m_pCodeArray; };
	inline size_t GetCodeArrayCount() const{ return m_CodeCount; };
};