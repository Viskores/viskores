//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_PointLocatorUniformGrid_h
#define vtk_m_cont_PointLocatorUniformGrid_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/PointLocator.h>
#include <vtkm/exec/PointLocatorUniformGrid.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT PointLocatorUniformGrid : public vtkm::cont::PointLocator
{
public:
  using RangeType = vtkm::Vec<vtkm::Range, 3>;

  void SetRange(const RangeType& range)
  {
    if (this->Range != range)
    {
      this->Range = range;
      this->SetModified();
    }
  }

  const RangeType& GetRange() const { return this->Range; }

  void SetComputeRangeFromCoordinates()
  {
    if (!this->IsRangeInvalid())
    {
      this->Range = { { 0.0, -1.0 } };
      this->SetModified();
    }
  }

  void SetNumberOfBins(const vtkm::Id3& bins)
  {
    if (this->Dims != bins)
    {
      this->Dims = bins;
      this->SetModified();
    }
  }

  const vtkm::Id3& GetNumberOfBins() const { return this->Dims; }

protected:
  void Build() override;

  struct PrepareExecutionObjectFunctor;

  VTKM_CONT void PrepareExecutionObject(ExecutionObjectHandleType& execObjHandle,
                                        vtkm::cont::DeviceAdapterId deviceId) const override;

  bool IsRangeInvalid() const
  {
    return (this->Range[0].Max < this->Range[0].Min) || (this->Range[1].Max < this->Range[1].Min) ||
      (this->Range[2].Max < this->Range[2].Min);
  }

private:
  RangeType Range = { { 0.0, -1.0 } };
  vtkm::Id3 Dims = { 32 };

  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> CellLower;
  vtkm::cont::ArrayHandle<vtkm::Id> CellUpper;
};
}
}
#endif //vtk_m_cont_PointLocatorUniformGrid_h
