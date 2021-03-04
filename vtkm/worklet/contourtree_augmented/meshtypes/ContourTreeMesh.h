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

#ifndef vtk_m_worklet_contourtree_augmented_mesh_dem_contour_tree_mesh_h
#define vtk_m_worklet_contourtree_augmented_mesh_dem_contour_tree_mesh_h

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/io/ErrorIO.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h> // This is needed only as an unused default argument.
#include <vtkm/worklet/contourtree_augmented/meshtypes/MeshStructureContourTreeMesh.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/ArcComparator.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/CombinedOtherStartIndexNNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/CombinedSimulatedSimplicityIndexComparator.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/CombinedVectorDifferentFromNext.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/CompressNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/ComputeMaxNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/FindStartIndexWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/InitToCombinedSortOrderArraysWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/MergeCombinedOtherStartIndexWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/ReplaceArcNumWithToVertexWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/SubtractAssignWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/contourtreemesh/UpdateCombinedNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/mesh_boundary/ComputeMeshBoundaryContourTreeMesh.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundaryContourTreeMesh.h>

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
  void SetPrepareForExecutionBehavior(bool getMax);

  contourtree_mesh_inc_ns::MeshStructureContourTreeMesh PrepareForExecution(
    vtkm::cont::DeviceAdapterId,
    vtkm::cont::Token& token) const;

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

  //  Construct a ContourTreeMesh from nodes/arcs and another ContourTreeMesh (instead of a DataSetMesh)
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
    Load(filename);
    this->NumVertices = this->SortedValues.GetNumberOfValues();
  }

  vtkm::Id GetNumberOfVertices() const { return this->NumVertices; }

  // Combine two ContourTreeMeshes
  void MergeWith(ContourTreeMesh<FieldType>& other);

  // Save/Load the mesh helpers
  void Save(const char* filename) const;
  void Load(const char* filename);

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

  vtkm::Id NumVertices;
  vtkm::cont::ArrayHandleIndex SortOrder;
  vtkm::cont::ArrayHandleIndex SortIndices;
  vtkm::cont::ArrayHandle<FieldType> SortedValues;
  IdArrayType GlobalMeshIndex;
  // neighbours stores for each vertex the indices of its neighbours. For each vertex
  // the indices are sorted by value, i.e, the first neighbour has the lowest and
  // the last neighbour the highest value for the vertex. In the array we just
  // concatinate the list of neighbours from all vertices, i.e., we first
  // have the list of neighbours of the first vertex, then the second vertex and so on, i.e.:
  // [ n_1_1, n_1_2, n_2_1, n_2_2, n_2_3, etc.]
  IdArrayType Neighbours;
  // firstNeighour gives us for each vertex an index into the neighours array indicating
  // the index where the list of neighbours for the vertex begins
  IdArrayType FirstNeighbour;
  // the maximum number of neighbours of a vertex
  vtkm::Id MaxNeighbours;

  // Print Contents
  void PrintContent(std::ostream& outStream = std::cout) const;

  // Debug print routine
  void DebugPrint(const char* message, const char* fileName, long lineNum) const;

  // Get boundary execution object
  MeshBoundaryContourTreeMeshExec GetMeshBoundaryExecutionObject(vtkm::Id3 globalSize,
                                                                 vtkm::Id3 minIdx,
                                                                 vtkm::Id3 maxIdx) const;

  void GetBoundaryVertices(IdArrayType& boundaryVertexArray,                    // output
                           IdArrayType& boundarySortIndexArray,                 // output
                           MeshBoundaryContourTreeMeshExec* meshBoundaryExecObj //input
  ) const;

  /// copies the global IDs for a set of sort IDs
  /// notice that the sort ID is the same as the mesh ID for the ContourTreeMesh class.
  /// To reduce memory usage we here use a fancy array handle rather than copy data
  /// as is needed for the DataSetMesh types.
  /// We here return a fancy array handle to convert values on-the-fly without requiring additional memory
  /// @param[in] sortIds Array with sort Ids to be converted from local to global Ids
  /// @param[in] localToGlobalIdRelabeler This parameter is here only for
  ///            consistency with the DataSetMesh types but is not
  ///            used here and as such can simply be set to nullptr
  inline vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> GetGlobalIdsFromSortIndices(
    const IdArrayType& sortIds,
    const vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler* localToGlobalIdRelabeler =
      nullptr) const
  {                                 // GetGlobalIDsFromSortIndices()
    (void)localToGlobalIdRelabeler; // avoid compiler warning
    return vtkm::cont::make_ArrayHandlePermutation(sortIds, this->GlobalMeshIndex);
  } // GetGlobalIDsFromSortIndices()

  /// copies the global IDs for a set of mesh IDs
  /// notice that the sort ID is the same as the mesh ID for the ContourTreeMesh class.
  /// To reduce memory usage we here use a fancy array handle rather than copy data
  /// as is needed for the DataSetMesh types.
  /// MeshIdArrayType must be an array if Ids. Usually this is a vtkm::worklet::contourtree_augmented::IdArrayType
  /// but in some cases it may also be a fancy array to avoid memory allocation
  /// We here return a fancy array handle to convert values on-the-fly without requiring additional memory
  /// @param[in] meshIds Array with mesh Ids to be converted from local to global Ids
  /// @param[in] localToGlobalIdRelabeler This parameter is here only for
  ///            consistency with the DataSetMesh types but is not
  ///            used here and as such can simply be set to nullptr
  template <typename MeshIdArrayType>
  inline vtkm::cont::ArrayHandlePermutation<MeshIdArrayType, IdArrayType>
  GetGlobalIdsFromMeshIndices(const MeshIdArrayType& meshIds,
                              const vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
                                localToGlobalIdRelabeler = nullptr) const
  {                                 // GetGlobalIDsFromMeshIndices()
    (void)localToGlobalIdRelabeler; // avoid compiler warning
    return vtkm::cont::make_ArrayHandlePermutation(meshIds, this->GlobalMeshIndex);
  } // GetGlobalIDsFromMeshIndices()

