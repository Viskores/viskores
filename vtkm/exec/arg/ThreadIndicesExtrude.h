//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesExtrude_h
#define vtk_m_exec_arg_ThreadIndicesExtrude_h

#include <vtkm/exec/ConnectivityExtrude.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

// Specialization for extrude types.
template <typename Device>
class ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>
{

  using ConnectivityType = vtkm::exec::ConnectivityExtrude<Device>;

public:
  using CellShapeTag = typename ConnectivityType::CellShapeTag;
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;


  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC ThreadIndicesTopologyMap(vtkm::Id threadIndex,
                                     vtkm::Id vtkmNotUsed(inputIndex),
                                     vtkm::IdComponent vtkmNotUsed(visitIndex),
                                     vtkm::Id vtkmNotUsed(outputIndex),
                                     const ConnectivityType& connectivity,
                                     vtkm::Id globalThreadIndexOffset = 0)
  {
    const LogicalIndexType logicalIndex = detail::Deflate(threadIndex, LogicalIndexType());
    const vtkm::Id index = connectivity.LogicalToFlatToIndex(logicalIndex);

    this->ThreadIndex = index;
    this->InputIndex = index;
    this->OutputIndex = index;
    this->VisitIndex = 0;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    //this->CellShape = connectivity.GetCellShape(index);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ThreadIndicesTopologyMap(const vtkm::Id3& threadIndex,
                           const ConnectivityType& connectivity,
                           vtkm::Id globalThreadIndexOffset = 0)
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
    //this->CellShape = connectivity.GetCellShape(index);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }

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

  /// \brief The input indices of the "from" elements.
  ///
  /// A topology map has "from" and "to" elements (for example from points to
  /// cells). For each worklet invocation, there is exactly one "to" element,
  /// but can be several "from" element. This method returns a Vec-like object
  /// containing the indices to the "from" elements.
  ///
  VTKM_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the "from" elements in pointer form.
  ///
  /// Returns the same object as GetIndicesFrom except that it returns a
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
  CellShapeTag GetCellShape() const { return vtkm::CellShapeTagWedge{}; }

private:
  vtkm::Id ThreadIndex;
  vtkm::Id InputIndex;
  vtkm::IdComponent VisitIndex;
  vtkm::Id OutputIndex;
  LogicalIndexType LogicalIndex;
  IndicesIncidentType IndicesIncident;
  //CellShapeTag CellShape;
  vtkm::Id GlobalThreadIndexOffset;
};

// Specialization for extrude types.
template <typename Device>
class ThreadIndicesTopologyMap<vtkm::exec::ReverseConnectivityExtrude<Device>>
{
  using ConnectivityType = vtkm::exec::ReverseConnectivityExtrude<Device>;

public:
  using CellShapeTag = typename ConnectivityType::CellShapeTag;
  using IndicesIncidentType = typename ConnectivityType::IndicesType;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ThreadIndicesTopologyMap(vtkm::Id& threadIndex,
                           vtkm::Id vtkmNotUsed(inputIndex),
                           vtkm::IdComponent vtkmNotUsed(visitIndex),
                           vtkm::Id vtkmNotUsed(outputIndex),
                           const ConnectivityType& connectivity,
                           vtkm::Id globalThreadIndexOffset = 0)
  {
    const LogicalIndexType logicalIndex = detail::Deflate(threadIndex, LogicalIndexType());
    const vtkm::Id index = connectivity.LogicalToFlatToIndex(logicalIndex);

    this->ThreadIndex = index;
    this->InputIndex = index;
    this->OutputIndex = index;
    this->VisitIndex = 0;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    //this->CellShape = connectivity.GetCellShape(index);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }

  ThreadIndicesTopologyMap(const vtkm::Id3& threadIndex,
                           const ConnectivityType& connectivity,
                           vtkm::Id globalThreadIndexOffset = 0)
  {

    const LogicalIndexType logicalIndex = detail::Deflate(threadIndex, LogicalIndexType());
    const vtkm::Id index = connectivity.LogicalToFlatToIndex(logicalIndex);

    this->ThreadIndex = index;
    this->InputIndex = index;
    this->OutputIndex = index;
    this->VisitIndex = 0;
    this->LogicalIndex = logicalIndex;
    this->IndicesIncident = connectivity.GetIndices(logicalIndex);
    //this->CellShape = connectivity.GetCellShape(index);
    this->GlobalThreadIndexOffset = globalThreadIndexOffset;
  }
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

  /// \brief The input indices of the "from" elements.
  ///
  /// A topology map has "from" and "to" elements (for example from points to
  /// cells). For each worklet invocation, there is exactly one "to" element,
  /// but can be several "from" element. This method returns a Vec-like object
  /// containing the indices to the "from" elements.
  ///
  VTKM_EXEC
  const IndicesIncidentType& GetIndicesIncident() const { return this->IndicesIncident; }

  /// \brief The input indices of the "from" elements in pointer form.
  ///
  /// Returns the same object as GetIndicesFrom except that it returns a
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
  CellShapeTag GetCellShape() const { return vtkm::CellShapeTagVertex{}; }

private:
  vtkm::Id ThreadIndex;
  vtkm::Id InputIndex;
  vtkm::IdComponent VisitIndex;
  vtkm::Id OutputIndex;
  LogicalIndexType LogicalIndex;
  IndicesIncidentType IndicesIncident;
  //CellShapeTag CellShape;
  vtkm::Id GlobalThreadIndexOffset;
};

} //namespace arg
}
} // namespace vtkm::exec

#include <vtkm/exec/arg/FetchExtrude.h>

#endif
