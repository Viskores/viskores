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

#ifndef vtkm_worklet_contourtree_augmented_meshextrema_h
#define vtkm_worklet_contourtree_augmented_meshextrema_h

#include <iomanip>

// local includes
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/contourtree_augmented/PointerDoubling.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/meshextrema/SetStarts.h>


#include <vtkm/worklet/contourtree_augmented/mesh_dem/SortIndices.h>

namespace mesh_extrema_inc_ns = vtkm::worklet::contourtree_augmented::mesh_extrema_inc;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

class MeshExtrema
{ // MeshExtrema
public:
  vtkm::cont::Invoker Invoke;
  // arrays for peaks & pits
  IdArrayType peaks;
  IdArrayType pits;
  vtkm::Id nVertices;
  vtkm::Id nLogSteps;

  // constructor
  VTKM_CONT
  MeshExtrema(vtkm::Id meshSize);

  // routine to initialise the array before chaining
  template <class MeshType>
  void SetStarts(MeshType& mesh, bool isMaximal);

  // routine that computes regular chains in a merge tree
  VTKM_CONT
  void BuildRegularChains(bool isMaximal);

  // debug routine
  VTKM_CONT
  void DebugPrint(const char* message, const char* fileName, long lineNum);

}; // MeshExtrema


inline MeshExtrema::MeshExtrema(vtkm::Id meshSize)
  : peaks()
  , pits()
  , nVertices(meshSize)
  , nLogSteps(0)
{ // MeshExrema
  // Compute the number of log steps required in this pass
  nLogSteps = 1;
  for (vtkm::Id shifter = nVertices; shifter != 0; shifter >>= 1)
    nLogSteps++;

  // Allocate memory for the peaks and pits
  peaks.Allocate(nVertices);
  pits.Allocate(nVertices);
  // TODO Check if we really need to set the peaks and pits to zero or whether it is enough to allocate them
  vtkm::cont::ArrayHandleConstant<vtkm::Id> constZeroArray(0, nVertices);
  vtkm::cont::Algorithm::Copy(constZeroArray, peaks);
  vtkm::cont::Algorithm::Copy(constZeroArray, pits);
} // MeshExtrema


inline void MeshExtrema::BuildRegularChains(bool isMaximal)
{ // BuildRegularChains()
  // Create vertex index array -- note, this is a fancy vtk-m array, i.e, the full array
  // is not actually allocated but the array only acts like a sequence of numbers
  vtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  IdArrayType& extrema = isMaximal ? peaks : pits;

  // Create the PointerDoubling worklet and corresponding dispatcher
  vtkm::worklet::contourtree_augmented::PointerDoubling pointerDoubler;

  // Iterate to perform pointer-doubling to build chains to extrema (i.e., maxima or minima)
  // depending on whether we are computing a JoinTree or a SplitTree
  for (vtkm::Id logStep = 0; logStep < this->nLogSteps; logStep++)
  {
    this->Invoke(pointerDoubler,
                 vertexIndexArray, // input
                 extrema);         // output. Update whole extrema array during pointer doubling
  }
  DebugPrint("Regular Chains Built", __FILE__, __LINE__);
} // BuildRegularChains()

template <class MeshType>
inline void MeshExtrema::SetStarts(MeshType& mesh, bool isMaximal)
{
  mesh.setPrepareForExecutionBehavior(isMaximal);
  mesh_extrema_inc_ns::SetStarts setStartsWorklet;
  vtkm::cont::ArrayHandleIndex sortIndexArray(mesh.nVertices);
  if (isMaximal)
  {
    this->Invoke(setStartsWorklet, sortIndexArray, mesh, peaks);
  }
  else
  {
    this->Invoke(setStartsWorklet, sortIndexArray, mesh, pits);
  }
  DebugPrint("Regular Starts Set", __FILE__, __LINE__);
}


// debug routine
inline void MeshExtrema::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Mesh Extrema Contain:      " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  printHeader(peaks.GetNumberOfValues());
  printIndices("Peaks", peaks);
  printIndices("Pits", pits);
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
