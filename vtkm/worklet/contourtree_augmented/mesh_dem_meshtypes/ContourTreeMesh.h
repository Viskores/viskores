//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_contour_tree_mesh_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_contour_tree_mesh_h

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/MeshBoundary.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/MeshStructureContourTreeMesh.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/ArcComparator.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/CombinedOtherStartIndexNNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/CombinedSimulatedSimplicityIndexComparator.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/CombinedVector.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/CombinedVectorDifferentFromNext.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/CompressNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/ComputeMaxNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/FindStartIndexWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/InitToCombinedSortOrderArraysWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/MergeCombinedOtherStartIndexWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/ReplaceArcNumWithToVertexWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/SubtractAssignWorklet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/contourtreemesh/UpdateCombinedNeighboursWorklet.h>

#include <vtkm/worklet/contourtree_augmented/PrintVectors.h> // TODO remove should not be needed

#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/cont/Invoker.h>

#include <algorithm>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace contourtree_mesh_inc_ns =
  vtkm::worklet::contourtree_augmented::mesh_dem_contourtree_mesh_inc;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

template <typename FieldType>
class ContourTreeMesh : public vtkm::cont::ExecutionObjectBase
{ // class ContourTreeMesh
public:
  //Mesh dependent helper functions
  void setPrepareForExecutionBehavior(bool getMax);

  template <typename DeviceTag>
  contourtree_mesh_inc_ns::MeshStructureContourTreeMesh<DeviceTag> PrepareForExecution(
    DeviceTag) const;

  ContourTreeMesh() {}

  // Constructor
  ContourTreeMesh(const IdArrayType& arcs,
                  const IdArrayType& inSortOrder,
                  const vtkm::cont::ArrayHandle<FieldType>& values,
                  const IdArrayType& inGlobalMeshIndex);

  // Constructor
  ContourTreeMesh(const IdArrayType& nodes,
                  const IdArrayType& arcs,
                  const IdArrayType& inSortOrder,
                  const vtkm::cont::ArrayHandle<FieldType>& values,
                  const IdArrayType& inGlobalMeshIndex);

  //  Construct a ContourTreeMesh from nodes/arcs and another ContourTreeMesh (instead of a Mesh_DEM_Triangulation)
  //     nodes/arcs: From the contour tree
  //     ContourTreeMesh: the contour tree mesh used to compute the contour tree described by nodes/arcs
  ContourTreeMesh(const IdArrayType& nodes,
                  const IdArrayType& arcs,
                  const ContourTreeMesh<FieldType>& mesh);

  // Initalize contour tree mesh from mesh and arcs. For fully augmented contour tree with all
  // mesh vertices as nodes. Same as using { 0, 1, ..., nodes.size()-1 } as nodes for the
  // ContourTreeMeshh(nodes, arcsm mesh) constructor above
  ContourTreeMesh(const IdArrayType& arcs, const ContourTreeMesh<FieldType>& mesh);

  // Load contour tree mesh from file
  ContourTreeMesh(const char* filename)
  {
    load(filename);
    this->nVertices = this->sortedValues.GetNumberOfValues();
  }

  vtkm::Id GetNumberOfVertices() const { return this->nVertices; }

  // Combine two ContourTreeMeshes
  template <typename DeviceTag>
  void mergeWith(ContourTreeMesh<FieldType>& other);

  // Save/load the mesh helpers
  void save(const char* filename) const;
  void load(const char* filename);

  // Empty placeholder function to ensure compliance of this class with the interface
  // the other mesh classes. This is a no-op here since this class is initalized
  // from a known contour tree so sort is already done
  template <typename T, typename StorageType>
  void SortData(const vtkm::cont::ArrayHandle<T, StorageType>& values) const
  {
    (void)values; // Do nothink but avoid unsused param warning
  }

  // Public fields
  static const int MAX_OUTDEGREE = 20;

