#include "PolynomialMap.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

#define EPS 1e-6

PolynomialMap::PolynomialMap(const PolynomialMap &other)
{
	m_Polynomial = other.m_Polynomial;
}

PolynomialMap::PolynomialMap(const std::string &file)
{
	assert(ReadFromFile(file));
}

PolynomialMap::PolynomialMap(const double *cof, const int *deg, int n)
{
	for (int i = 0; i < n; i++)
	{
		m_Polynomial[deg[i]] = cof[i];
	}
}

PolynomialMap::PolynomialMap(const std::vector<int> &deg, const std::vector<double> &cof)
{
	assert(deg.size() == cof.size());
	for (int i = 0; i < deg.size(); i++)
	// .size()返回size_t（unsigned int或unsigned long int），可以使用auto解决
	{
		m_Polynomial[deg[i]] = cof[i];
		// 可写为coff(deg[i])=cof[i]，因为自动判断为左值，使用了double &的函数
	}
}

double PolynomialMap::coff(int i) const
{
	std::map<int, double>::const_iterator it = m_Polynomial.find(i);
	if (it == m_Polynomial.end())
	{
		return 0.0f;
	}
	return it->second; // you should return a correct value
					   // 重点！注意检查可能出现的失效的iterator
}

double &PolynomialMap::coff(int i)
{
	return m_Polynomial[i]; // you should return a correct value
}

void PolynomialMap::compress()
{
	std::map<int, double> Polynomial = m_Polynomial;
	m_Polynomial.clear();
	for (std::map<int, double>::iterator it = Polynomial.begin(); it != Polynomial.end(); it++)
	{
		if (fabs(it->second) > EPS)
		{
			m_Polynomial[it->first] = it->second;
		}
	}
	Polynomial.clear();
}

PolynomialMap PolynomialMap::operator+(const PolynomialMap &right) const
{
	PolynomialMap Polynomial(right);
	// 构造函数
	for (std::map<int, double>::const_iterator it = m_Polynomial.begin(); it != m_Polynomial.end(); it++)
	{
		Polynomial.m_Polynomial[it->first] += it->second;
	}
	Polynomial.compress();
	return Polynomial; // you should return a correct value
}

PolynomialMap PolynomialMap::operator-(const PolynomialMap &right) const
{
	PolynomialMap Polynomial;
	Polynomial.m_Polynomial = m_Polynomial;
	for (std::map<int, double>::const_iterator it = right.m_Polynomial.begin(); it != right.m_Polynomial.end(); it++)
	{
		Polynomial.m_Polynomial[it->first] -= it->second;
	}
	Polynomial.compress();
	return Polynomial; // you should return a correct value
}

PolynomialMap PolynomialMap::operator*(const PolynomialMap &right) const
{
	PolynomialMap Polynomial;
	for (std::map<int, double>::const_iterator itl = m_Polynomial.begin(); itl != m_Polynomial.end(); itl++)
	{
		for (std::map<int, double>::const_iterator itr = right.m_Polynomial.begin(); itr != right.m_Polynomial.end(); itr++)
		{
			Polynomial.m_Polynomial[itl->first + itr->first] += itl->second * itr->second;
		}
	}
	Polynomial.compress();
	return Polynomial; // you should return a correct value
}

PolynomialMap &PolynomialMap::operator=(const PolynomialMap &right)
{
	m_Polynomial = right.m_Polynomial;
	return *this;
}

void PolynomialMap::Print() const
{
	std::map<int, double>::const_iterator it = m_Polynomial.begin();
	if (it == m_Polynomial.end())
	{
		printf("0");
	}
	for (; it != m_Polynomial.end(); it++)
	{
		if (it != m_Polynomial.begin() && it->second > 0)
		{
			printf("+");
		}
		if (it->second < 0)
		{
			printf("-");
		}
		printf("%.15g", fabs(it->second));
		if (it->first > 0)
		{
			printf("x^%d", it->first);
		}
	}
	printf("\n\n");
	return;
}

bool PolynomialMap::ReadFromFile(const std::string &file)
{
	m_Polynomial.clear();
	FILE *f = fopen(file.c_str(), "r");
	if (f == NULL)
	{
		printf("Can't open file %s!\n", file.c_str());
		return false;
	}

	char ch;
	int n;
	fscanf(f, "%c%d", &ch, &n);
	for (int i = 0; i < n; i++)
	{
		int d;
		double c;
		fscanf(f, "%d %lf", &d, &c);
		m_Polynomial[d] += c;
	}
	return true; // you should return a correct value
}
