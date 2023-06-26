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

#include <vtkm/filter/contour/vtkm_filter_contour_export.h>
#include <vtkm/filter/contour/worklet/contour/CommonState.h>
#include <vtkm/filter/contour/worklet/contour/FieldPropagation.h>
#include <vtkm/filter/contour/worklet/contour/MarchingCells.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/internal/Instantiations.h>


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
  template <typename CellSetType, typename ValueType, typename StorageTagField>
  void operator()(const CellSetType& cells,
                  const vtkm::cont::CoordinateSystem& coordinateSystem,
                  vtkm::cont::CellSetSingleType<>& outputCells,
                  const std::vector<ValueType>& isovalues,
                  const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
                  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
                  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
                  vtkm::worklet::contour::CommonState& sharedState) const;
};

// Declared outside of class, non-inline so that instantiations can be exported correctly.
template <typename CellSetType, typename ValueType, typename StorageTagField>
void DeduceCellType::operator()(const CellSetType& cells,
                                const vtkm::cont::CoordinateSystem& coordinateSystem,
                                vtkm::cont::CellSetSingleType<>& outputCells,
                                const std::vector<ValueType>& isovalues,
                                const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
                                vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
                                vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
                                vtkm::worklet::contour::CommonState& sharedState) const
{
  vtkm::cont::CastAndCall(coordinateSystem,
                          contour::DeduceCoordType{},
                          cells,
                          outputCells,
                          isovalues,
                          input,
                          vertices,
                          normals,
                          sharedState);
}

} // namespace contour

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

public:
  // Filter called without normals generation
  template <typename ValueType, typename StorageTagField>
  VTKM_CONT vtkm::cont::CellSetSingleType<> Run(
    const std::vector<ValueType>& isovalues,
    const vtkm::cont::UnknownCellSet& cells,
    const vtkm::cont::CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices)
  {
    this->SharedState.GenerateNormals = false;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> normals;

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
  template <typename ValueType, typename StorageTagField>
  VTKM_CONT vtkm::cont::CellSetSingleType<> Run(
    const std::vector<ValueType>& isovalues,
    const vtkm::cont::UnknownCellSet& cells,
    const vtkm::cont::CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals)
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

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetStructured<2>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float32>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetStructured<2>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float64>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetStructured<3>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float32>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetStructured<3>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float64>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetExplicit<>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float32>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetExplicit<>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float64>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetSingleType<>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float32>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END
VTKM_INSTANTIATION_BEGIN
extern template void vtkm::worklet::contour::DeduceCellType::operator()(
  const vtkm::cont::CellSetSingleType<>& cells,
  const vtkm::cont::CoordinateSystem& coordinateSystem,
  vtkm::cont::CellSetSingleType<>& outputCells,
  const std::vector<vtkm::Float64>& isovalues,
  const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>& input,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState) const;
VTKM_INSTANTIATION_END

#endif // vtk_m_worklet_ContourMarchingCells_h
