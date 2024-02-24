#pragma once

#include <cstdio>
#include <cstdlib>

#include <typeinfo>
#include <cstring>
#include <cassert>

// interfaces of Dynamic Array class DArray
template <class Datatype>
class DArray
{
public:
	DArray();								// default constructor
	DArray(int nSize, Datatype dValue = 0); // set an array with default values
	DArray(const DArray<Datatype> &arr);	// copy constructor
	~DArray();								// deconstructor

	void Print() const; // print the elements of the array

	int GetSize() const;	 // get the size of the array
	void SetSize(int nSize); // set the size of the array

	const Datatype &GetAt(int nIndex) const; // get an element at an index
	void SetAt(int nIndex, Datatype dValue); // set the value of an element

	Datatype &operator[](int nIndex);			  // overload operator '[]'
	const Datatype &operator[](int nIndex) const; // overload operator '[]'

	void PushBack(Datatype dValue);				// add a new element at the end of the array
	void DeleteAt(int nIndex);					// delete an element at some index
	void InsertAt(int nIndex, Datatype dValue); // insert a new element at some index

	DArray<Datatype> &operator=(const DArray<Datatype> &arr); // overload operator '='

private:
	Datatype *m_pData; // the pointer to the array memory
	int m_nSize;	   // the size of the array
	int m_nMax;

private:
	void Init();			 // initilize the array
	void Free();			 // free the array
	void Reserve(int nSize); // allocate enough memory
};

// default constructor
template <class Datatype>
DArray<Datatype>::DArray()
{
	Init();
}

// set an array with default values
template <class Datatype>
DArray<Datatype>::DArray(int nSize, Datatype dValue)
{
	Init();
	SetSize(nSize);
	for (int i = 0; i < nSize; i++)
	{
		m_pData[i] = dValue;
	}
}

template <class Datatype>
DArray<Datatype>::DArray(const DArray<Datatype> &arr)
{
	Init();
	SetSize(arr.GetSize());
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.GetAt(i);
	}
}

// deconstructor
template <class Datatype>
DArray<Datatype>::~DArray()
{
	Free();
}

// display the elements of the array
template <class Datatype>
void DArray<Datatype>::Print() const
{
	if (m_nSize == 0)
	{
		printf("Nothing in this array. \n");
	}
	char const *typen = typeid(Datatype).name();
	for (int i = 0; i < m_nSize; i++)
	{
		if (!strcmp(typen, "int"))
		{
			printf("%d\n", (int)m_pData[i]);
		}
		if (!strcmp(typen, "char"))
		{
			printf("%c\n", (char)m_pData[i]);
		}
		if (!strcmp(typen, "float"))
		{
			printf("%f\n", (float)m_pData[i]);
		}
		if (!strcmp(typen, "double"))
		{
			printf("%lf\n", (double)m_pData[i]);
		}
	}
	printf("\n");
	return;
}

// initilize the array
template <class Datatype>
void DArray<Datatype>::Init()
{
	m_nSize = 0;
	m_nMax = 0;
	m_pData = NULL;
	return;
}

// free the array
template <class Datatype>
void DArray<Datatype>::Free()
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

template <class Datatype>
void DArray<Datatype>::Reserve(int nSize)
{
	int nMax = 1;
	while (nSize != 0)
	{
		nSize /= 2;
		nMax *= 2;
	}
	m_nMax = nMax;

	Datatype *pDataNew = (Datatype *)malloc(nMax * sizeof(Datatype));
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

	free(m_pData);
	m_pData = pDataNew;
	return;
}

// get the size of the array
template <class Datatype>
int DArray<Datatype>::GetSize() const
{
	return m_nSize; // you should return a correct value
}

// set the size of the array
template <class Datatype>
void DArray<Datatype>::SetSize(int nSize)
{
	if (nSize > m_nMax)
	{
		Reserve(nSize);
	}

	m_nSize = nSize;
	return;
}

// get an element at an index
template <class Datatype>
const Datatype &DArray<Datatype>::GetAt(int nIndex) const
{
	assert(nIndex >= 0 || nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// set the value of an element
template <class Datatype>
void DArray<Datatype>::SetAt(int nIndex, Datatype dValue)
{
	assert(nIndex >= 0 || nIndex < m_nSize);
	m_pData[nIndex] = dValue;
	return;
}

// overload operator '[]'
template <class Datatype>
Datatype &DArray<Datatype>::operator[](int nIndex)
{
	assert(nIndex >= 0 || nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// overload operator '[]'
template <class Datatype>
const Datatype &DArray<Datatype>::operator[](int nIndex) const
{
	assert(nIndex >= 0 || nIndex < m_nSize);
	return m_pData[nIndex]; // you should return a correct value
}

// add a new element at the end of the array
template <class Datatype>
void DArray<Datatype>::PushBack(Datatype dValue)
{
	SetSize(m_nSize + 1);
	m_pData[m_nSize - 1] = dValue;
	return;
}

// delete an element at some index
template <class Datatype>
void DArray<Datatype>::DeleteAt(int nIndex)
{
	assert(nIndex >= 0 || nIndex < m_nSize);
	for (int i = nIndex; i < m_nSize - 1; i++)
	{
		m_pData[i] = m_pData[i + 1];
	}
	m_nSize--;
	return;
}

// insert a new element at some index
template <class Datatype>
void DArray<Datatype>::InsertAt(int nIndex, Datatype dValue)
{
	assert(nIndex >= 0 || nIndex < m_nSize);
	SetSize(m_nSize + 1);
	for (int i = m_nSize - 1; i > nIndex; i--)
	{
		m_pData[i] = m_pData[i - 1];
	}
	m_pData[nIndex] = dValue;
	return;
}

// overload operator '='
template <class Datatype>
DArray<Datatype> &DArray<Datatype>::operator=(const DArray<Datatype> &arr)
{
	SetSize(arr.GetSize());
	for (int i = 0; i < m_nSize; i++)
	{
		m_pData[i] = arr.GetAt(i);
	}
	return *this;
}
