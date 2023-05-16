//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_ContourMarchingCells_h
#define vtk_m_worklet_ContourMarchingCells_h

#include <vtkm/filter/contour/worklet/contour/CommonState.h>
#include <vtkm/filter/contour/worklet/contour/FieldPropagation.h>
#include <vtkm/filter/contour/worklet/contour/MarchingCells.h>


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

/// \brief Compute the isosurface of a given 3D data set, supports all linear cell types
class ContourMarchingCells
{
public:
  //----------------------------------------------------------------------------
  ContourMarchingCells(bool mergeDuplicates = true)
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

  // Filter called with normals generation
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


private:
  vtkm::worklet::contour::CommonState SharedState;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ContourMarchingCells_h