private:
  vtkm::cont::Invoker Invoke;

  bool mGetMax; // Define the behavior for the PrepareForExecution function

  // Private init and helper functions
  void InitialiseNeighboursFromArcs(const IdArrayType& arcs);
  void ComputeNNeighboursVector(IdArrayType& nNeighbours) const;
  void ComputeMaxNeighbours();

  // Private helper functions for saving data vectors
  // Internal helper function to save 1D index array to file
  template <typename ValueType>
  void SaveVector(std::ostream& os, const vtkm::cont::ArrayHandle<ValueType>& vec) const;

  // Internal helper function to Load 1D index array from file
  template <typename ValueType>
  void LoadVector(std::istream& is, vtkm::cont::ArrayHandle<ValueType>& vec);

}; // ContourTreeMesh

// print content
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::PrintContent(std::ostream& outStream /*= std::cout*/) const
{ // PrintContent()
  PrintHeader(this->NumVertices, outStream);
  //PrintIndices("SortOrder", SortOrder, outStream);
  PrintValues("SortedValues", SortedValues, -1, outStream);
  PrintIndices("GlobalMeshIndex", GlobalMeshIndex, -1, outStream);
  PrintIndices("Neighbours", Neighbours, -1, outStream);
  PrintIndices("FirstNeighbour", FirstNeighbour, -1, outStream);
  outStream << "MaxNeighbours=" << MaxNeighbours << std::endl;
  outStream << "mGetMax=" << mGetMax << std::endl;
} // PrintContent()

// debug routine
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::DebugPrint(const char* message,
                                                   const char* fileName,
                                                   long lineNum) const
{ // DebugPrint()
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Contour Tree Mesh Contains:     " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  PrintContent(std::cout);
} // DebugPrint()

