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

#include <vtkm/worklet/contour/CommonState.h>
#include <vtkm/worklet/contour/FieldPropagation.h>
#include <vtkm/worklet/contour/MarchingCells.h>


namespace vtkm
{
namespace worklet
{

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
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices>
  vtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices)
  {
    this->SharedState.GenerateNormals = false;
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>> normals;

    vtkm::cont::CellSetSingleType<> outputCells;
    vtkm::cont::CastAndCall(cells,
                            DeduceCellType{},
                            this,
                            outputCells,
                            isovalues,
                            numIsoValues,
                            coordinateSystem,
                            input,
                            vertices,
                            normals);
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
    const ValueType* const isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagNormals>& normals)
  {
    this->SharedState.GenerateNormals = true;

    vtkm::cont::CellSetSingleType<> outputCells;
    vtkm::cont::CastAndCall(cells,
                            DeduceCellType{},
                            this,
                            outputCells,
                            isovalues,
                            numIsoValues,
                            coordinateSystem,
                            input,
                            vertices,
                            normals);
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
  struct DeduceCellType
  {
    template <typename CellSetType, typename ContourAlg, typename... Args>
    void operator()(const CellSetType& cells,
                    ContourAlg* contour,
                    vtkm::cont::CellSetSingleType<>& result,
                    Args&&... args) const
    {
      result = contour->DoRun(cells, std::forward<Args>(args)...);
    }
  };

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType>
  vtkm::cont::CellSetSingleType<> DoRun(
    const CellSetType& cells,
    const ValueType* isovalues,
    const vtkm::Id numIsoValues,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagNormals> normals)
  {
    return worklet::marching_cells::execute(isovalues,
                                            numIsoValues,
                                            cells,
                                            coordinateSystem,
                                            inputField,
                                            vertices,
                                            normals,
                                            this->SharedState);
  }

  vtkm::worklet::contour::CommonState SharedState;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Contour_h
