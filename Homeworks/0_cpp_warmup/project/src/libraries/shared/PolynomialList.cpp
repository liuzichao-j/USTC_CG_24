#include "PolynomialList.h"

#include <cstdio>
#include <cassert>

const double EPS = 1e-6;
// 重点！double的eps 可以设置为1.0e-10

PolynomialList::PolynomialList(const PolynomialList &other)
{
    m_Polynomial = other.m_Polynomial;
}

PolynomialList::PolynomialList(const std::string &file)
{
    assert(ReadFromFile(file));
}

PolynomialList::PolynomialList(const double *cof, const int *deg, int n)
{
    for (int i = 0; i < n; i++)
    {
        AddOneTerm(Term(deg[i], cof[i]));
    }
}

PolynomialList::PolynomialList(const std::vector<int> &deg, const std::vector<double> &cof)
{
    assert(deg.size() == cof.size());
    for (int i = 0; i < deg.size(); i++)
    {
        AddOneTerm(Term(deg.at(i), cof.at(i)));
    }
}

double PolynomialList::coff(int i) const
{
    for (std::list<Term>::const_iterator it = m_Polynomial.begin(); it != m_Polynomial.end(); it++)
    {
        if (it->deg == i)
        {
            return it->cof;
        }
    }

    // C++11：基于范围的for循环，for(const Term &term : m_Polynomial)
    return 0.; // you should return a correct value
}

double &PolynomialList::coff(int i)
{
    return AddOneTerm(Term(i, 0)).cof; // you should return a correct value
    // 重点！应返回对应元素而非副本（由&自动保证）
}

void PolynomialList::compress()
{
    std::list<Term>::iterator it = m_Polynomial.begin();
    for (; it != m_Polynomial.end();)
    {
        if (fabs(it->cof) < EPS)
        {
            it = m_Polynomial.erase(it);
            // 重点！erase返回下一个iterator，若不赋值会失效，此时也无需++
        }
        else
        {
            it++;
        }
    }
    return;
}

PolynomialList PolynomialList::operator+(const PolynomialList &right) const
{
    PolynomialList Polynomial(*this);
    for (std::list<Term>::const_iterator it = right.m_Polynomial.begin(); it != right.m_Polynomial.end(); it++)
    {
        Polynomial.AddOneTerm(*it);
    }
    Polynomial.compress();
    return Polynomial; // you should return a correct value
}

PolynomialList PolynomialList::operator-(const PolynomialList &right) const
{
    PolynomialList Polynomial(*this);
    for (std::list<Term>::const_iterator it = right.m_Polynomial.begin(); it != right.m_Polynomial.end(); it++)
    {
        Polynomial.AddOneTerm(Term(it->deg, -it->cof));
    }
    Polynomial.compress();
    return Polynomial; // you should return a correct value
}

PolynomialList PolynomialList::operator*(const PolynomialList &right) const
{
    PolynomialList Polynomial;
    for (std::list<Term>::const_iterator itl = m_Polynomial.begin(); itl != m_Polynomial.end(); itl++)
    {
        for (std::list<Term>::const_iterator itr = right.m_Polynomial.begin(); itr != right.m_Polynomial.end(); itr++)
        {
            Polynomial.AddOneTerm(Term(itl->deg + itr->deg, itl->cof * itr->cof));
        }
    }
    Polynomial.compress();
    return Polynomial; // you should return a correct value
}

PolynomialList &PolynomialList::operator=(const PolynomialList &right)
{
    m_Polynomial = right.m_Polynomial;
    return *this;
}

void PolynomialList::Print() const
{
    std::list<Term>::const_iterator it = m_Polynomial.begin();
    if (it == m_Polynomial.end())
    {
        printf("0");
    }
    for (; it != m_Polynomial.end(); it++)
    {
        if (it != m_Polynomial.begin() && it->cof > 0)
        {
            printf("+");
        }
        if (it->cof < 0)
        {
            printf("-");
        }

        printf("%.15g", fabs(it->cof));
        if (it->deg != 0)
        {
            printf("x^%d", it->deg);
        }
    }
    printf("\n\n");
    return;
}

bool PolynomialList::ReadFromFile(const std::string &file)
{
    FILE *f = fopen(file.c_str(), "r");
    if (f == NULL)
    {
        printf("Can't read file %s!\n", file.c_str());
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
        AddOneTerm(Term(d, c));
    }
    fclose(f);
    return true; // you should return a correct value
}

PolynomialList::Term &PolynomialList::AddOneTerm(const Term &term)
{
    std::list<Term>::iterator it = m_Polynomial.begin();
    for (; it != m_Polynomial.end(); it++)
    {
        if (it->deg == term.deg)
        {
            it->cof += term.cof;
            return *it;
        }
        if (it->deg > term.deg) // 从小次数到大次数，与map的int型默认顺序相合
        {
            break;
        }
    }
    return *m_Polynomial.insert(it, term); // you should return a correct value
}
