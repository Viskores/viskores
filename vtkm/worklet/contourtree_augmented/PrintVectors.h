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
#ifndef vtkm_worklet_contourtree_augmented_print_vectors_h
#define vtkm_worklet_contourtree_augmented_print_vectors_h

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

// and we store a debug value for the number of printing columns
constexpr vtkm::Id printCols = 10;

template <typename T, typename StorageType>
void printValues(std::string label,
                 const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                 vtkm::Id nValues = -1);
void printIndices(std::string label,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& iVec,
                  vtkm::Id nIndices = -1);
template <typename T, typename StorageType>
void printSortedValues(std::string label,
                       const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                       IdArrayType& sortVec,
                       vtkm::Id nValues = -1);

// base routines for printing label & prefix bars
inline void printLabel(std::string label)
{ // printLabel()
  // print out the front end
  std::cout << std::setw(PREFIX_WIDTH) << std::left << label;
  // print out the vertical line
  std::cout << std::right << "|";
} // printLabel()

inline void printSeparatingBar(vtkm::Id howMany)
{ // printSeparatingBar()
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
} // printSeparatingBar()

// routine to print out a single value
template <typename T>
inline void printDataType(T value)
{ // printDataType
  std::cout << std::setw(PRINT_WIDTH) << value;
} // printDataType

// routine to print out a single index
inline void printIndexType(vtkm::Id index)
{ // printIndexType
  std::cout << std::setw(PRINT_WIDTH - 6) << maskedIndex(index) << " " << flagString(index);
} // printIndexType

// print blank of width PRINT_WIDTH
inline void printBlank()
{ // printBlank()
  std::cout << std::setw(PRINT_WIDTH) << "";
} // printBlank()

// print text with PRINT_WIDTH indent
inline void printText(std::string text)
{ // printText()
  std::cout << std::setw(PRINT_WIDTH) << text;
} // printText()

// header line
inline void printHeader(vtkm::Id howMany)
{ // printHeader()
  // print out a separating bar
  printSeparatingBar(howMany);
  // print out a label
  printLabel("ID");
  // print out the ID numbers
  for (vtkm::Id entry = 0; entry < howMany; entry++)
  {
    printIndexType(entry);
  }
  // and an std::endl
  std::cout << std::endl;
  // print out another separating bar
  printSeparatingBar(howMany);
} // printHeader()


// base routines for reading & writing host vectors
template <typename T, typename StorageType>
inline void printValues(std::string label,
                        const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                        vtkm::Id nValues)
{ // printValues()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = dVec.GetNumberOfValues();
  }

  // print the label
  printLabel(label);

  // now print the data
  auto portal = dVec.GetPortalConstControl();
  for (vtkm::Id entry = 0; entry < nValues; entry++)
  {
    printDataType(portal.Get(entry));
  }
  // and an std::endl
  std::cout << std::endl;
} // printValues()

// base routines for reading & writing host vectors
template <typename T, typename StorageType>
inline void printSortedValues(std::string label,
                              const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                              IdArrayType& sortVec,
                              vtkm::Id nValues)
{ // printValues()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = sortVec.GetNumberOfValues();
  }

  // print the label
  printLabel(label);

  // now print the data
  auto dportal = dVec.GetPortalConstControl();
  auto sortPortal = sortVec.GetPortalConstControl();
  for (vtkm::Id entry = 0; entry < nValues; entry++)
  {
    printDataType(dportal.Get(sortPortal.Get(entry)));
  }
  // and an std::endl
  std::cout << std::endl;
} // printValues()


inline void printIndices(std::string label,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& iVec,
                         vtkm::Id nIndices)
{ // printIndices()
  // -1 means full size
  if (nIndices == -1)
  {
    nIndices = iVec.GetNumberOfValues();
  }

  // print the label
  printLabel(label);

  auto portal = iVec.GetPortalConstControl();
  for (vtkm::Id entry = 0; entry < nIndices; entry++)
    printIndexType(portal.Get(entry));

  // and the std::endl
  std::cout << std::endl;
} // printIndices()

