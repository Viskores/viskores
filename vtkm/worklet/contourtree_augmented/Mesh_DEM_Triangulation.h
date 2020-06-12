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
//
//      Parallel Peak Pruning v. 2.0
//
// Mesh_2D_DEM_Triangulation.h - a 2D regular mesh
//
//==============================================================================
//
// COMMENTS:
//
// This is an abstraction to separate out the mesh from the graph algorithm
// that we will be executing.
//
// In this version, we will sort the values up front, and then keep track of
// them using indices only, without looking up their values. This should
// simplify several parts of code significantly, and reduce the memory bandwidth.
// Of course, in moving to 64-bit indices, we will not necessarily see gains.
//
//==============================================================================



#ifndef vtk_m_worklet_contourtree_augmented_mesh_dem_triangulation_h
#define vtk_m_worklet_contourtree_augmented_mesh_dem_triangulation_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/SimulatedSimplicityComperator.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/SortIndices.h>


//Define namespace alias for the freudenthal types to make the code a bit more readable
namespace mesh_dem_ns = vtkm::worklet::contourtree_augmented::mesh_dem;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

template <typename T, typename StorageType>
class Mesh_DEM_Triangulation
{
public:
  // common mesh size parameters
  vtkm::Id NumVertices, NumLogSteps;

  // Define dimensionality of the mesh
  vtkm::Id NumDims;

  // Array with the sorted order of the mesh vertices
  IdArrayType SortOrder;

  // Array with the sort index for each vertex
  // i.e. the inverse permutation for SortOrder
  IdArrayType SortIndices;

  //empty constructor
  Mesh_DEM_Triangulation()
    : NumVertices(0)
    , NumLogSteps(0)
    , NumDims(2)
  {
  }

  // Getter function for NumVertices
  vtkm::Id GetNumberOfVertices() const { return this->NumVertices; }

  // sorts the data and initializes the sortIndex & indexReverse
  void SortData(const vtkm::cont::ArrayHandle<T, StorageType>& values);

  //routine that dumps out the contents of the mesh
  void DebugPrint(const char* message, const char* fileName, long lineNum);

protected:
  virtual void DebugPrintExtends() = 0;
  virtual void DebugPrintValues(const vtkm::cont::ArrayHandle<T, StorageType>& values) = 0;
}; // class Mesh_DEM_Triangulation

template <typename T, typename StorageType>
class Mesh_DEM_Triangulation_2D : public Mesh_DEM_Triangulation<T, StorageType>
{
public:
  // 2D mesh size parameters
  vtkm::Id NumColumns, NumRows;

  // Maximum outdegree
  static constexpr int MAX_OUTDEGREE = 3;

  // empty constructor
  Mesh_DEM_Triangulation_2D()
    : Mesh_DEM_Triangulation<T, StorageType>()
    , NumColumns(0)
    , NumRows(0)
  {
    this->NumDims = 2;
  }

  // base constructor
  Mesh_DEM_Triangulation_2D(vtkm::Id ncols, vtkm::Id nrows)
    : Mesh_DEM_Triangulation<T, StorageType>()
    , NumColumns(ncols)
    , NumRows(nrows)
  {
    this->NumDims = 2;
    this->NumVertices = NumRows * NumColumns;

    // compute the number of log-jumping steps (i.e. lg_2 (NumVertices))
    this->NumLogSteps = 1;
    for (vtkm::Id shifter = this->NumVertices; shifter > 0; shifter >>= 1)
      this->NumLogSteps++;
  }

protected:
  virtual void DebugPrintExtends();
  virtual void DebugPrintValues(const vtkm::cont::ArrayHandle<T, StorageType>& values);
}; // class Mesh_DEM_Triangulation_2D

template <typename T, typename StorageType>
class Mesh_DEM_Triangulation_3D : public Mesh_DEM_Triangulation<T, StorageType>
{
public:
  // 2D mesh size parameters
  vtkm::Id NumColumns, NumRows, NumSlices;

  // Maximum outdegree
  static constexpr int MAX_OUTDEGREE = 6; // True for Freudenthal and Marching Cubes

  // empty constructor
  Mesh_DEM_Triangulation_3D()
    : Mesh_DEM_Triangulation<T, StorageType>()
    , NumColumns(0)
    , NumRows(0)
    , NumSlices(0)
  {
    this->NumDims = 3;
  }

  // base constructor
  Mesh_DEM_Triangulation_3D(vtkm::Id ncols, vtkm::Id nrows, vtkm::Id nslices)
    : Mesh_DEM_Triangulation<T, StorageType>()
    , NumColumns(ncols)
    , NumRows(nrows)
    , NumSlices(nslices)
  {
    this->NumDims = 3;
    this->NumVertices = NumRows * NumColumns * NumSlices;

    // compute the number of log-jumping steps (i.e. lg_2 (NumVertices))
    this->NumLogSteps = 1;
    for (vtkm::Id shifter = this->NumVertices; shifter > 0; shifter >>= 1)
      this->NumLogSteps++;
  }

protected:
  virtual void DebugPrintExtends();
  virtual void DebugPrintValues(const vtkm::cont::ArrayHandle<T, StorageType>& values);
}; // class Mesh_DEM_Triangulation_3D


