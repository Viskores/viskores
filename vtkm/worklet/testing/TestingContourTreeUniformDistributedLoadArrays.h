//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
#ifndef vtk_m_worklet_testing_contourtree_distributed_load_arrays_h
#define vtk_m_worklet_testing_contourtree_distributed_load_arrays_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace worklet
{
namespace testing
{
namespace contourtree_distributed
{

// Types used in binary test files
typedef size_t FileSizeType;
typedef unsigned long long FileIndexType;
const FileIndexType FileIndexMask = 0x07FFFFFFFFFFFFFFLL;
typedef double FileDataType;

inline void ReadIndexArray(std::ifstream& is, vtkm::cont::ArrayHandle<vtkm::Id>& indexArray)
{
  FileSizeType sz;
  is.read(reinterpret_cast<char*>(&sz), sizeof(sz));
  //std::cout << "Reading index array of size " << sz << std::endl;
  indexArray.Allocate(sz);
  auto writePortal = indexArray.WritePortal();

  for (vtkm::Id i = 0; i < static_cast<vtkm::Id>(sz); ++i)
  {
    FileIndexType x;
    is.read(reinterpret_cast<char*>(&x), sizeof(x));
    // Covert from index type size in file (64 bit) to index type currently used by
    // shifting the flag portion of the index accordingly
    vtkm::Id shiftedFlagVal = (x & FileIndexMask) |
      ((x & ~FileIndexMask) >> ((sizeof(FileIndexType) - sizeof(vtkm::Id)) << 3));
    writePortal.Set(i, shiftedFlagVal);
  }
}

inline void ReadIndexArrayVector(std::ifstream& is,
                                 std::vector<vtkm::cont::ArrayHandle<vtkm::Id>>& indexArrayVector)
{
  FileSizeType sz;
  is.read(reinterpret_cast<char*>(&sz), sizeof(sz));
  //std::cout << "Reading vector of " << sz << " index arrays" << std::endl;
  indexArrayVector.resize(sz);

  for (vtkm::Id i = 0; i < static_cast<vtkm::Id>(sz); ++i)
  {
    ReadIndexArray(is, indexArrayVector[i]);
  }
}

template <class FieldType>
inline void ReadDataArray(std::ifstream& is, vtkm::cont::ArrayHandle<FieldType>& dataArray)
{
  FileSizeType sz;
  is.read(reinterpret_cast<char*>(&sz), sizeof(sz));
  //std::cout << "Reading data array of size " << sz << std::endl;
  dataArray.Allocate(sz);
  auto writePortal = dataArray.WritePortal();

  for (vtkm::Id i = 0; i < static_cast<vtkm::Id>(sz); ++i)
  {
    FileDataType x;
    is.read(reinterpret_cast<char*>(&x), sizeof(x));
    //std::cout << "Read " << x << std::endl;
    writePortal.Set(
      i,
      FieldType(x)); // Test data is stored as double but generally is also ok to be cast to float.
  }
}

} // namespace contourtree_distributed
} // namespace testing
} // namespace worklet
} // namespace vtkm

#endif
