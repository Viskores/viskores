//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_MemStream_h
#define vtk_m_filter_MemStream_h

#include <vtkm/filter/vtkm_filter_export.h>

#include <cstring>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace vtkm
{
namespace filter
{

class MemStream
{
public:
  MemStream(std::size_t sz0 = 32);
  MemStream(std::size_t sz, const unsigned char* buff);
  MemStream(const MemStream& s);
  ~MemStream();

  void Rewind() { this->Pos = 0; }
  std::size_t GetPos() const { return this->Pos; }
  void SetPos(std::size_t p);
  std::size_t GetLen() const { return this->Len; }
  std::size_t GetCapacity() const { return this->MaxLen; }
  unsigned char* GetData() const { return this->Data; }

  //Read from buffer.
  void ReadBinary(unsigned char* buff, const std::size_t& size);

  //Write to buffer.
  void WriteBinary(const unsigned char* buff, std::size_t size);

  void ClearMemStream();

private:
  // data members
  unsigned char* Data;
  std::size_t Len;
  std::size_t MaxLen;
  std::size_t Pos;

  void CheckSize(std::size_t sz);

  friend std::ostream& operator<<(std::ostream& out, const MemStream& m)
  {
    out << " MemStream(p= " << m.GetPos() << ", l= " << m.GetLen() << "[" << m.GetCapacity()
        << "]) data=[";
    /*
        for (std::size_t i=0; i < m.GetLen(); i++)
            out<<(int)(m.Data[i])<<" ";
        */
    out << "]";
    return out;
  }
};

inline void MemStream::ReadBinary(unsigned char* buff, const std::size_t& size)
{
  std::size_t nBytes = sizeof(unsigned char) * size;
  std::memcpy(buff, &this->Data[this->Pos], nBytes);
  this->Pos += nBytes;
}

inline void MemStream::WriteBinary(const unsigned char* buff, std::size_t size)
{
  std::size_t nBytes = sizeof(unsigned char) * size;
  this->CheckSize(nBytes);
  std::memcpy(&this->Data[this->Pos], buff, nBytes);
  this->Pos += nBytes;

  if (this->Pos > this->Len)
    this->Len = this->Pos;
}

inline void MemStream::SetPos(std::size_t p)
{
  this->Pos = p;
  if (this->Pos > this->GetLen())
    throw "MemStream::setPos failed";
}

template <typename T>
struct Serialization
{
#if (defined(__clang__) && !defined(__ppc64__)) || (defined(__GNUC__) && __GNUC__ >= 5)
  static_assert(std::is_trivially_copyable<T>::value,
                "Default serialization works only for trivially copyable types");
#endif
  static void write(MemStream& memstream, const T& data)
  {
    memstream.WriteBinary((const unsigned char*)&data, sizeof(T));
  }
  static void read(MemStream& memstream, T& data)
  {
    memstream.ReadBinary((unsigned char*)&data, sizeof(T));
  }
};

template <typename T>
static void write(MemStream& memstream, const T& data)
{
  Serialization<T>::write(memstream, data);
}

template <typename T>
static void read(MemStream& memstream, T& data)
{
  Serialization<T>::read(memstream, data);
}

template <class T>
struct Serialization<std::vector<T>>
{
  static void write(MemStream& memstream, const std::vector<T>& data)
  {
    const std::size_t sz = data.size();
    vtkm::filter::write(memstream, sz);
    for (std::size_t i = 0; i < sz; i++)
      vtkm::filter::write(memstream, data[i]);
  }

  static void read(MemStream& memstream, std::vector<T>& data)
  {
    std::size_t sz;
    vtkm::filter::read(memstream, sz);
    data.resize(sz);
    for (std::size_t i = 0; i < sz; i++)
      vtkm::filter::read(memstream, data[i]);
  }
};

template <class T>
struct Serialization<std::list<T>>
{
  static void write(MemStream& memstream, const std::list<T>& data)
  {
    vtkm::filter::write(memstream, data.size());
    typename std::list<T>::const_iterator it;
    for (it = data.begin(); it != data.end(); it++)
      vtkm::filter::write(memstream, *it);
  }

  static void read(MemStream& memstream, std::list<T>& data)
  {
    std::size_t sz;
    vtkm::filter::read(memstream, sz);
    for (std::size_t i = 0; i < sz; i++)
    {
      T v;
      vtkm::filter::read(memstream, v);
      data.push_back(v);
    }
  }
};

template <class T, class U>
struct Serialization<std::pair<T, U>>
{
  static void write(MemStream& memstream, const std::pair<T, U>& data)
  {
    vtkm::filter::write(memstream, data.first);
    vtkm::filter::write(memstream, data.second);
  }

  static void read(MemStream& memstream, std::pair<T, U>& data)
  {
    vtkm::filter::read(memstream, data.first);
    vtkm::filter::read(memstream, data.second);
  }
};

//template<>
//struct Serialization<std::string>
//{
//  static void write(MemStream &memstream, const std::string &data)
//  {
//    std::size_t sz = data.size();
//    memstream.write(sz);
//    memstream.write(data.data(), sz);
//  }
//
//  static void read(MemStream &memstream, std::string &data)
//  {
//    std::size_t sz;
//    memstream.read(sz);
//    data.resize(sz);
//    memstream.read(&data[0], sz);
//  }
//};
}
} // namespace vtkm::filter
#endif
