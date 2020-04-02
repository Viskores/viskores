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
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

// include guard
#ifndef vtk_m_worklet_contourtree_augmented_print_vectors_h
#define vtk_m_worklet_contourtree_augmented_print_vectors_h

// global libraries
#include <iomanip>
#include <iostream>
#include <string>
#include <vtkm/Pair.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

// local includes
#include <vtkm/cont/arg/Transport.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// local constants to allow changing the spacing as needed
constexpr int PRINT_WIDTH = 12;
constexpr int PREFIX_WIDTH = 24;

template <typename T, typename StorageType>
void PrintValues(std::string label,
                 const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                 vtkm::Id nValues = -1);
void PrintIndices(std::string label,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& iVec,
                  vtkm::Id nIndices = -1);
template <typename T, typename StorageType>
void PrintSortedValues(std::string label,
                       const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                       IdArrayType& sortVec,
                       vtkm::Id nValues = -1);

// base routines for printing label & prefix bars
inline void PrintLabel(std::string label)
{ // PrintLabel()
  // print out the front end
  std::cout << std::setw(PREFIX_WIDTH) << std::left << label;
  // print out the vertical line
  std::cout << std::right << "|";
} // PrintLabel()


inline void PrintSeparatingBar(vtkm::Id howMany)
{ // PrintSeparatingBar()
  // print out the front end
  std::cout << std::setw(PREFIX_WIDTH) << std::setfill('-') << "";
  // now the + at the vertical line
  std::cout << "+";
  // now print out the tail end - fixed number of spaces per entry
  for (vtkm::Id block = 0; block < howMany; block++)
  {
    std::cout << std::setw(PRINT_WIDTH) << std::setfill('-') << "";
  }
  // now the std::endl, resetting the fill character
  std::cout << std::setfill(' ') << std::endl;
} // PrintSeparatingBar()


// routine to print out a single value
template <typename T>
inline void PrintDataType(T value)
{ // PrintDataType
  std::cout << std::setw(PRINT_WIDTH) << value;
} // PrintDataType


// routine to print out a single index
inline void PrintIndexType(vtkm::Id index)
{ // PrintIndexType
  std::cout << std::setw(PRINT_WIDTH - 6) << MaskedIndex(index) << " " << FlagString(index);
} // PrintIndexType


// header line
inline void PrintHeader(vtkm::Id howMany)
{ // PrintHeader()
  // print out a separating bar
  PrintSeparatingBar(howMany);
  // print out a label
  PrintLabel("ID");
  // print out the ID numbers
  for (vtkm::Id entry = 0; entry < howMany; entry++)
  {
    PrintIndexType(entry);
  }
  // and an std::endl
  std::cout << std::endl;
  // print out another separating bar
  PrintSeparatingBar(howMany);
} // PrintHeader()


// base routines for reading & writing host vectors
template <typename T, typename StorageType>
inline void PrintValues(std::string label,
                        const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                        vtkm::Id nValues)
{ // PrintValues()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = dVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label);

  // now print the data
  auto portal = dVec.ReadPortal();
  for (vtkm::Id entry = 0; entry < nValues; entry++)
  {
    PrintDataType(portal.Get(entry));
  }
  // and an std::endl
  std::cout << std::endl;
} // PrintValues()


// base routines for reading & writing host vectors
template <typename T, typename StorageType>
inline void PrintSortedValues(std::string label,
                              const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                              IdArrayType& sortVec,
                              vtkm::Id nValues)
{ // PrintSortedValues()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = sortVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label);

  // now print the data
  auto dportal = dVec.ReadPortal();
  auto sortPortal = sortVec.ReadPortal();
  for (vtkm::Id entry = 0; entry < nValues; entry++)
  {
    PrintDataType(dportal.Get(sortPortal.Get(entry)));
  }
  // and an std::endl
  std::cout << std::endl;
} // PrintSortedValues()


// routine for printing index arrays
inline void PrintIndices(std::string label,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& iVec,
                         vtkm::Id nIndices)
{ // PrintIndices()
  // -1 means full size
  if (nIndices == -1)
  {
    nIndices = iVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label);

  auto portal = iVec.ReadPortal();
  for (vtkm::Id entry = 0; entry < nIndices; entry++)
    PrintIndexType(portal.Get(entry));

  // and the std::endl
  std::cout << std::endl;
} // PrintIndices()


template <typename T, typename StorageType>
inline void PrintLabelledDataBlock(std::string label,
                                   const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                                   vtkm::Id nColumns)
{ // PrintLabelledDataBlock()
  // start with a header
  PrintHeader(nColumns);
  // loop control variable
  vtkm::Id entry = 0;
  // per row
  auto portal = dVec.ReadPortal();
  for (vtkm::Id row = 0; entry < portal.GetNumberOfValues(); row++)
  { // per row
    PrintLabel(label + "[" + std::to_string(row) + "]");
    // now print the data
    for (vtkm::Id col = 0; col < nColumns; col++, entry++)
    {
      PrintDataType(portal.Get(entry));
    }
    std::cout << std::endl;
  } // per row
  // and a final std::endl
  std::cout << std::endl;
} // PrintLabelledDataBlock()


// routine for printing list of edge pairs. Used, e.g., to print the sorted list of saddle peaks from the ContourTree
inline void PrintEdgePairArray(const EdgePairArray& edgePairArray)
{ // PrintEdgePairArray()
  // now print them out
  auto edgePairArrayConstPortal = edgePairArray.ReadPortal();
  for (vtkm::Id superarc = 0; superarc < edgePairArray.GetNumberOfValues(); superarc++)
  { // per superarc
    std::cout << std::right << std::setw(PRINT_WIDTH)
              << edgePairArrayConstPortal.Get(superarc).first << " ";
    std::cout << std::right << std::setw(PRINT_WIDTH)
              << edgePairArrayConstPortal.Get(superarc).second << std::endl;
  } // per superarc
} // PrintEdgePairArray()


} // namespace contourtree_augmented
} // worklet
} // vtkm

// tail of include guard
#endif
