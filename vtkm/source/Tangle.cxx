//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/source/Tangle.h>

namespace vtkm
{
namespace source
{
namespace tangle
{
class TangleField : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn, FieldOut v);
  using ExecutionSignature = void(ThreadIndices, _2);
  using InputDomain = _1;

  const vtkm::Vec3f CellDimsf;
  const vtkm::Vec3f Mins;
  const vtkm::Vec3f Maxs;

  VTKM_CONT
  TangleField(const vtkm::Id3& cdims, const vtkm::Vec3f& mins, const vtkm::Vec3f& maxs)
    : CellDimsf(static_cast<vtkm::FloatDefault>(cdims[0]),
                static_cast<vtkm::FloatDefault>(cdims[1]),
                static_cast<vtkm::FloatDefault>(cdims[2]))
    , Mins(mins)
    , Maxs(maxs)
  {
  }

  template <typename ThreadIndexType>
  VTKM_EXEC void operator()(const ThreadIndexType& threadIndex, vtkm::Float32& v) const
  {
    //We are operating on a 3d structured grid. This means that the threadIndex has
    //efficiently computed the i,j,k of the point current point for us
    const vtkm::Id3 ijk = threadIndex.GetInputIndex3D();
    const vtkm::Vec3f xyzf = static_cast<vtkm::Vec3f>(ijk) / this->CellDimsf;

    const vtkm::Vec3f_32 values = 3.0f * vtkm::Vec3f_32(Mins + (Maxs - Mins) * xyzf);
    const vtkm::Float32& xx = values[0];
    const vtkm::Float32& yy = values[1];
    const vtkm::Float32& zz = values[2];

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};
} // namespace tangle

vtkm::cont::DataSet Tangle::Execute() const
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 pdims{ this->Dims + vtkm::Id3{ 1, 1, 1 } };
  const vtkm::Vec3f mins = { -1.0f, -1.0f, -1.0f };
  const vtkm::Vec3f maxs = { 1.0f, 1.0f, 1.0f };

  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(pdims);
  dataSet.SetCellSet(cellSet);

  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  this->Invoke(tangle::TangleField{ this->Dims, mins, maxs }, cellSet, pointFieldArray);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> cellFieldArray;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 1, cellSet.GetNumberOfCells()),
    cellFieldArray);

  const vtkm::Vec3f origin(0.0f, 0.0f, 0.0f);
  const vtkm::Vec3f spacing(1.0f / static_cast<vtkm::FloatDefault>(this->Dims[0]),
                            1.0f / static_cast<vtkm::FloatDefault>(this->Dims[1]),
                            1.0f / static_cast<vtkm::FloatDefault>(this->Dims[2]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));
  dataSet.AddField(vtkm::cont::make_FieldPoint("nodevar", pointFieldArray));
  dataSet.AddField(vtkm::cont::make_FieldCell("cellvar", cellFieldArray));

  return dataSet;
}

} // namespace source
} // namespace vtkm