// create the contour tree mesh from contour tree data
template <typename FieldType>
ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& arcs,
                                            const IdArrayType& inSortOrder,
                                            const vtkm::cont::ArrayHandle<FieldType>& values,
                                            const IdArrayType& inGlobalMeshIndex)
  : SortOrder()
  , SortedValues()
  , GlobalMeshIndex(inGlobalMeshIndex)
  , Neighbours()
  , FirstNeighbour()
{
  this->NumVertices = inSortOrder.GetNumberOfValues();
  // Initalize the SortedIndices as a smart array handle
  this->SortIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  // values permuted by SortOrder to sort the values
  auto permutedValues = vtkm::cont::make_ArrayHandlePermutation(inSortOrder, values);
  // TODO check if we actually need to make this copy here. we could just store the permutedValues array to save memory
  vtkm::cont::Algorithm::Copy(permutedValues, this->SortedValues);
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
inline ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& nodes,
                                                   const IdArrayType& arcs,
                                                   const IdArrayType& inSortOrder,
                                                   const vtkm::cont::ArrayHandle<FieldType>& values,
                                                   const IdArrayType& inGlobalMeshIndex)
  : GlobalMeshIndex(inGlobalMeshIndex)
  , Neighbours()
  , FirstNeighbour()
{
  // Initialize the SortedValues array the values permutted by the SortOrder permutted by the nodes, i.e.,
  // this->SortedValues[v] = values[inSortOrder[nodes[v]]];
  vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedSortOrder(nodes,
                                                                                 inSortOrder);
  auto permutedValues = vtkm::cont::make_ArrayHandlePermutation(permutedSortOrder, values);
  vtkm::cont::Algorithm::Copy(permutedValues, this->SortedValues);
  // Initalize the SortedIndices as a smart array handle
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}

