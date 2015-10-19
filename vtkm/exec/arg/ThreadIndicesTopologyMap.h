//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesTopologyMap_h
#define vtk_m_exec_arg_ThreadIndicesTopologyMap_h

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/exec/ConnectivityStructured.h>

namespace vtkm {
namespace exec {
namespace arg {

namespace detail {

/// Most cell shape tags have a default constructor, but the generic cell shape
/// tag does not to prevent accidently losing the Id, which, unlike the other
/// cell shapes, can vary.
///
template<typename CellShapeTag>
struct CellShapeInitializer
{
  VTKM_EXEC_CONT_EXPORT
  static CellShapeTag GetDefault() { return CellShapeTag(); }
};

template<>
struct CellShapeInitializer<vtkm::CellShapeTagGeneric>
{
  VTKM_EXEC_CONT_EXPORT
  static vtkm::CellShapeTagGeneric GetDefault()
  {
    return vtkm::CellShapeTagGeneric(vtkm::CELL_SHAPE_EMPTY);
  }
};

} // namespace detail

/// \brief Container for thread indices in a topology map
///
/// This specialization of \c ThreadIndices adds extra indices that deal with
/// topology maps. In particular, it saves the indices used to map the "from"
/// elements in the map. The input and output indices from the superclass are
/// considered to be indexing the "to" elements.
///
/// This class is templated on the type that stores the connectivity (such
/// as \c ConnectivityExplicit or \c ConnectivityStructured).
///
template<typename ConnectivityType>
class ThreadIndicesTopologyMap : public vtkm::exec::arg::ThreadIndicesBasic
{
  typedef vtkm::exec::arg::ThreadIndicesBasic Superclass;

public:
  typedef typename ConnectivityType::IndicesType IndicesFromType;
  typedef typename ConnectivityType::CellShapeTag CellShapeTag;

  template<typename Invocation>
  VTKM_EXEC_EXPORT
  ThreadIndicesTopologyMap(vtkm::Id threadIndex, const Invocation &invocation)
    : Superclass(threadIndex, invocation),
      CellShape(detail::CellShapeInitializer<CellShapeTag>::GetDefault())
  {
    // The connectivity is stored in the invocation parameter at the given
    // input domain index. If this class is being used correctly, the type
    // of the domain will match the connectivity type used here. If there is
    // a compile error here about a type mismatch, chances are a worklet has
    // set its input domain incorrectly.
    const ConnectivityType &connectivity = invocation.GetInputDomain();

    this->IndicesFrom = connectivity.GetIndices(this->GetIndex());
    this->CellShape = connectivity.GetCellShape(this->GetIndex());
  }

  /// \brief The input indices of the "from" elements.
  ///
  /// A topology map has "from" and "to" elements (for example from points to
  /// cells). For each worklet invocation, there is exactly one "to" element,
  /// but can be several "from" element. This method returns a Vec-like object
  /// containing the indices to the "from" elements.
  ///
  VTKM_EXEC_EXPORT
  IndicesFromType GetIndicesFrom() const { return this->IndicesFrom; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VTKM_EXEC_EXPORT
  CellShapeTag GetCellShape() const { return this->CellShape; }

private:
  IndicesFromType IndicesFrom;
  CellShapeTag CellShape;
};

namespace detail {

// Helper function to increase an index to 3D.
VTKM_EXEC_EXPORT
vtkm::Id3 InflateTo3D(vtkm::Id3 index) { return index; }

VTKM_EXEC_EXPORT
vtkm::Id3 InflateTo3D(vtkm::Id2 index)
{
  return vtkm::Id3(index[0], index[1], 0);
}

VTKM_EXEC_EXPORT
vtkm::Id3 InflateTo3D(vtkm::Vec<vtkm::Id,1> index)
{
  return vtkm::Id3(index[0], 0, 0);
}

VTKM_EXEC_EXPORT
vtkm::Id3 InflateTo3D(vtkm::Id index)
{
  return vtkm::Id3(index, 0, 0);
}

} // namespace detail

// Specialization for structured connectivity types.
template<typename FromTopology,
         typename ToTopology,
         vtkm::IdComponent Dimension>
class ThreadIndicesTopologyMap<
    vtkm::exec::ConnectivityStructured<FromTopology,ToTopology,Dimension> >
    : public vtkm::exec::arg::ThreadIndicesBasic
{
  typedef vtkm::exec::arg::ThreadIndicesBasic Superclass;
  typedef vtkm::exec::ConnectivityStructured<FromTopology,ToTopology,Dimension>
      ConnectivityType;

public:
  typedef typename ConnectivityType::IndicesType IndicesFromType;
  typedef typename ConnectivityType::CellShapeTag CellShapeTag;
  typedef typename ConnectivityType::SchedulingRangeType LogicalIndexType;

  template<typename Invocation>
  VTKM_EXEC_EXPORT
  ThreadIndicesTopologyMap(vtkm::Id threadIndex, const Invocation &invocation)
    : Superclass(threadIndex, invocation)
  {
    // The connectivity is stored in the invocation parameter at the given
    // input domain index. If this class is being used correctly, the type
    // of the domain will match the connectivity type used here. If there is
    // a compile error here about a type mismatch, chances are a worklet has
    // set its input domain incorrectly.
    const ConnectivityType &connectivity = invocation.GetInputDomain();

    this->LogicalIndex = connectivity.FlatToLogicalToIndex(this->GetIndex());
    this->IndicesFrom = connectivity.GetIndices(this->LogicalIndex);
    this->CellShape = connectivity.GetCellShape(this->GetIndex());
  }

  /// \brief The logical index into the input domain.
  ///
  /// This is similar to \c GetIndex3D except the Vec size matches the actual
  /// dimensions of the data.
  ///
  VTKM_EXEC_EXPORT
  LogicalIndexType GetIndexLogical() const
  {
    return this->LogicalIndex;
  }

  /// \brief The 3D index into the input domain.
  ///
  /// Overloads the implementation in the base class to return the 3D index
  /// for the input.
  ///
  VTKM_EXEC_EXPORT
  vtkm::Id3 GetIndex3D() const
  {
    return detail::InflateTo3D(this->GetIndexLogical());
  }

  /// \brief The input indices of the "from" elements.
  ///
  /// A topology map has "from" and "to" elements (for example from points to
  /// cells). For each worklet invocation, there is exactly one "to" element,
  /// but can be several "from" element. This method returns a Vec-like object
  /// containing the indices to the "from" elements.
  ///
  VTKM_EXEC_EXPORT
  IndicesFromType GetIndicesFrom() const { return this->IndicesFrom; }

  /// \brief The shape of the input cell.
  ///
  /// In topology maps that map from points to something, the indices make up
  /// the structure of a cell. Although the shape tag is not technically and
  /// index, it defines the meaning of the indices, so we put it here. (That
  /// and this class is the only convenient place to store it.)
  ///
  VTKM_EXEC_EXPORT
  CellShapeTag GetCellShape() const { return this->CellShape; }

private:
  LogicalIndexType LogicalIndex;
  IndicesFromType IndicesFrom;
  CellShapeTag CellShape;
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesTopologyMap_h