// sorts the data and initialises the SortIndices & SortOrder
template <typename T, typename StorageType>
void Mesh_DEM_Triangulation<T, StorageType>::SortData(
  const vtkm::cont::ArrayHandle<T, StorageType>& values)
{
  // Define namespace alias for mesh dem worklets
  namespace mesh_dem_worklets = vtkm::worklet::contourtree_augmented::mesh_dem;

  // Make sure that the values have the correct size
  assert(values.GetNumberOfValues() == NumVertices);

  // Just in case, make sure that everything is cleaned up
  SortIndices.ReleaseResources();
  SortOrder.ReleaseResources();

  // allocate memory for the sort arrays
  SortOrder.Allocate(NumVertices);
  SortIndices.Allocate(NumVertices);

  // now sort the sort order vector by the values, i.e,. initialize the SortOrder member variable
  vtkm::cont::ArrayHandleIndex initVertexIds(NumVertices); // create sequence 0, 1, .. NumVertices
  vtkm::cont::ArrayCopy(initVertexIds, SortOrder);

  vtkm::cont::Algorithm::Sort(SortOrder,
                              mesh_dem::SimulatedSimplicityIndexComparator<T, StorageType>(values));

  // now set the index lookup, i.e., initialize the SortIndices member variable
  // In serial this would be
  //  for (indexType vertex = 0; vertex < NumVertices; vertex++)
  //            SortIndices[SortOrder[vertex]] = vertex;
  mesh_dem_worklets::SortIndices sortIndicesWorklet;
  vtkm::cont::Invoker invoke;
  invoke(sortIndicesWorklet, SortOrder, SortIndices);

  // Debug print statement
  DebugPrint("Data Sorted", __FILE__, __LINE__);
  DebugPrintValues(values);
} // SortData()


template <typename T, typename StorageType>
void Mesh_DEM_Triangulation<T, StorageType>::DebugPrint(const char* message,
                                                        const char* fileName,
                                                        long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "------------------------------------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Mesh Contains:                                        " << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;
  //DebugPrintExtents();
  PrintLabel("NumVertices");
  PrintIndexType(NumVertices);
  std::cout << std::endl;
  PrintLabel("NumLogSteps");
  PrintIndexType(this->NumLogSteps);
  std::cout << std::endl;
  PrintIndices("Sort Indices", SortIndices);
  PrintIndices("Sort Order", SortOrder);
  std::cout << std::endl;
#else
  // Avoid unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()

// print mesh extends for 2D mesh
template <typename T, typename StorageType>
void Mesh_DEM_Triangulation_2D<T, StorageType>::DebugPrintExtends()
{
  PrintLabel("NumRows");
  PrintIndexType(NumRows);
  std::cout << std::endl;
  PrintLabel("NumColumns");
  PrintIndexType(NumColumns);
  std::cout << std::endl;
} // DebugPrintExtends for 2D

// print mesh extends for 3D mesh
template <typename T, typename StorageType>
void Mesh_DEM_Triangulation_3D<T, StorageType>::DebugPrintExtends()
{
  PrintLabel("NumRows");
  PrintIndexType(NumRows);
  std::cout << std::endl;
  PrintLabel("NumColumns");
  PrintIndexType(NumColumns);
  std::cout << std::endl;
  PrintLabel("NumSlices");
  PrintIndexType(NumSlices);
  std::cout << std::endl;
}

template <typename T, typename StorageType>
void Mesh_DEM_Triangulation_2D<T, StorageType>::DebugPrintValues(
  const vtkm::cont::ArrayHandle<T, StorageType>& values)
{
#ifdef DEBUG_PRINT
  if (NumColumns > 0)
  {
    PrintLabelledDataBlock<T, StorageType>("Value", values, NumColumns);
    PrintSortedValues("Sorted Values", values, this->SortOrder);
  }
  PrintHeader(values.GetNumberOfValues());
#else
  // Avoid unused parameter warning
  (void)values;
#endif
} // DebugPrintValues

template <typename T, typename StorageType>
void Mesh_DEM_Triangulation_3D<T, StorageType>::DebugPrintValues(
  const vtkm::cont::ArrayHandle<T, StorageType>& values)
{
#ifdef DEBUG_PRINT
  if (NumColumns > 0)
  {
    PrintLabelledDataBlock<T, StorageType>("Value", values, NumColumns);
  }
  PrintHeader(values.GetNumberOfValues());
#else
  // Avoid unused parameter warning
  (void)values;
#endif
} // DebugPrintValues

} // namespace contourtree_augmented
} // worklet
} // vtkm

#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/Freudenthal_2D_Triangulation.h> // include Mesh_DEM_Triangulation_2D_Freudenthal
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/Freudenthal_3D_Triangulation.h> // include Mesh_DEM_Triangulation_3D_Freudenthal
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/MarchingCubes_3D_Triangulation.h> // include Mesh_DEM_Triangulation_3D_MarchinCubes

#endif
