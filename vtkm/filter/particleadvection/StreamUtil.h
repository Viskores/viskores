//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_StreamUtil_h
#define vtk_m_filter_StreamUtil_h

#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace filter
{

template <class T>
inline std::ostream& operator<<(std::ostream& os, const std::list<T>& l)
{
  os << "{";
  for (auto it = l.begin(); it != l.end(); it++)
    os << (*it) << " ";
  os << "}";
  return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::deque<T>& l)
{
  os << "{";
  for (auto it = l.begin(); it != l.end(); it++)
    os << (*it) << " ";
  os << "}";
  return os;
}

// Forward declaration so we can have pairs with vectors
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v);

template <typename T1, typename T2>
inline std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2>& p)
{
  os << "(" << p.first << "," << p.second << ")";
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
  os << "[";
  int n = v.size();
  if (n > 0)
  {
    for (int i = 0; i < n - 1; i++)
      os << v[i] << " ";
    os << v[n - 1];
  }
  os << "]";
  return os;
}

template <typename T1, typename T2>
inline std::ostream& operator<<(std::ostream& os, const std::map<T1, T2>& m)
{
  os << "{";
  for (auto it = m.begin(); it != m.end(); it++)
    os << "(" << it->first << "," << it->second << ") ";
  os << "}";
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const vtkm::cont::DataSet& ds)
{
  ds.PrintSummary(os);
  return os;
}

/*
inline std::ostream &operator<<(std::ostream &os, const vtkm::Bounds &b)
{
    os<<"{("<<b.X.Min<<":"<<b.X.Max<<")("<<b.Y.Min<<":"<<b.Y.Max<<")("<<b.Z.Min<<":"<<b.Z.Max<<")}";
    return os;
}
    */
}
} // namespace vtkm

#endif