  vtkm::Id nVertices;
  // TODO we should be able to remove this one, but we need to figure out what we need to return in the worklet instead
  IdArrayType sortOrder;
  vtkm::cont::ArrayHandle<FieldType> sortedValues;
  IdArrayType globalMeshIndex;
  // neighbours stores for each vertex the indices of its neighbours. For each vertex
  // the indices are sorted by value, i.e, the first neighbour has the lowest and
  // the last neighbour the highest value for the vertex. In the array we just
  // concatinate the list of neighbours from all vertices, i.e., we first
  // have the list of neighbours of the first vertex, then the second vertex and so on, i.e.:
  // [ n_1_1, n_1_2, n_2_1, n_2_2, n_2_3, etc.]
  IdArrayType neighbours;
  // firstNeighour gives us for each vertex an index into the neighours array indicating
  // the index where the list of neighbours for the vertex begins
  IdArrayType firstNeighbour;
  // the maximum number of neighbours of a vertex
  vtkm::Id maxNeighbours;

  // Debug print routine
  void DebugPrint(const char* message, const char* fileName, long lineNum);

  // Get boundary execution object
  MeshBoundaryContourTreeMeshExec GetMeshBoundaryExecutionObject(vtkm::Id totalNRows,
                                                                 vtkm::Id totalNCols,
                                                                 vtkm::Id3 minIdx,
                                                                 vtkm::Id3 maxIdx) const;

private:
  vtkm::cont::Invoker Invoke;

  bool mGetMax; // Define the behavior for the PrepareForExecution function

  // Private init and helper functions
  void InitialiseNeighboursFromArcs(const IdArrayType& arcs);
  void ComputeNNeighboursVector(IdArrayType& nNeighbours) const;
  void ComputeMaxNeighbours();

  // Private helper functions for saving data vectors
  template <typename ValueType>
  void saveVector(std::ostream& os, const vtkm::cont::ArrayHandle<ValueType>& vec)
    const; // Internal helper function to save 1D index array to file
  template <typename ValueType>
  void loadVector(std::istream& is,
                  const vtkm::cont::ArrayHandle<ValueType>&
                    vec); // Internal helper function to load 1D index array from file


}; // ContourTreeMesh


// debug routine
template <typename FieldType>
void ContourTreeMesh<FieldType>::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Contour Tree Mesh Contains:     " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  printHeader(this->nVertices);
  printIndices("sortOrder", sortOrder);
  printValues("sortedValues", sortedValues);
  printIndices("globalMeshIndex", globalMeshIndex);
  printIndices("neighbours", neighbours);
  printIndices("firstNeighbour", firstNeighbour);
  std::cout << "maxNeighbours=" << maxNeighbours << std::endl;
  std::cout << "mGetMax=" << mGetMax << std::endl;

#else
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()




// create the contour tree mesh from contour tree data
template <typename FieldType>
ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& arcs,
                                            const IdArrayType& inSortOrder,
                                            const vtkm::cont::ArrayHandle<FieldType>& values,
                                            const IdArrayType& inGlobalMeshIndex)
  : sortOrder(inSortOrder)
  , sortedValues()
  , globalMeshIndex(inGlobalMeshIndex)
  , neighbours()
  , firstNeighbour()
{
  this->nVertices = this->sortOrder.GetNumberOfValues();
  // values permuted by sortOrder to sort the values
  auto permutedValues = vtkm::cont::make_ArrayHandlePermutation(this->sortOrder, values);
  // TODO check if we actually need to make this copy here. we could just store the permutedValues array to save memory
  vtkm::cont::Algorithm::Copy(permutedValues, this->sortedValues);
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& nodes,
                                            const IdArrayType& arcs,
                                            const IdArrayType& inSortOrder,
                                            const vtkm::cont::ArrayHandle<FieldType>& values,
                                            const IdArrayType& inGlobalMeshIndex)
  : globalMeshIndex(inGlobalMeshIndex)
  , neighbours()
  , firstNeighbour()
{
  // Initialize the sortedValues array the values permutted by the sortOrder permutted by the nodes, i.e.,
  // this->sortedValues[v] = values[inSortOrder[nodes[v]]];
  vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedSortOrder(nodes,
                                                                                 inSortOrder);
  auto permutedValues = vtkm::cont::make_ArrayHandlePermutation(permutedSortOrder, values);
  vtkm::cont::Algorithm::Copy(permutedValues, this->sortedValues);
  vtkm::cont::Algorithm::Copy(
    permutedSortOrder,
    this
      ->sortOrder); // TODO Check if the sortOrder needs to be set form the input or the permutted sortOrder
  this->nVertices = this->sortedValues.GetNumberOfValues();
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}

