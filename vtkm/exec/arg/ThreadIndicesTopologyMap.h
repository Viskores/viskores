//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesTopologyMap_h
#define vtk_m_exec_arg_ThreadIndicesTopologyMap_h

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/exec/ConnectivityPermuted.h>
#include <vtkm/exec/ConnectivityStructured.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

namespace detail
{

/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
///
static inline VTKM_EXEC vtkm::Id3 InflateTo3D(vtkm::Id3 index)
{
  return index;
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
/// \overload
static inline VTKM_EXEC vtkm::Id3 InflateTo3D(vtkm::Id2 index)
{
  return vtkm::Id3(index[0], index[1], 0);
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
/// \overload
static inline VTKM_EXEC vtkm::Id3 InflateTo3D(vtkm::Vec<vtkm::Id, 1> index)
{
  return vtkm::Id3(index[0], 0, 0);
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
/// \overload
static inline VTKM_EXEC vtkm::Id3 InflateTo3D(vtkm::Id index)
{
  return vtkm::Id3(index, 0, 0);
}

/// Given a vtkm::Id3, reduce down to an identifier of choice.
///
static inline VTKM_EXEC vtkm::Id3 Deflate(const vtkm::Id3& index, vtkm::Id3)
{
  return index;
}

/// Given a vtkm::Id3, reduce down to an identifier of choice.
/// \overload
static inline VTKM_EXEC vtkm::Id2 Deflate(const vtkm::Id3& index, vtkm::Id2)
{
  return vtkm::Id2(index[0], index[1]);
}

} // namespace detail


/// \brief Container for thread indices in a topology map
///
/// This specialization of \c ThreadIndices adds extra indices that deal with
/// topology maps. In particular, it saves the incident element indices. The
/// input and output indices from the superclass are considered to be indexing
/// the visited elements.
///
/// This class is templated on the type that stores the connectivity (such
/// as \c ConnectivityExplicit or \c ConnectivityStructured).
///
template <typename ConnectivityType>
class ThreadIndicesTopologyMap : public vtkm::exec::arg::ThreadIndicesBasic
{
  using Superclass = vtkm::exec::arg::ThreadIndicesBasic;

public:
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using CellShapeTag = typename ConnectivityType::CellShapeTag;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC ThreadIndicesTopologyMap(vtkm::Id threadIndex,
                                     vtkm::Id inputIndex,
                                     vtkm::IdComponent visitIndex,
                                     vtkm::Id outputIndex,
                                     const ConnectivityType& connectivity,
                                     vtkm::Id globalThreadIndexOffset = 0)
    : Superclass(threadIndex, inputIndex, visitIndex, outputIndex, globalThreadIndexOffset)
    // The connectivity is stored in the invocation parameter at the given
    // input domain index. If this class is being used correctly, the type
    // of the domain will match the connectivity type used here. If there is
    // a compile error here about a type mismatch, chances are a worklet has
    // set its input domain incorrectly.
    , IndicesIncident(connectivity.GetIndices(inputIndex))
    , CellShape(connectivity.GetCellShape(inputIndex))
  {
  }

  /// \brief The indices of the incident elements.
  ///
  /// A topology map has "visited" and "incident" elements (e.g. points, cells,
  /// etc). For each worklet invocation, there is exactly one visited element,
  /// but there can be several incident elements. This method returns a Vec-like
  /// object containing the indices to the incident elements.
  ///
  VTKM_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the incident elements in pointer form.
  ///
  /// Returns the same object as GetIndicesIncident except that it returns a
  /// pointer to the internally held object rather than a reference or copy.
  /// Since the from indices can be a sizeable Vec (8 entries is common), it is
  /// best not to have a bunch a copies. Thus, you can pass around a pointer
  /// instead. However, care should be taken to make sure that this object does
  /// not go out of scope, at which time the returned pointer becomes invalid.
  ///
  VTKM_EXEC
  const IndicesIncidentType* GetIndicesIncidentPointer() const { return &this->IndicesIncident; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VTKM_EXEC
  CellShapeTag GetCellShape() const { return this->CellShape; }

private:
  IndicesIncidentType IndicesIncident;
  CellShapeTag CellShape;
};

// Specialization for structured connectivity types.
template <typename VisitTopology, typename IncidentTopology, vtkm::IdComponent Dimension>
class ThreadIndicesTopologyMap<
  vtkm::exec::ConnectivityStructured<VisitTopology, IncidentTopology, Dimension>>
{
  using ConnectivityType =
    vtkm::exec::ConnectivityStructured<VisitTopology, IncidentTopology, Dimension>;

public:
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using CellShapeTag = typename ConnectivityType::CellShapeTag;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  VTKM_EXEC ThreadIndicesTopologyMap(vtkm::Id threadIndex,
                                     vtkm::Id inIndex,
                                     vtkm::IdComponent visitIndex,
                                     vtkm::Id outIndex,
                                     const ConnectivityType& connectivity,
                                     vtkm::Id globalThreadIndexOffset = 0)
  {
    this->ThreadIndex = threadIndex;
    this->InputIndex = inIndex;
    this->VisitIndex = visitIndex;
    this->OutputIndex = outIndex;
    this->LogicalIndex = connectivity.FlatToLogicalToIndex(this->InputIndex);
    this->IndicesIncident = connectivity.GetIndices(this->LogicalIndex);
    this->CellShape = connectivity.GetCellShape(this->InputIndex);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }

  VTKM_EXEC ThreadIndicesTopologyMap(const vtkm::Id3& threadIndex,
                                     const ConnectivityType& connectivity,
                                     const vtkm::Id globalThreadIndexOffset = 0)
  {
    // We currently only support multidimensional indices on one-to-one input-
    // to-output mappings. (We don't have a use case otherwise.)
    // That is why we treat teh threadIndex as also the inputIndex and outputIndex
    const LogicalIndexType logicalIndex = detail::Deflate(threadIndex, LogicalIndexType());
    const vtkm::Id index = connectivity.LogicalToFlatToIndex(logicalIndex);

    this->ThreadIndex = index;
    this->InputIndex = index;
    this->OutputIndex = index;
    this->VisitIndex = 0;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    this->CellShape = connectivity.GetCellShape(index);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }

  /// \brief The index of the thread or work invocation.
  ///
  /// This index refers to which instance of the worklet is being invoked. Every invocation of the
  /// worklet has a unique thread index. This is also called the work index depending on the
  /// context.
  ///
  VTKM_EXEC
  vtkm::Id GetThreadIndex() const { return this->ThreadIndex; }

  /// \brief The logical index into the input domain.
  ///
  /// This is similar to \c GetIndex3D except the Vec size matches the actual
  /// dimensions of the data.
  ///
  VTKM_EXEC
  LogicalIndexType GetIndexLogical() const { return this->LogicalIndex; }

  /// \brief The index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. This is the typical index used during
  /// fetches.
  ///
  VTKM_EXEC
  vtkm::Id GetInputIndex() const { return this->InputIndex; }

  /// \brief The 3D index into the input domain.
  ///
  /// Overloads the implementation in the base class to return the 3D index
  /// for the input.
  ///
  VTKM_EXEC
  vtkm::Id3 GetInputIndex3D() const { return detail::InflateTo3D(this->GetIndexLogical()); }

  /// \brief The index into the output domain.
  ///
  /// This index refers to the output element (array value, cell, etc.) that
  /// this thread is creating. This is the typical index used during
  /// Fetch::Store.
  ///
  VTKM_EXEC
  vtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  /// \brief The visit index.
  ///
  /// When multiple output indices have the same input index, they are
  /// distinguished using the visit index.
  ///
  VTKM_EXEC
  vtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  VTKM_EXEC
  vtkm::Id GetGlobalIndex() const { return (this->GlobalThreadIndexOffset + this->OutputIndex); }

  /// \brief The indices of the incident elements.
  ///
  /// A topology map has "visited" and "incident" elements (e.g. points, cells,
  /// etc). For each worklet invocation, there is exactly one visited element,
  /// but there can be several incident elements. This method returns a
  /// Vec-like object containing the indices to the incident elements.
  ///
  VTKM_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the incident elements in pointer form.
  ///
  /// Returns the same object as GetIndicesIncident except that it returns a
  /// pointer to the internally held object rather than a reference or copy.
  /// Since the from indices can be a sizeable Vec (8 entries is common), it is
  /// best not to have a bunch a copies. Thus, you can pass around a pointer
  /// instead. However, care should be taken to make sure that this object does
  /// not go out of scope, at which time the returned pointer becomes invalid.
  ///
  VTKM_EXEC
  const IndicesIncidentType* GetIndicesIncidentPointer() const { return &this->IndicesIncident; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VTKM_EXEC
  CellShapeTag GetCellShape() const { return this->CellShape; }

private:
  vtkm::Id ThreadIndex;
  vtkm::Id InputIndex;
  vtkm::IdComponent VisitIndex;
  vtkm::Id OutputIndex;
  LogicalIndexType LogicalIndex;
  IndicesIncidentType IndicesIncident;
  CellShapeTag CellShape;
  vtkm::Id GlobalThreadIndexOffset;
};

// Specialization for permuted structured connectivity types.
template <typename PermutationPortal, vtkm::IdComponent Dimension>
class ThreadIndicesTopologyMap<vtkm::exec::ConnectivityPermutedVisitCellsWithPoints<
  PermutationPortal,
  vtkm::exec::
    ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, Dimension>>>
{
  using PermutedConnectivityType = vtkm::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                       vtkm::TopologyElementTagPoint,
                                       Dimension>>;
  using ConnectivityType = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                                              vtkm::TopologyElementTagPoint,
                                                              Dimension>;

public:
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using CellShapeTag = typename ConnectivityType::CellShapeTag;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  VTKM_EXEC ThreadIndicesTopologyMap(vtkm::Id threadIndex,
                                     vtkm::Id inputIndex,
                                     vtkm::IdComponent visitIndex,
                                     vtkm::Id outputIndex,
                                     const PermutedConnectivityType& permutation,
                                     vtkm::Id globalThreadIndexOffset = 0)
  {
    this->ThreadIndex = threadIndex;
    this->InputIndex = inputIndex;
    this->VisitIndex = visitIndex;
    this->OutputIndex = outputIndex;

    const vtkm::Id permutedIndex = permutation.Portal.Get(this->InputIndex);
    this->LogicalIndex = permutation.Connectivity.FlatToLogicalToIndex(permutedIndex);
    this->IndicesIncident = permutation.Connectivity.GetIndices(this->LogicalIndex);
    this->CellShape = permutation.Connectivity.GetCellShape(permutedIndex);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }

  /// \brief The index of the thread or work invocation.
  ///
  /// This index refers to which instance of the worklet is being invoked. Every invocation of the
  /// worklet has a unique thread index. This is also called the work index depending on the
  /// context.
  ///
  VTKM_EXEC
  vtkm::Id GetThreadIndex() const { return this->ThreadIndex; }

  /// \brief The logical index into the input domain.
  ///
  /// This is similar to \c GetIndex3D except the Vec size matches the actual
  /// dimensions of the data.
  ///
  VTKM_EXEC
  LogicalIndexType GetIndexLogical() const { return this->LogicalIndex; }

  /// \brief The index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. This is the typical index used during
  /// fetches.
  ///
  VTKM_EXEC
  vtkm::Id GetInputIndex() const { return this->InputIndex; }

  /// \brief The 3D index into the input domain.
  ///
  /// Overloads the implementation in the base class to return the 3D index
  /// for the input.
  ///
  VTKM_EXEC
  vtkm::Id3 GetInputIndex3D() const { return detail::InflateTo3D(this->GetIndexLogical()); }

  /// \brief The index into the output domain.
  ///
  /// This index refers to the output element (array value, cell, etc.) that
  /// this thread is creating. This is the typical index used during
  /// Fetch::Store.
  ///
  VTKM_EXEC
  vtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  /// \brief The visit index.
  ///
  /// When multiple output indices have the same input index, they are
  /// distinguished using the visit index.
  ///
  VTKM_EXEC
  vtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  VTKM_EXEC
  vtkm::Id GetGlobalIndex() const { return (this->GlobalThreadIndexOffset + this->OutputIndex); }

  /// \brief The indices of the incident elements.
  ///
  /// A topology map has "visited" and "incident" elements (e.g. points, cells,
  /// etc). For each worklet invocation, there is exactly one visited element,
  /// but there can be several incident elements. This method returns a
  /// Vec-like object containing the indices to the incident elements.
  ///
  VTKM_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the incident elements in pointer form.
  ///
  /// Returns the same object as GetIndicesIncident except that it returns a
  /// pointer to the internally held object rather than a reference or copy.
  /// Since the from indices can be a sizeable Vec (8 entries is common), it is
  /// best not to have a bunch a copies. Thus, you can pass around a pointer
  /// instead. However, care should be taken to make sure that this object does
  /// not go out of scope, at which time the returned pointer becomes invalid.
  ///
  VTKM_EXEC
  const IndicesIncidentType* GetIndicesIncidentPointer() const { return &this->IndicesIncident; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VTKM_EXEC
  CellShapeTag GetCellShape() const { return this->CellShape; }

private:
  vtkm::Id ThreadIndex;
  vtkm::Id InputIndex;
  vtkm::IdComponent VisitIndex;
  vtkm::Id OutputIndex;
  LogicalIndexType LogicalIndex;
  IndicesIncidentType IndicesIncident;
  CellShapeTag CellShape;
  vtkm::Id GlobalThreadIndexOffset;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesTopologyMap_h