template <typename FieldType>
inline ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& arcs,
                                                   const ContourTreeMesh<FieldType>& mesh)
  : SortedValues(mesh.SortedValues)
  , GlobalMeshIndex(mesh.GlobalMeshIndex)
  , Neighbours()
  , FirstNeighbour()
{
  // Initalize the SortedIndices as a smart array handle
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
inline ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& nodes,
                                                   const IdArrayType& arcs,
                                                   const ContourTreeMesh<FieldType>& mesh)
  : Neighbours()
  , FirstNeighbour()
{
  // Initatlize the global mesh index with the GlobalMeshIndex permutted by the nodes
  vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedGlobalMeshIndex(
    nodes, mesh.GlobalMeshIndex);
  vtkm::cont::Algorithm::Copy(permutedGlobalMeshIndex, this->GlobalMeshIndex);
  // Initialize the SortedValues array with the SortedValues permutted by the nodes
  auto permutedSortedValues = vtkm::cont::make_ArrayHandlePermutation(nodes, mesh.SortedValues);
  vtkm::cont::Algorithm::Copy(permutedSortedValues, this->SortedValues);
  // Initialize the neighbours from the arcs
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->InitialiseNeighboursFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


// Initalize the contour tree from the arcs array and sort order
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::InitialiseNeighboursFromArcs(const IdArrayType& arcs)
{
  // Find target indices for valid arcs in neighbours array ...
  IdArrayType arcTargetIndex;
  arcTargetIndex.Allocate(arcs.GetNumberOfValues());
  OneIfArcValid oneIfArcValidFunctor;
  auto oneIfArcValidArrayHandle =
    vtkm::cont::ArrayHandleTransform<IdArrayType, OneIfArcValid>(arcs, oneIfArcValidFunctor);
  vtkm::cont::Algorithm::ScanExclusive(oneIfArcValidArrayHandle, arcTargetIndex);
  vtkm::Id nValidArcs = arcTargetIndex.ReadPortal().Get(arcTargetIndex.GetNumberOfValues() - 1) +
    oneIfArcValidFunctor(arcs.ReadPortal().Get(arcs.GetNumberOfValues() - 1));

  // ... and compress array
  this->Neighbours.ReleaseResources();
  this->Neighbours.Allocate(2 * nValidArcs);

  contourtree_mesh_inc_ns::CompressNeighboursWorklet compressNeighboursWorklet;
  this->Invoke(compressNeighboursWorklet, arcs, arcTargetIndex, this->Neighbours);

  // Sort arcs so that all arcs from the same vertex are adjacent. All arcs are in neighbours array based on
  // sort index of their 'from' vertex (and then within a run sorted by sort index of their 'to' vertex).
  vtkm::cont::Algorithm::Sort(this->Neighbours, contourtree_mesh_inc_ns::ArcComparator(arcs));

  // Find start index for each vertex into neighbours array
  this->FirstNeighbour.Allocate(this->NumVertices);

  contourtree_mesh_inc_ns::FindStartIndexWorklet findStartIndexWorklet;
  this->Invoke(findStartIndexWorklet,
               this->Neighbours,
               arcs,
               this->FirstNeighbour // output
  );


  // Replace arc number with 'to' vertex in neighbours array
  contourtree_mesh_inc_ns::ReplaceArcNumWithToVertexWorklet replaceArcNumWithToVertexWorklet;
  this->Invoke(replaceArcNumWithToVertexWorklet,
               this->Neighbours, // input/output
               arcs              // input
  );

  // Compute maximum number of neighbours
  this->ComputeMaxNeighbours();

#ifdef DEBUG_PRINT
  std::cout << std::setw(30) << std::left << __FILE__ << ":" << std::right << std::setw(4)
            << __LINE__ << std::endl;
  auto firstNeighbourPortal = this->FirstNeighbour.ReadPortal();
  auto neighboursPortal = this->Neighbours.ReadPortal();
  for (vtkm::Id vtx = 0; vtx < FirstNeighbour.GetNumberOfValues(); ++vtx)
  {
    std::cout << vtx << ": ";
    vtkm::Id neighboursBeginIndex = firstNeighbourPortal.Get(vtx);
    vtkm::Id neighboursEndIndex = (vtx < this->NumVertices - 1) ? firstNeighbourPortal.Get(vtx + 1)
                                                                : Neighbours.GetNumberOfValues();

    for (vtkm::Id ni = neighboursBeginIndex; ni < neighboursEndIndex; ++ni)
    {
      std::cout << neighboursPortal.Get(ni) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "Max neighbours: " << this->MaxNeighbours << std::endl;
#endif
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::ComputeNNeighboursVector(IdArrayType& nNeighbours) const
{
  nNeighbours.Allocate(this->FirstNeighbour.GetNumberOfValues()); // same as this->NumVertices
  contourtree_mesh_inc_ns::ComputeMaxNeighboursWorklet computeMaxNeighboursWorklet(
    this->Neighbours.GetNumberOfValues());
  this->Invoke(computeMaxNeighboursWorklet, this->FirstNeighbour, nNeighbours);
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::ComputeMaxNeighbours()
{
  // Compute maximum number of neighbours
  IdArrayType nNeighbours;
  this->ComputeNNeighboursVector(nNeighbours);
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray = vtkm::cont::ArrayRangeCompute(nNeighbours);
  this->MaxNeighbours = static_cast<vtkm::Id>(rangeArray.ReadPortal().Get(0).Max);
}

// Define the behavior for the execution object generate by the PrepareForExecution function
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::SetPrepareForExecutionBehavior(bool getMax)
{
  this->mGetMax = getMax;
}

// Get VTKM execution object that represents the structure of the mesh and provides the mesh helper functions on the device
template <typename FieldType>
contourtree_mesh_inc_ns::MeshStructureContourTreeMesh inline ContourTreeMesh<
  FieldType>::PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                  vtkm::cont::Token& token) const
{
  return contourtree_mesh_inc_ns::MeshStructureContourTreeMesh(
    this->Neighbours, this->FirstNeighbour, this->MaxNeighbours, this->mGetMax, device, token);
}

struct NotNoSuchElement
{
  VTKM_EXEC_CONT bool operator()(vtkm::Id x) const { return x != NO_SUCH_ELEMENT; }
};

// Combine two ContourTreeMeshes
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::MergeWith(ContourTreeMesh<FieldType>& other)
{ // Merge With
#ifdef DEBUG_PRINT
  this->DebugPrint("THIS ContourTreeMesh", __FILE__, __LINE__);
  other.DebugPrint("OTHER ContourTreeMesh", __FILE__, __LINE__);
#endif

  // Create combined sort order
  // TODO This vector could potentially be implemented purely as a smart array handle to reduce memory usage
  IdArrayType overallSortOrder;
  overallSortOrder.Allocate(this->NumVertices + other.NumVertices);

  { // Create a new scope so that the following two vectors get deleted when leaving the scope
    auto thisIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices); // A regular index array
    MarkOther markOtherFunctor;
    auto otherIndices = vtkm::cont::make_ArrayHandleTransform(
      vtkm::cont::ArrayHandleIndex(other.NumVertices), markOtherFunctor);
    contourtree_mesh_inc_ns::CombinedSimulatedSimplicityIndexComparator<FieldType>
      cssicFunctorExecObj(
        this->GlobalMeshIndex, other.GlobalMeshIndex, this->SortedValues, other.SortedValues);
    // TODO FIXME We here need to force the arrays for the comparator onto the CPU by using DeviceAdapterTagSerial
    //            Instead we should implement the merge of the arrays on the device and not use std::merge
    vtkm::cont::Token tempToken;
    auto cssicFunctor =
      cssicFunctorExecObj.PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial(), tempToken);
    // Merge the arrays
    std::merge(vtkm::cont::ArrayPortalToIteratorBegin(thisIndices.ReadPortal()),
               vtkm::cont::ArrayPortalToIteratorEnd(thisIndices.ReadPortal()),
               vtkm::cont::ArrayPortalToIteratorBegin(otherIndices.ReadPortal()),
               vtkm::cont::ArrayPortalToIteratorEnd(otherIndices.ReadPortal()),
               vtkm::cont::ArrayPortalToIteratorBegin(overallSortOrder.WritePortal()),
               cssicFunctor);
  }

#ifdef DEBUG_PRINT
  std::cout << "OverallSortOrder.size  " << overallSortOrder.GetNumberOfValues() << std::endl;
  PrintIndices("overallSortOrder", overallSortOrder);
  std::cout << std::endl;
#endif

  IdArrayType overallSortIndex;
  overallSortIndex.Allocate(overallSortOrder.GetNumberOfValues());
  {
    // Array decorator with functor returning 0, 1 for each element depending
    // on whethern the current value is different from the next
    auto differentFromNextArr = vtkm::cont::make_ArrayHandleDecorator(
      overallSortIndex.GetNumberOfValues() - 1,
      mesh_dem_contourtree_mesh_inc::CombinedVectorDifferentFromNextDecoratorImpl{},
      overallSortOrder,
      this->GlobalMeshIndex,
      other.GlobalMeshIndex);

    // Compute the exclusive scan of our transformed combined vector
    overallSortIndex.WritePortal().Set(0, 0);
    IdArrayType tempArr;

    vtkm::cont::Algorithm::ScanInclusive(differentFromNextArr, tempArr);
    vtkm::cont::Algorithm::CopySubRange(
      tempArr, 0, tempArr.GetNumberOfValues(), overallSortIndex, 1);
  }
  vtkm::Id numVerticesCombined =
    overallSortIndex.ReadPortal().Get(overallSortIndex.GetNumberOfValues() - 1) + 1;

#ifdef DEBUG_PRINT
  std::cout << "OverallSortIndex.size  " << overallSortIndex.GetNumberOfValues() << std::endl;
  PrintIndices("overallSortIndex", overallSortIndex);
  std::cout << "numVerticesCombined: " << numVerticesCombined << std::endl;
  std::cout << std::endl;
#endif

  // thisToCombinedSortOrder and otherToCombinedSortOrder
  IdArrayType thisToCombinedSortOrder;
  thisToCombinedSortOrder.Allocate(this->FirstNeighbour.GetNumberOfValues());
  IdArrayType otherToCombinedSortOrder;
  otherToCombinedSortOrder.Allocate(other.FirstNeighbour.GetNumberOfValues());
  contourtree_mesh_inc_ns::InitToCombinedSortOrderArraysWorklet
    initToCombinedSortOrderArraysWorklet;
  this->Invoke(initToCombinedSortOrderArraysWorklet,
               overallSortIndex,
               overallSortOrder,
               thisToCombinedSortOrder,
               otherToCombinedSortOrder);

#ifdef DEBUG_PRINT
  PrintIndices("thisToCombinedSortOrder", thisToCombinedSortOrder);
  PrintIndices("otherToCombinedSortOrder", otherToCombinedSortOrder);
#endif

  IdArrayType combinedNNeighbours;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numVerticesCombined),
                              combinedNNeighbours);
  { // New scope so that array gets deleted when leaving scope
    IdArrayType nNeighbours;
    this->ComputeNNeighboursVector(nNeighbours);
    auto permutedCombinedNNeighbours =
      vtkm::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedNNeighbours);
    vtkm::cont::Algorithm::Copy(nNeighbours, permutedCombinedNNeighbours);
  }

  IdArrayType combinedOtherStartIndex;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numVerticesCombined),
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
  PrintIndices("combinedNNeighbours", combinedNNeighbours);
  PrintIndices("combinedOtherStartIndex", combinedOtherStartIndex);