template <typename FieldType>
ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& arcs,
                                            const ContourTreeMesh<FieldType>& mesh)
  : sortOrder(mesh.sortOrder)
  , sortedValues(mesh.sortedValues)
  , globalMeshIndex(mesh.globalMeshIndex)
  , neighbours()
  , firstNeighbour()
{
  this->nVertices = this->sortedValues.GetNumberOfValues();
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& nodes,
                                            const IdArrayType& arcs,
                                            const ContourTreeMesh<FieldType>& mesh)
  : sortOrder(mesh.sortOrder)
  , neighbours()
  , firstNeighbour()
{
  // Initatlize the global mesh index with the globalMeshIndex permutted by the nodes
  vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedGlobalMeshIndex(
    nodes, mesh.globalMeshIndex);
  vtkm::cont::Algorithm::Copy(permutedGlobalMeshIndex, this->globalMeshIndex);
  // Initialize the sortedValues array with the sortedValues permutted by the nodes
  auto permutedSortedValues = vtkm::cont::make_ArrayHandlePermutation(nodes, mesh.sortedValues);
  vtkm::cont::Algorithm::Copy(permutedSortedValues, this->sortedValues);
  // Initialize the neighbours from the arcs
  this->nVertices = this->sortedValues.GetNumberOfValues();
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


// Initalize the contour tree from the arcs array and sort order
template <typename FieldType>
void ContourTreeMesh<FieldType>::InitialiseNeighboursFromArcs(const IdArrayType& arcs)
{
  // Find target indices for valid arcs in neighbours array ...
  IdArrayType arcTargetIndex;
  arcTargetIndex.Allocate(arcs.GetNumberOfValues());
  oneIfArcValid oneIfArcValidFunctor;
  auto oneIfArcValidArrayHandle =
    vtkm::cont::ArrayHandleTransform<IdArrayType, oneIfArcValid>(arcs, oneIfArcValidFunctor);
  vtkm::cont::Algorithm::ScanExclusive(oneIfArcValidArrayHandle, arcTargetIndex);
  vtkm::Id nValidArcs =
    arcTargetIndex.GetPortalConstControl().Get(arcTargetIndex.GetNumberOfValues() - 1) +
    oneIfArcValidFunctor(arcs.GetPortalConstControl().Get(arcs.GetNumberOfValues() - 1));

  // ... and compress array
  this->neighbours.ReleaseResources();
  this->neighbours.Allocate(2 * nValidArcs);

  contourtree_mesh_inc_ns::CompressNeighboursWorklet compressNeighboursWorklet;
  this->Invoke(compressNeighboursWorklet, arcs, arcTargetIndex, this->neighbours);

  // Sort arcs so that all arcs from the same vertex are adjacent. All arcs are in neighbours array based on
  // sort index of their 'from' vertex (and then within a run sorted by sort index of their 'to' vertex).
  vtkm::cont::Algorithm::Sort(this->neighbours, contourtree_mesh_inc_ns::ArcComparator(arcs));

  // Find start index for each vertex into neighbours array
  this->firstNeighbour.Allocate(this->nVertices);

  contourtree_mesh_inc_ns::FindStartIndexWorklet findStartIndexWorklet;
  this->Invoke(findStartIndexWorklet,
               this->neighbours,
               arcs,
               this->firstNeighbour // output
               );


  // Replace arc number with 'to' vertex in neighbours array
  contourtree_mesh_inc_ns::ReplaceArcNumWithToVertexWorklet replaceArcNumWithToVertexWorklet;
  this->Invoke(replaceArcNumWithToVertexWorklet,
               this->neighbours, // input/output
               arcs              // input
               );

  // Compute maximum number of neighbours
  this->ComputeMaxNeighbours();

#ifdef DEBUG_PRINT
  std::cout << std::setw(30) << std::left << __FILE__ << ":" << std::right << std::setw(4)
            << __LINE__ << std::endl;
  auto firstNeighbourPortal = this->firstNeighbour.GetPortalConstControl();
  auto neighboursPortal = this->neighbours.GetPortalConstControl();
  for (vtkm::Id vtx = 0; vtx < firstNeighbour.GetNumberOfValues(); ++vtx)
  {
    std::cout << vtx << ": ";
    vtkm::Id neighboursBeginIndex = firstNeighbourPortal.Get(vtx);
    vtkm::Id neighboursEndIndex = (vtx < this->nVertices - 1) ? firstNeighbourPortal.Get(vtx + 1)
                                                              : neighbours.GetNumberOfValues();

    for (vtkm::Id ni = neighboursBeginIndex; ni < neighboursEndIndex; ++ni)
    {
      std::cout << neighboursPortal.Get(ni) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "Max neighbours: " << this->maxNeighbours << std::endl;
#endif
}

template <typename FieldType>
void ContourTreeMesh<FieldType>::ComputeNNeighboursVector(IdArrayType& nNeighbours) const
{
  nNeighbours.Allocate(this->firstNeighbour.GetNumberOfValues()); // same as this->nVertices
  contourtree_mesh_inc_ns::ComputeMaxNeighboursWorklet computeMaxNeighboursWorklet(
    this->neighbours.GetNumberOfValues());
  this->Invoke(computeMaxNeighboursWorklet, this->firstNeighbour, nNeighbours);
}

template <typename FieldType>
void ContourTreeMesh<FieldType>::ComputeMaxNeighbours()
{
  // Compute maximum number of neighbours
  IdArrayType nNeighbours;
  this->ComputeNNeighboursVector(nNeighbours);
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray = vtkm::cont::ArrayRangeCompute(nNeighbours);
  this->maxNeighbours = static_cast<vtkm::Id>(rangeArray.GetPortalConstControl().Get(0).Max);
}

// Define the behavior for the execution object generate by the PrepareForExecution function
template <typename FieldType>
void ContourTreeMesh<FieldType>::setPrepareForExecutionBehavior(bool getMax)
{
  this->mGetMax = getMax;
}

// Get VTKM execution object that represents the structure of the mesh and provides the mesh helper functions on the device
template <typename FieldType>
template <typename DeviceTag>
contourtree_mesh_inc_ns::MeshStructureContourTreeMesh<DeviceTag>
  ContourTreeMesh<FieldType>::PrepareForExecution(DeviceTag) const
{
  return contourtree_mesh_inc_ns::MeshStructureContourTreeMesh<DeviceTag>(
    this->neighbours, this->firstNeighbour, this->maxNeighbours, this->mGetMax);
}

struct NotNoSuchElement
{
  VTKM_EXEC_CONT bool operator()(vtkm::Id x) const { return x != NO_SUCH_ELEMENT; }
};

// Combine two ContourTreeMeshes
template <typename FieldType>
template <typename DeviceTag>
void ContourTreeMesh<FieldType>::mergeWith(ContourTreeMesh<FieldType>& other)
{
#ifdef DEBUG_PRINT
  this->DebugPrint("THIS ContourTreeMesh", __FILE__, __LINE__);
  other.DebugPrint("OTHER ContourTreeMesh", __FILE__, __LINE__);
#endif

  mesh_dem_contourtree_mesh_inc::CombinedVectorExecObj<vtkm::Id> allGlobalIndicesExecObj(
    this->globalMeshIndex, other.globalMeshIndex);
  auto allGlobalIndices = allGlobalIndicesExecObj.PrepareForExecution(DeviceTag());
  mesh_dem_contourtree_mesh_inc::CombinedVectorExecObj<FieldType> allSortedValuesExecObj(
    this->sortedValues, other.sortedValues);
  auto allSortedValues = allSortedValuesExecObj.PrepareForExecution(DeviceTag());
  //auto allGlobalIndices = CombinedVector<FieldType(this->thisGlobalMeshIndex, other.globalMeshIndex);

  // Create combined sort order
  IdArrayType
    overallSortOrder; // TODO This vector could potentially be implemented purely as a smart array handle to reduce memory usage
  overallSortOrder.Allocate(this->nVertices + other.nVertices);

  { // Create a new scope so that the following two vectors get deleted when leaving the scope
    auto thisIndices = vtkm::cont::ArrayHandleIndex(this->nVertices); // A regular index array
    markOther markOtherFunctor;
    auto otherIndices = vtkm::cont::make_ArrayHandleTransform(
      vtkm::cont::ArrayHandleIndex(other.nVertices), markOtherFunctor);
    contourtree_mesh_inc_ns::CombinedSimulatedSimplicityIndexComparator<FieldType, DeviceTag>
      cssicFunctor(allSortedValues, allGlobalIndices);
    std::merge(vtkm::cont::ArrayPortalToIteratorBegin(thisIndices.GetPortalConstControl()),
               vtkm::cont::ArrayPortalToIteratorEnd(thisIndices.GetPortalConstControl()),
               vtkm::cont::ArrayPortalToIteratorBegin(otherIndices.GetPortalConstControl()),
               vtkm::cont::ArrayPortalToIteratorEnd(otherIndices.GetPortalConstControl()),
               vtkm::cont::ArrayPortalToIteratorBegin(overallSortOrder.GetPortalControl()),
               cssicFunctor);
  }

#ifdef DEBUG_PRINT
  std::cout << "OverallSortOrder.size  " << overallSortOrder.GetNumberOfValues() << std::endl;
  printIndices("overallSortOrder", overallSortOrder);
  std::cout << std::endl;
#endif

  IdArrayType overallSortIndex;
  overallSortIndex.Allocate(overallSortOrder.GetNumberOfValues());
  {
    // Functor return 0,1 for each element of a CombinedVector depending on whethern the current value is different from the next
    mesh_dem_contourtree_mesh_inc::CombinedVectorDifferentFromNext<vtkm::Id, DeviceTag>
      differentFromNextFunctor(&allGlobalIndices, overallSortOrder);
    auto differentFromNextArr = vtkm::cont::make_ArrayHandleTransform(
      vtkm::cont::ArrayHandleIndex(overallSortIndex.GetNumberOfValues() - 1),
      differentFromNextFunctor);

    // Compute the exclusive scan of our transformed combined vector
    overallSortIndex.GetPortalControl().Set(0, 0);
    IdArrayType tempArr;
    vtkm::cont::Algorithm::ScanInclusive(differentFromNextArr, tempArr);
    vtkm::cont::Algorithm::CopySubRange(
      tempArr, 0, tempArr.GetNumberOfValues(), overallSortIndex, 1);
  }
  vtkm::Id nVerticesCombined =
    overallSortIndex.GetPortalConstControl().Get(overallSortIndex.GetNumberOfValues() - 1) + 1;
#ifdef DEBUG_PRINT
  std::cout << "OverallSortIndex.size  " << overallSortIndex.GetNumberOfValues() << std::endl;
  printIndices("overallSortIndex", overallSortIndex);
  std::cout << "nVerticesCombined: " << nVerticesCombined << std::endl;
  std::cout << std::endl;
#endif

  // thisToCombinedSortOrder and otherToCombinedSortOrder
  IdArrayType thisToCombinedSortOrder;
  thisToCombinedSortOrder.Allocate(this->firstNeighbour.GetNumberOfValues());
  IdArrayType otherToCombinedSortOrder;
  otherToCombinedSortOrder.Allocate(other.firstNeighbour.GetNumberOfValues());
  contourtree_mesh_inc_ns::InitToCombinedSortOrderArraysWorklet
    initToCombinedSortOrderArraysWorklet;
  this->Invoke(initToCombinedSortOrderArraysWorklet,
               overallSortIndex,
               overallSortOrder,
               thisToCombinedSortOrder,
               otherToCombinedSortOrder);

#ifdef DEBUG_PRINT
  printIndices("thisToCombinedSortOrder", thisToCombinedSortOrder);
  printIndices("otherToCombinedSortOrder", otherToCombinedSortOrder);
#endif

  IdArrayType combinedNNeighbours;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nVerticesCombined),
                              combinedNNeighbours);
  { // New scope so that array gets deleted when leaving scope
    IdArrayType nNeighbours;
    this->ComputeNNeighboursVector(nNeighbours);
    auto permutedCombinedNNeighbours =
      vtkm::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedNNeighbours);
    vtkm::cont::Algorithm::Copy(nNeighbours, permutedCombinedNNeighbours);
  }

  IdArrayType combinedOtherStartIndex;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nVerticesCombined),
                              combinedOtherStartIndex);
  { // New scope so that array gets deleted when leaving scope
    IdArrayType nNeighbours;
    other.ComputeNNeighboursVector(nNeighbours);
    contourtree_mesh_inc_ns::CombinedOtherStartIndexNNeighboursWorklet
      combinedOtherStartIndexNNeighboursWorklet;
    this->Invoke(combinedOtherStartIndexNNeighboursWorklet,
                 nNeighbours,              // input
                 otherToCombinedSortOrder, // input
                 combinedNNeighbours,      // input/output
                 combinedOtherStartIndex   // input/output
                 );
  }

