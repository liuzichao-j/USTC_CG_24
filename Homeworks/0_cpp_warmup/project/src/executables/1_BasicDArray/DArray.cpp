// implementation of class DArray
#include "DArray.h"

#include <cstdio>
#include <cstdlib>

#include <typeinfo>
#include <cstring>

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
	for (int i = 0; i < m_nSize; i++)
	{
		printf("%lf\n", m_pData[i]);
	}
	return;
}

// initilize the array
void DArray::Init()
{
	m_nSize = 0;
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
	m_nSize = nSize;
	m_pData = (double *)malloc(nSize * sizeof(double));
	if (m_pData == NULL)
	{
		printf("Out of Memory!\n");
		exit(0);
	}
	for (int i = 0; i < nSize; i++)
	{
		m_pData[i] = 0.0f;
	}
	return;
}

// get an element at an index
const double &DArray::GetAt(int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		printf("Invalid Index at GetAt()!\n");
		return 0;
	}
	return m_pData[nIndex]; // you should return a correct value
}

// set the value of an element
void DArray::SetAt(int nIndex, double dValue)
{
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		printf("Invalid Index at SetAt()!\n");
	}
	else
	{
		m_pData[nIndex] = dValue;
	}
	return;
}

// overload operator '[]'
const double &DArray::operator[](int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		return 0;
	}
	return m_pData[nIndex]; // you should return a correct value
}

// add a new element at the end of the array
void DArray::PushBack(double dValue)
{
	double *m_pDataNew = (double *)malloc((m_nSize + 1) * sizeof(double));
	if (m_pDataNew == NULL)
	{
		printf("Out of Memory!\n");
		exit(0);
	}

	for (int i = 0; i < m_nSize; i++)
	{
		m_pDataNew[i] = m_pData[i];
	}

	m_pDataNew[++m_nSize] = dValue;
	free(m_pData);
	m_pData = m_pDataNew;

	return;
}

// delete an element at some index
void DArray::DeleteAt(int nIndex)
{
	if (nIndex < 0 || nIndex > m_nSize)
	{
		printf("Invalid Index at DeleteAt()!\n");
		return;
	}
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
	double *m_pDataNew = (double *)malloc((m_nSize + 1) * sizeof(double));
	if (m_pDataNew == NULL)
	{
		printf("Out of Memory!\n");
		exit(0);
	}

	for (int i = 0; i < nIndex; i++)
	{
		m_pDataNew[i] = m_pData[i];
	}
	m_pDataNew[nIndex] = dValue;
	for (int i = nIndex + 1; i <= m_nSize; i++)
	{
		m_pDataNew[i] = m_pData[i - 1];
	}

	m_pDataNew[++m_nSize] = dValue;
	free(m_pData);
	m_pData = m_pDataNew;

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