#endif

  IdArrayType combinedFirstNeighbour;
  combinedFirstNeighbour.Allocate(numVerticesCombined);
  vtkm::cont::Algorithm::ScanExclusive(combinedNNeighbours, combinedFirstNeighbour);
  vtkm::Id nCombinedNeighbours =
    combinedFirstNeighbour.ReadPortal().Get(combinedFirstNeighbour.GetNumberOfValues() - 1) +
    combinedNNeighbours.ReadPortal().Get(combinedNNeighbours.GetNumberOfValues() - 1);

  IdArrayType combinedNeighbours;
  combinedNeighbours.Allocate(nCombinedNeighbours);

  // Update combined neighbours
  contourtree_mesh_inc_ns::UpdateCombinedNeighboursWorklet updateCombinedNeighboursWorklet;
  // Updata neighbours from this
  this->Invoke(
    updateCombinedNeighboursWorklet,
    this->FirstNeighbour,
    this->Neighbours,
    thisToCombinedSortOrder,
    combinedFirstNeighbour,
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(
      0,
      numVerticesCombined), // Constant 0 array. Just needed so we can use the same worklet for both cases
    combinedNeighbours);
  // Update neighbours from other
  this->Invoke(updateCombinedNeighboursWorklet,
               other.FirstNeighbour,
               other.Neighbours,
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
  auto combinedOtherStartIndexPortal = combinedOtherStartIndex.WritePortal();
  auto combinedFirstNeighbourPortal = combinedFirstNeighbour.ReadPortal();
  auto combinedNeighboursPortal = combinedNeighbours.WritePortal();
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
    vtkm::cont::Algorithm::Copy(GlobalMeshIndex, permutedCombinedGlobalMeshIndex);
  }
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedGlobalMeshIndex =
      vtkm::cont::make_ArrayHandlePermutation(otherToCombinedSortOrder, combinedGlobalMeshIndex);
    vtkm::cont::Algorithm::Copy(other.GlobalMeshIndex, permutedCombinedGlobalMeshIndex);
  }

  // Compute combined sorted values
  vtkm::cont::ArrayHandle<FieldType> combinedSortedValues;
  combinedSortedValues.Allocate(combinedFirstNeighbour.GetNumberOfValues());
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedSortedValues =
      vtkm::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedSortedValues);
    vtkm::cont::Algorithm::Copy(SortedValues, permutedCombinedSortedValues);
  }
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedSortedValues =
      vtkm::cont::make_ArrayHandlePermutation(otherToCombinedSortOrder, combinedSortedValues);
    vtkm::cont::Algorithm::Copy(other.SortedValues, permutedCombinedSortedValues);
  }

  // Swap in combined version. VTKM ArrayHandles are smart so we can just swap in the new for the old
  this->SortedValues = combinedSortedValues;
  this->GlobalMeshIndex = combinedGlobalMeshIndex;
  this->Neighbours = combinedNeighbours;
  this->FirstNeighbour = combinedFirstNeighbour;
  this->NumVertices = SortedValues.GetNumberOfValues();
  this->SortIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = vtkm::cont::ArrayHandleIndex(this->NumVertices);

  // Re-compute maximum number of neigbours
  ComputeMaxNeighbours();