#ifdef DEBUG_PRINT
  printIndices("combinedNNeighbours", combinedNNeighbours);
  printIndices("combinedOtherStartIndex", combinedOtherStartIndex);
#endif

  IdArrayType combinedFirstNeighbour;
  combinedFirstNeighbour.Allocate(nVerticesCombined);
  vtkm::cont::Algorithm::ScanExclusive(combinedNNeighbours, combinedFirstNeighbour);
  vtkm::Id nCombinedNeighbours = combinedFirstNeighbour.GetPortalConstControl().Get(
                                   combinedFirstNeighbour.GetNumberOfValues() - 1) +
    combinedNNeighbours.GetPortalConstControl().Get(combinedNNeighbours.GetNumberOfValues() - 1);

  IdArrayType combinedNeighbours;
  combinedNeighbours.Allocate(nCombinedNeighbours);

  // Update combined neighbours
  contourtree_mesh_inc_ns::UpdateCombinedNeighboursWorklet updateCombinedNeighboursWorklet;
  // Updata neighbours from this
  this->Invoke(
    updateCombinedNeighboursWorklet,
    this->firstNeighbour,
    this->neighbours,
    thisToCombinedSortOrder,
    combinedFirstNeighbour,
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(
      0,
      nVerticesCombined), // Constant 0 array. Just needed so we can use the same worklet for both cases
    combinedNeighbours);
  // Update neighbours from other
  this->Invoke(updateCombinedNeighboursWorklet,
               other.firstNeighbour,
               other.neighbours,
               otherToCombinedSortOrder,
               combinedFirstNeighbour,
               combinedOtherStartIndex,
               combinedNeighbours);

  // TODO VTKM -Version MergedCombinedOtherStartIndex. Replace 1r block with the 1s block. Need to check for Segfault in contourtree_mesh_inc_ns::MergeCombinedOtherStartIndexWorklet. This workloat also still uses a number of stl algorithms that should be replaced with VTKm code (which is porbably also why the worklet fails).
  /* // 1s--start
    contourtree_mesh_inc_ns::MergeCombinedOtherStartIndexWorklet<DeviceAdapter> mergeCombinedOtherStartIndexWorklet;
    vtkm::worklet::DispatcherMapField< contourtree_mesh_inc_ns::MergeCombinedOtherStartIndexWorklet<DeviceAdapter>> mergeCombinedOtherStartIndexDispatcher(mergeCombinedOtherStartIndexWorklet);
    this->Invoke(mergeCombinedOtherStartIndexWorklet,
       combinedOtherStartIndex, // (input/output)
       combinedNeighbours,      // (input/output)
       combinedFirstNeighbour   // (input)
       );
    // 1s--end
    */

  // TODO Fix the MergedCombinedOtherStartIndex worklet and remove //1r block below
  // 1r--start
  auto combinedOtherStartIndexPortal = combinedOtherStartIndex.GetPortalControl();
  auto combinedFirstNeighbourPortal = combinedFirstNeighbour.GetPortalConstControl();
  auto combinedNeighboursPortal = combinedNeighbours.GetPortalControl();
  std::vector<vtkm::Id> tempCombinedNeighours((std::size_t)combinedNeighbours.GetNumberOfValues());
  for (vtkm::Id vtx = 0; vtx < combinedNeighbours.GetNumberOfValues(); ++vtx)
  {
    tempCombinedNeighours[(std::size_t)vtx] = combinedNeighboursPortal.Get(vtx);
  }
  for (vtkm::Id vtx = 0; vtx < combinedFirstNeighbour.GetNumberOfValues(); ++vtx)
  {
    if (combinedOtherStartIndexPortal.Get(vtx)) // Needs merge
    {
      auto neighboursBegin = tempCombinedNeighours.begin() + combinedFirstNeighbourPortal.Get(vtx);
      auto neighboursEnd = (vtx < combinedFirstNeighbour.GetNumberOfValues() - 1)
        ? tempCombinedNeighours.begin() + combinedFirstNeighbourPortal.Get(vtx + 1)
        : tempCombinedNeighours.end();
      std::inplace_merge(
        neighboursBegin, neighboursBegin + combinedOtherStartIndexPortal.Get(vtx), neighboursEnd);
      auto it = std::unique(neighboursBegin, neighboursEnd);
      combinedOtherStartIndexPortal.Set(vtx, static_cast<vtkm::Id>(neighboursEnd - it));
      while (it != neighboursEnd)
        *(it++) = NO_SUCH_ELEMENT;
    }
  }
  // copy the values back to VTKm
  for (vtkm::Id vtx = 0; vtx < combinedNeighbours.GetNumberOfValues(); ++vtx)
  {
    combinedNeighboursPortal.Set(vtx, tempCombinedNeighours[(std::size_t)vtx]);
  }
  // 1r--end

  IdArrayType combinedFirstNeighbourShift;
  combinedFirstNeighbourShift.Allocate(combinedFirstNeighbour.GetNumberOfValues());
  vtkm::cont::Algorithm::ScanExclusive(combinedOtherStartIndex, combinedFirstNeighbourShift);

  {
    IdArrayType tempCombinedNeighbours;
    vtkm::cont::Algorithm::CopyIf(
      combinedNeighbours, combinedNeighbours, tempCombinedNeighbours, NotNoSuchElement());
    combinedNeighbours = tempCombinedNeighbours; // Swap the two arrays
  }

  // Adjust firstNeigbour indices by deleted elements
  { // make sure variables created are deleted after the context
    contourtree_mesh_inc_ns::SubtractAssignWorklet subAssignWorklet;
    this->Invoke(subAssignWorklet, combinedFirstNeighbour, combinedFirstNeighbourShift);
  }

  // Compute combined global mesh index arrays
  IdArrayType combinedGlobalMeshIndex;
  combinedGlobalMeshIndex.Allocate(combinedFirstNeighbour.GetNumberOfValues());
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedGlobalMeshIndex =
      vtkm::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedGlobalMeshIndex);
    vtkm::cont::Algorithm::Copy(globalMeshIndex, permutedCombinedGlobalMeshIndex);
  }
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedGlobalMeshIndex =
      vtkm::cont::make_ArrayHandlePermutation(otherToCombinedSortOrder, combinedGlobalMeshIndex);
    vtkm::cont::Algorithm::Copy(other.globalMeshIndex, permutedCombinedGlobalMeshIndex);
  }

  // Compute combined sorted values
  vtkm::cont::ArrayHandle<FieldType> combinedSortedValues;
  combinedSortedValues.Allocate(combinedFirstNeighbour.GetNumberOfValues());
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedSortedValues =
      vtkm::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedSortedValues);
    vtkm::cont::Algorithm::Copy(sortedValues, permutedCombinedSortedValues);
  }
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedSortedValues =
      vtkm::cont::make_ArrayHandlePermutation(otherToCombinedSortOrder, combinedSortedValues);
    vtkm::cont::Algorithm::Copy(other.sortedValues, permutedCombinedSortedValues);
  }

  // Swap in combined version. VTKM ArrayHandles are smart so we can just swap in the new for the old
  this->sortedValues = combinedSortedValues;
  this->globalMeshIndex = combinedGlobalMeshIndex;
  this->neighbours = combinedNeighbours;
  this->firstNeighbour = combinedFirstNeighbour;
  this->nVertices = sortedValues.GetNumberOfValues();
  // TODO Do we need to set the sort order as well?

  // Re-compute maximum number of neigbours
  ComputeMaxNeighbours();

