// implementation of class DArray
#include "DArray.h"

#include <cstdio>
#include <cstdlib>

#include <typeinfo>
#include <cstring>
#include <cassert>

// default constructor
DArray::DArray()
{
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue)
{
	Init();
	SetSize(nSize);
	for (int i = 0; i < nSize; i++)
	{
		m_pData[i] = dValue;
	}
}

DArray::DArray(const DArray &arr)
{
	Init();
	SetSize(arr.GetSize());
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.GetAt(i);
	}
}

// deconstructor
DArray::~DArray()
{
	Free();
}

// display the elements of the array
void DArray::Print() const
{
	if (m_nSize == 0)
	{
		printf("Nothing in this array. \n");
	}
	for (int i = 0; i < m_nSize; i++)
	{
		printf("%.15g\n", m_pData[i]);
	}
	printf("\n");
	return;
}

// initilize the array
void DArray::Init()
{
	m_nSize = 0;
	m_nMax = 0;
	m_pData = NULL;
	return;
}

// free the array
void DArray::Free()
{
	if (m_pData != NULL)
	{
		free(m_pData);
		m_pData = NULL;
	}
	m_nSize = 0;
	m_nMax = 0;
	return;
}

void DArray::Reserve(int nSize)
{
	int nMax = 1;
	while (nSize != 0)
	{
		nSize /= 2;
		nMax *= 2;
	}
	m_nMax = nMax;

	// 另解：倍增m_nMax即可，但缩小后不会恢复

	double *pDataNew = (double *)malloc(nMax * sizeof(double));
	if (pDataNew == NULL)
	{
		printf("Out of Memory! \n");
		exit(0);
	}

	for (int i = 0; i < m_nSize; i++)
	{
		pDataNew[i] = m_pData[i];
	}
	for (int i = m_nSize; i < nMax; i++)
	{
		pDataNew[i] = 0.0f;
	}
	// memcpy(pData, m_pData, m_nSize * sizeof(double));

	free(m_pData);
	m_pData = pDataNew;
	return;
}

// get the size of the array
int DArray::GetSize() const
{
	return m_nSize; // you should return a correct value
}

// set the size of the array
void DArray::SetSize(int nSize)
{
	if (nSize > m_nMax)
	{
		Reserve(nSize);
	}
	// 可以均Reverse
	m_nSize = nSize;
	return;
}

// get an element at an index
const double &DArray::GetAt(int nIndex) const
{
	assert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// set the value of an element
void DArray::SetAt(int nIndex, double dValue)
{
	assert(nIndex >= 0 && nIndex < m_nSize);
	m_pData[nIndex] = dValue;
	return;
}

// overload operator '[]'
double &DArray::operator[](int nIndex)
{
	assert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// overload operator '[]'
const double &DArray::operator[](int nIndex) const
{
	assert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// add a new element at the end of the array
void DArray::PushBack(double dValue)
{
	SetSize(m_nSize + 1);
	m_pData[m_nSize - 1] = dValue;
	return;
}

// delete an element at some index
void DArray::DeleteAt(int nIndex)
{
	assert(nIndex >= 0 && nIndex < m_nSize);
	for (int i = nIndex; i < m_nSize - 1; i++)
	{
		m_pData[i] = m_pData[i + 1];
	}
	m_nSize--;
	return;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue)
{
	assert(nIndex >= 0 && nIndex <= m_nSize);
	SetSize(m_nSize + 1);
	for (int i = m_nSize - 1; i > nIndex; i--)
	{
		m_pData[i] = m_pData[i - 1];
	}
	m_pData[nIndex] = dValue;
	return;
}

// overload operator '='
DArray &DArray::operator=(const DArray &arr)
{
	SetSize(arr.GetSize());
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.GetAt(i);
	}
	return *this;
}