// routines for printing indices & data in blocks
template <typename T, typename StorageType>
inline void printNakedDataBlock(const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                                vtkm::Id nColumns)
{ // printNakedDataBlock()
  // loop control variable
  vtkm::Id entry = 0;
  // per row
  for (vtkm::Id row = 0; entry < dVec.GetNumberOfValues(); row++)
  { // per row
    // now print the data
    auto portal = dVec.GetPortalConstControl();
    for (vtkm::Id col = 0; col < nColumns; col++, entry++)
    {
      printIndexType(portal.Get(entry));
    }
    std::cout << std::endl;
  } // per row
  // and a final std::endl
  std::cout << std::endl;
} // printNakedDataBlock()

inline void printNakedIndexBlock(IdArrayType& iVec, vtkm::Id nColumns)
{ // printNakedIndexBlock()
  // loop control variable
  vtkm::Id entry = 0;
  // per row
  for (vtkm::Id row = 0; entry < iVec.GetNumberOfValues(); row++)
  { // per row
    // now print the data
    auto portal = iVec.GetPortalConstControl();
    for (vtkm::Id col = 0; col < nColumns; col++, entry++)
    {
      printIndexType(portal.Get(entry));
    }
    std::cout << std::endl;
  } // per row
  // and a final std::endl
  std::cout << std::endl;
} // printNakedIndexBlock()


template <typename T, typename StorageType>
inline void printLabelledDataBlock(std::string label,
                                   const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                                   vtkm::Id nColumns)
{ // printLabelledDataBlock()
  // start with a header
  printHeader(nColumns);
  // loop control variable
  vtkm::Id entry = 0;
  // per row
  auto portal = dVec.GetPortalConstControl();
  for (vtkm::Id row = 0; entry < portal.GetNumberOfValues(); row++)
  { // per row
    printLabel(label + "[" + std::to_string(row) + "]");
    // now print the data
    for (vtkm::Id col = 0; col < nColumns; col++, entry++)
    {
      printDataType(portal.Get(entry));
    }
    std::cout << std::endl;
  } // per row
  // and a final std::endl
  std::cout << std::endl;
} // printLabelledDataBlock()



inline void printLabelledIndexBlock(std::string label, IdArrayType& iVec, vtkm::Id nColumns)
{ // printLabelledIndexBlock()
  // start with a header
  printHeader(nColumns);
  // loop control variable
  vtkm::Id entry = 0;
  // per row
  auto portal = iVec.GetPortalConstControl();
  for (vtkm::Id row = 0; entry < portal.GetNumberOfValues(); row++)
  { // per row
    printLabel(label + "[" + std::to_string(row) + "]");
    // now print the data
    for (vtkm::Id col = 0; col < nColumns; col++, entry++)
    {
      printIndexType(portal.Get(entry));
    }
    std::cout << std::endl;
  } // per row
  // and a final std::endl
  std::cout << std::endl;
} // printLabelledIndexBlock()


// routine for printing one element per line for diffing
inline void printByLine(IdArrayType& block)
{ // printByLine()
  auto portal = block.GetPortalConstControl();
  for (vtkm::Id entry = 0; entry < block.GetNumberOfValues(); entry++)
  { // per entry
    if (noSuchElement(portal.Get(entry)))
    {
      std::cout << std::to_string(-1L) << "\n";
    }
    else
    {
      std::cout << portal.Get(entry) << "\n";
    }
  } // per entry
} // printByLine()


// routine for printing list of edge pairs. Used, e.g., to print the sorted list of saddle peaks from the ContourTree
inline void printEdgePairArray(const EdgePairArray& edgePairArray)
{ // printEdgePairArray()
  // now print them out
  auto edgePairArrayConstPortal = edgePairArray.GetPortalConstControl();
  for (vtkm::Id superarc = 0; superarc < edgePairArray.GetNumberOfValues(); superarc++)
  { // per superarc
    std::cout << std::right << std::setw(PRINT_WIDTH)
              << edgePairArrayConstPortal.Get(superarc).first << " ";
    std::cout << std::right << std::setw(PRINT_WIDTH)
              << edgePairArrayConstPortal.Get(superarc).second << std::endl;
  } // per superarc
} // printEdgePairArray()


} // namespace contourtree_augmented
} // worklet
} // vtkm

// tail of include guard
#endif