#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMeshes merged", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
void ContourTreeMesh<FieldType>::save(const char* filename) const
{
  std::ofstream os(filename);
  saveVector(os, this->sortOrder);
  saveVector(os, this->sortedValues);
  saveVector(os, this->globalMeshIndex);
  saveVector(os, this->neighbours);
  saveVector(os, this->firstNeighbour);
}

template <typename FieldType>
void ContourTreeMesh<FieldType>::load(const char* filename)
{
  std::ifstream is(filename);
  loadVector(is, this->sortOrder);
  loadVector(is, this->sortedValues);
  loadVector(is, this->globalMeshIndex);
  loadVector(is, this->neighbours);
  loadVector(is, this > firstNeighbour);
  this->ComputeMaxNeighbours();
}

template <typename FieldType>
template <typename ValueType>
void ContourTreeMesh<FieldType>::saveVector(std::ostream& os,
                                            const vtkm::cont::ArrayHandle<ValueType>& vec) const
{
  vtkm::Id numVals = vec.GetNumberOfValues();
  os.write(reinterpret_cast<const char*>(&numVals), sizeof(ValueType));
  auto vecPortal = vec.GetPortalConstControl();
  for (vtkm::Id i = 0; i < numVals; ++i)
    os.write(reinterpret_cast<const char*>(vecPortal.Get(i)), sizeof(ValueType));
}

template <typename FieldType>
template <typename ValueType>
void ContourTreeMesh<FieldType>::loadVector(std::istream& is,
                                            const vtkm::cont::ArrayHandle<ValueType>& vec)
{
  vtkm::Id numVals;
  is.read(reinterpret_cast<char*>(&numVals), sizeof(ValueType));
  vec.Allocate(numVals);
  auto vecPortal = vec.GetPortalControl();
  vtkm::Id val;
  for (vtkm::Id i = 0; i < numVals; ++i)
  {
    is.read(reinterpret_cast<char*>(val), sizeof(ValueType));
    vecPortal.Set(i, val);
  }
}

template <typename FieldType>
MeshBoundaryContourTreeMeshExec ContourTreeMesh<FieldType>::GetMeshBoundaryExecutionObject(
  vtkm::Id totalNRows,
  vtkm::Id totalNCols,
  vtkm::Id3 minIdx,
  vtkm::Id3 maxIdx) const
{
  return MeshBoundaryContourTreeMeshExec(
    this->globalMeshIndex, totalNRows, totalNCols, minIdx, maxIdx);
}

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
