//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/particleadvection/MemStream.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

MemStream::MemStream(std::size_t sz0)
  : Data(nullptr)
  , Len(0)
  , MaxLen(0)
  , Pos(0)
{
  CheckSize(sz0);
}

MemStream::MemStream(std::size_t sz, const unsigned char* buff)
  : Data(nullptr)
  , Len(sz)
  , MaxLen(sz)
  , Pos(0)
{
  this->Data = new unsigned char[this->Len];
  std::memcpy(this->Data, buff, this->Len);
}

MemStream::MemStream(const MemStream& s)
{
  this->Pos = s.GetPos();
  this->Len = s.GetLen();
  this->MaxLen = this->Len;
  this->Data = new unsigned char[this->Len];
  std::memcpy(this->Data, s.GetData(), this->Len);
}

MemStream::MemStream(MemStream&& s)
{
  this->Pos = 0;
  this->Len = s.GetLen();
  this->MaxLen = this->Len;
  this->Data = s.Data;

  s.Pos = 0;
  s.Len = 0;
  s.MaxLen = 0;
  s.Data = nullptr;
}

MemStream::~MemStream()
{
  this->ClearMemStream();
}

void MemStream::ClearMemStream()
{
  if (this->Data)
  {
    delete[] this->Data;
    this->Data = nullptr;
  }
  this->Pos = 0;
  this->Len = 0;
  this->MaxLen = 0;
}

void MemStream::CheckSize(std::size_t sz)
{
  std::size_t reqLen = this->Pos + sz;

  if (reqLen > this->MaxLen)
  {
    std::size_t newLen = 2 * this->MaxLen; // double current size.
    if (newLen < reqLen)
      newLen = reqLen;

    unsigned char* newData = new unsigned char[newLen];

    if (this->Data)
    {
      std::memcpy(newData, this->Data, this->Len); // copy existing data to new buffer.
      delete[] this->Data;
    }
    this->Data = newData;
    this->MaxLen = newLen;
  }
}
}
}
} // namespace vtkm::filter::particleadvection
