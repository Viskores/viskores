//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_Contour_h
#define vtk_m_worklet_Contour_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/worklet/contour/CommonState.h>
#include <vtkm/worklet/contour/FieldPropagation.h>
#include <vtkm/worklet/contour/FlyingEdges.h>
#include <vtkm/worklet/contour/MarchingCells.h>


namespace vtkm
{
namespace worklet
{


namespace contour
{
struct DeduceCoordType
{
  template <typename CoordinateType, typename CellSetType, typename... Args>
  void operator()(const CoordinateType& coords,
                  const CellSetType& cells,
                  vtkm::cont::CellSetSingleType<>& result,
                  Args&&... args) const
  {
    result = marching_cells::execute(cells, coords, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec3f, vtkm::cont::StorageTagUniformPoints>& coords,
    const vtkm::cont::CellSetStructured<3>& cells,
    vtkm::cont::CellSetSingleType<>& result,
    Args&&... args) const
  {
    result = flying_edges::execute(cells, coords, std::forward<Args>(args)...);
  }
};

struct DeduceCellType
{
  template <typename CellSetType, typename CoordinateType, typename... Args>
  void operator()(const CellSetType& cells, CoordinateType&& coordinateSystem, Args&&... args) const
  {
    vtkm::cont::CastAndCall(
      coordinateSystem, contour::DeduceCoordType{}, cells, std::forward<Args>(args)...);
  }
};
}

/// \brief Compute the isosurface of a given 3D data set, supports all
/// linear cell types
class Contour
{
public:
  //----------------------------------------------------------------------------
  Contour(bool mergeDuplicates = true)
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
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices>
  vtkm::cont::CellSetSingleType<> Run(
    const std::vector<ValueType>& isovalues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices)
  {
    this->SharedState.GenerateNormals = false;
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>> normals;

    vtkm::cont::CellSetSingleType<> outputCells;
    vtkm::cont::CastAndCall(cells,
                            contour::DeduceCellType{},
                            coordinateSystem,
                            outputCells,
                            isovalues,
                            input,
                            vertices,
                            normals,
                            this->SharedState);
    return outputCells;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices,
            typename StorageTagNormals>
  vtkm::cont::CellSetSingleType<> Run(
    const std::vector<ValueType>& isovalues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagNormals>& normals)
  {
    this->SharedState.GenerateNormals = true;

    vtkm::cont::CellSetSingleType<> outputCells;
    vtkm::cont::CastAndCall(cells,
                            contour::DeduceCellType{},
                            coordinateSystem,
                            outputCells,
                            isovalues,
                            input,
                            vertices,
                            normals,
                            this->SharedState);
    return outputCells;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {

    using vtkm::worklet::contour::MapPointField;
    vtkm::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher;

    vtkm::cont::ArrayHandle<ValueType> output;
    applyFieldDispatcher.Invoke(this->SharedState.InterpolationEdgeIds,
                                this->SharedState.InterpolationWeights,
                                input,
                                output);

    return output;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->SharedState.CellIdMap, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

  //----------------------------------------------------------------------------
  void ReleaseCellMapArrays() { this->SharedState.CellIdMap.ReleaseResources(); }

private:
  vtkm::worklet::contour::CommonState SharedState;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Contour_h
