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
	: m_pData(new double[nSize]),m_nSize(nSize)
// 重点！善用便捷的构造函数形式和new函数代替malloc
{
	for (int i = 0; i < nSize; i++)
	{
		m_pData[i] = dValue;
	}
}

DArray::DArray(const DArray &arr)
	: m_pData(new double[arr.m_nSize]), m_nSize(arr.m_nSize)
{
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.GetAt(i);
		// 使用外接接口函数，但是也可以直接访问arr.m_pData[i]
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
		printf("%.15g\n", m_pData[i]); // 仅输出有效位数
	}
	printf("\n");
	return;
}

// initilize the array
void DArray::Init()
{
	m_nSize = 0;
	m_pData = NULL; // 或nullptr
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
	// 上述语句可以直接用free，因为free(NULL)无任何行为或报错
	// 可以使用便捷的delete[] m_pData;
	m_nSize = 0;
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
	double *pDataNew = (double *)malloc(nSize * sizeof(double));
	// 可改作 = new double[nSize]，可以对nSize==m_nSize特判
	if (pDataNew == NULL)
	{
		printf("Out of Memory! \n");
		exit(0);
	}

	int nCopyNum = nSize > m_nSize ? m_nSize : nSize;
	for (int i = 0; i < nCopyNum; i++)
	{
		pDataNew[i] = m_pData[i];
	}
	for (int i = nCopyNum; i < nSize; i++)
	{
		pDataNew[i] = 0.0f;
	}

	m_nSize = nSize;
	free(m_pData);
	m_pData = pDataNew;
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
const double &DArray::operator[](int nIndex) const
{
	assert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// add a new element at the end of the array
void DArray::PushBack(double dValue)
{
	double *pDataNew = (double *)malloc((m_nSize + 1) * sizeof(double));
	if (pDataNew == NULL)
	{
		printf("Out of Memory! \n");
		exit(0);
	}

	for (int i = 0; i < m_nSize; i++)
	{
		pDataNew[i] = m_pData[i];
	}

	pDataNew[m_nSize++] = dValue;
	free(m_pData);
	m_pData = pDataNew;

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
	double *pDataNew = (double *)malloc((m_nSize + 1) * sizeof(double));
	// new double[static_cast<size_t>(m_nSize) + 1]，使用强制类型转换为size_t（一般为unsigned int）
	if (pDataNew == NULL)
	{
		printf("Out of Memory! \n");
		exit(0);
	}

	for (int i = 0; i < nIndex; i++)
	{
		pDataNew[i] = m_pData[i];
	}
	pDataNew[nIndex] = dValue;
	for (int i = nIndex + 1; i <= m_nSize; i++)
	{
		pDataNew[i] = m_pData[i - 1];
	}

	m_nSize++;
	free(m_pData);
	m_pData = pDataNew;
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
