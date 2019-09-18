//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

#ifndef vtkm_worklet_contourtree_print_vector_h
#define vtkm_worklet_contourtree_print_vector_h

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree
{

// debug value for number of columns to print
#define PRINT_COLS 10
#define PRINT_WIDTH 12
#define PREFIX_WIDTH 20

// utility routine to convert number to a string
std::string NumString(vtkm::Id number);

// base routines for printing label & prefix bars
void printLabel(std::string label);
void printSeparatingBar(vtkm::Id howMany);

// routines to print out a single value
template <typename T>
void printDataType(T value);
void printIndexType(vtkm::Id value);

// header line
void printHeader(vtkm::Id howMany);

// base routines for reading & writing host vectors
template <typename T, typename StorageType>
void printValues(std::string label,
                 vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                 vtkm::Id nValues = -1);
void printIndices(std::string label,
                  vtkm::cont::ArrayHandle<vtkm::Id>& iVec,
                  vtkm::Id nIndices = -1);

// routines for printing indices & data in blocks
template <typename T, typename StorageType>
void printLabelledBlock(std::string label,
                        const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                        vtkm::Id nRows,
                        vtkm::Id nColumns);

// utility routine to convert number to a string
inline std::string NumString(vtkm::Id number)
{ // NumString()
  char strBuf[20];
  sprintf(strBuf, "%1d", (int)number);
  return std::string(strBuf);
} // NumString()

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
    std::cout << std::setw(PRINT_WIDTH) << std::setfill('-') << "";
  // now the endl, resetting the fill character
  std::cout << std::setfill(' ') << std::endl;
} // printSeparatingBar()

// routine to print out a single value
template <typename T>
void printDataType(T value)
{ // printDataType
  std::cout << std::setw(PRINT_WIDTH) << value;
} // printDataType

// routine to print out a single value
inline void printIndexType(vtkm::Id value)
{ // printIndexType
  std::cout << std::setw(PRINT_WIDTH) << value;
} // printIndexType

// header line
inline void printHeader(vtkm::Id howMany)
{ // printHeader()
  if (howMany > PRINT_COLS)
    howMany = PRINT_COLS;
  // print out a separating bar
  printSeparatingBar(howMany);
  // print out a label
  printLabel("ID");
  // print out the ID numbers
  for (vtkm::Id entry = 0; entry < howMany; entry++)
    printIndexType(entry);
  // and an endl
  std::cout << std::endl;
  // print out another separating bar
  printSeparatingBar(howMany);
} // printHeader()

// base routines for reading & writing host vectors
template <typename T, typename StorageType>
void printValues(std::string label, vtkm::cont::ArrayHandle<T, StorageType>& dVec, vtkm::Id nValues)
{
  // -1 means full size
  if (nValues == -1)
  {
    nValues = dVec.GetNumberOfValues();
  }

  if (nValues > PRINT_COLS)
  {
    nValues = PRINT_COLS;
  }

  // print the label
  printLabel(label);

  // now print the data
  const auto dVecPortal = dVec.GetPortalConstControl();
  for (vtkm::Id entry = 0; entry < nValues; entry++)
  {
    printDataType(dVecPortal.Get(entry));
  }

  // and an endl
  std::cout << std::endl;
} // printValues()

// base routines for reading & writing host vectors
inline void printIndices(std::string label,
                         vtkm::cont::ArrayHandle<vtkm::Id>& iVec,
                         vtkm::Id nIndices)
{
  // -1 means full size
  if (nIndices == -1)
  {
    nIndices = iVec.GetNumberOfValues();
  }

  if (nIndices > PRINT_COLS)
  {
    nIndices = PRINT_COLS;
  }

  // print the label
  printLabel(label);

  // now print the data
  const auto iVecPortal = iVec.GetPortalConstControl();
  for (vtkm::Id entry = 0; entry < nIndices; entry++)
  {
    printIndexType(iVecPortal.Get(entry));
  }

  // and an endl
  std::cout << std::endl;
} // printIndices()

template <typename T, typename StorageType>
void printLabelledBlock(std::string label,
                        const vtkm::cont::ArrayHandle<T, StorageType>& dVec,
                        vtkm::Id nRows,
                        vtkm::Id nColumns)
{
  if (nRows > PRINT_COLS)
  {
    nRows = PRINT_COLS;
  }

  if (nColumns > PRINT_COLS)
  {
    nColumns = PRINT_COLS;
  }

  // start with a header
  printHeader(nColumns);
  // loop control variable
  vtkm::Id entry = 0;
  // per row
  const auto dVecPortal = dVec.GetPortalConstControl();
  for (vtkm::Id row = 0; row < nRows; row++)
  { // per row
    printLabel(label + "[" + NumString(row) + "]");
    // now print the data
    for (vtkm::Id col = 0; col < nColumns; col++, entry++)
    {
      printDataType(dVecPortal.Get(entry));
    }
    std::cout << std::endl;
  } // per row
  std::cout << std::endl;
} // printLabelledBlock()
}
}
}

#endif