#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMeshes merged", __FILE__, __LINE__);
#endif
} // Merge With


template <typename FieldType>
inline void ContourTreeMesh<FieldType>::Save(const char* filename) const
{
  std::ofstream os(filename);
  SaveVector(os, this->SortedValues);
  SaveVector(os, this->GlobalMeshIndex);
  SaveVector(os, this->Neighbours);
  SaveVector(os, this->FirstNeighbour);
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::Load(const char* filename)
{
  std::ifstream is(filename);
  if (!is.is_open())
  {
    throw vtkm::io::ErrorIO(std::string("Unable to open file: ") + std::string(filename));
  }
  LoadVector(is, this->SortedValues);
  LoadVector(is, this->GlobalMeshIndex);
  LoadVector(is, this->Neighbours);
  LoadVector(is, this->FirstNeighbour);
  this->ComputeMaxNeighbours();
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortOrder = vtkm::cont::ArrayHandleIndex(this->NumVertices);
  this->SortIndices = vtkm::cont::ArrayHandleIndex(this->NumVertices);
}

template <typename FieldType>
template <typename ValueType>
inline void ContourTreeMesh<FieldType>::SaveVector(
  std::ostream& os,
  const vtkm::cont::ArrayHandle<ValueType>& vec) const
{
  vtkm::Id numVals = vec.GetNumberOfValues();
  //os.write(rXeinterpret_cast<const char*>(&numVals), sizeof(ValueType));
  os << numVals << ": ";
  auto vecPortal = vec.ReadPortal();
  for (vtkm::Id i = 0; i < numVals; ++i)
    os << vecPortal.Get(i) << " ";
  //os.write(reinterpret_cast<const char*>(vecPortal.Get(i)), sizeof(ValueType));
  os << std::endl;
}

template <typename FieldType>
template <typename ValueType>
inline void ContourTreeMesh<FieldType>::LoadVector(std::istream& is,
                                                   vtkm::cont::ArrayHandle<ValueType>& vec)
{
  vtkm::Id numVals;
  is >> numVals;
  char colon = is.get();
  if (colon != ':')
  {
    throw vtkm::io::ErrorIO("Error parsing file");
  }

  vec.Allocate(numVals);
  auto vecPortal = vec.WritePortal();
  ValueType val;
  for (vtkm::Id i = 0; i < numVals; ++i)
  {
    is >> val;
    vecPortal.Set(i, val);
  }
}

template <typename FieldType>
inline MeshBoundaryContourTreeMeshExec ContourTreeMesh<FieldType>::GetMeshBoundaryExecutionObject(
  vtkm::Id3 globalSize,
  vtkm::Id3 minIdx,
  vtkm::Id3 maxIdx) const
{
  return MeshBoundaryContourTreeMeshExec(this->GlobalMeshIndex, globalSize, minIdx, maxIdx);
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::GetBoundaryVertices(
  IdArrayType& boundaryVertexArray,                    // output
  IdArrayType& boundarySortIndexArray,                 // output
  MeshBoundaryContourTreeMeshExec* meshBoundaryExecObj //input
) const
{
  // start by generating a temporary array of indices
  auto indexArray = vtkm::cont::ArrayHandleIndex(this->GlobalMeshIndex.GetNumberOfValues());
  // compute the boolean array indicating which values lie on the boundary
  vtkm::cont::ArrayHandle<bool> isOnBoundary;
  ComputeMeshBoundaryContourTreeMesh computeMeshBoundaryContourTreeMeshWorklet;
  this->Invoke(computeMeshBoundaryContourTreeMeshWorklet,
               indexArray,           // input
               *meshBoundaryExecObj, // input
               isOnBoundary          // outut
  );

  // we will conditionally copy the boundary vertices' indices, capturing the end iterator to compute the # of boundary vertices
  vtkm::cont::Algorithm::CopyIf(indexArray, isOnBoundary, boundaryVertexArray);
  // duplicate these into the index array, since the BRACT uses indices into the underlying mesh anyway
  vtkm::cont::Algorithm::Copy(boundaryVertexArray, boundarySortIndexArray);
}

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
