//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_ContourFlyingEdges_h
#define vtk_m_worklet_ContourFlyingEdges_h


#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/filter/contour/worklet/contour/CommonState.h>
#include <vtkm/filter/contour/worklet/contour/FieldPropagation.h>
#include <vtkm/filter/contour/worklet/contour/FlyingEdges.h>

namespace vtkm
{
namespace worklet
{

/// \brief Compute the isosurface of a given \c CellSetStructured<3> input with
/// \c ArrayHandleUniformPointCoordinates for point coordinates using the Flying Edges algorithm.
class ContourFlyingEdges
{
public:
  //----------------------------------------------------------------------------
  ContourFlyingEdges(bool mergeDuplicates = true)
    : SharedState(mergeDuplicates)
  {
  }

  //----------------------------------------------------------------------------
  vtkm::cont::ArrayHandle<vtkm::Id2> GetInterpolationEdgeIds() const
  {
    return this->SharedState.InterpolationEdgeIds;
  }

  //----------------------------------------------------------------------------
  void SetMergeDuplicatePoints(bool merge) { this->SharedState.MergeDuplicatePoints = merge; }

  //----------------------------------------------------------------------------
  bool GetMergeDuplicatePoints() const { return this->SharedState.MergeDuplicatePoints; }

  //----------------------------------------------------------------------------
  vtkm::cont::ArrayHandle<vtkm::Id> GetCellIdMap() const { return this->SharedState.CellIdMap; }

  //----------------------------------------------------------------------------
  template <typename InArrayType, typename OutArrayType>
  void ProcessPointField(const InArrayType& input, const OutArrayType& output) const
  {

    using vtkm::worklet::contour::MapPointField;
    vtkm::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher;

    applyFieldDispatcher.Invoke(this->SharedState.InterpolationEdgeIds,
                                this->SharedState.InterpolationWeights,
                                input,
                                output);
  }

  //----------------------------------------------------------------------------
  void ReleaseCellMapArrays() { this->SharedState.CellIdMap.ReleaseResources(); }

  // Filter called without normals generation
  template <typename IVType,
            typename ValueType,
            typename CoordsType,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices>
  vtkm::cont::CellSetSingleType<> Run(
    const std::vector<IVType>& isovalues,
    const vtkm::cont::CellSetStructured<3>& cells,
    const CoordsType& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices)
  {
    this->SharedState.GenerateNormals = false;
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>> normals;

    vtkm::cont::CellSetSingleType<> outputCells;
    return flying_edges::execute(
      cells, coordinateSystem, isovalues, input, vertices, normals, this->SharedState);
  }

  // Filter called with normals generation
  template <typename IVType,
            typename ValueType,
            typename CoordsType,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices,
            typename StorageTagNormals>
  vtkm::cont::CellSetSingleType<> Run(
    const std::vector<IVType>& isovalues,
    const vtkm::cont::CellSetStructured<3>& cells,
    const CoordsType& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagNormals>& normals)
  {
    this->SharedState.GenerateNormals = true;
    vtkm::cont::CellSetSingleType<> outputCells;
    return flying_edges::execute(
      cells, coordinateSystem, isovalues, input, vertices, normals, this->SharedState);
  }

private:
  vtkm::worklet::contour::CommonState SharedState;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ContourFlyingEdges_h
