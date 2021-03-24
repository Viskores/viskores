//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_PointLocatorSparseGrid_h
#define vtk_m_cont_PointLocatorSparseGrid_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/internal/PointLocatorBase.h>
#include <vtkm/exec/PointLocatorSparseGrid.h>

namespace vtkm
{
namespace cont
{

/// \brief A locator that bins points in a sparsely stored grid.
///
/// `PointLocatorSparseGrid` creates a very dense logical grid over the region containing
/// the points of the provided data set. Although this logical grid has uniform structure,
/// it is stored sparsely. So, it is expected that most of the bins in the structure will
/// be empty but not explicitly stored. This makes `PointLocatorSparseGrid` a good
/// representation for unstructured or irregular collections of points.
///
/// The algorithm used in `PointLocatorSparseGrid` is described in the following publication:
///
/// Abhishek Yenpure, Hank Childs, and Kenneth Moreland. "Efficient Point Merging Using Data
/// Parallel Techniques." In _Eurographics Symposium on Parallel Graphics and Visualization
/// (EGPGV)_, June 2019. DOI 10.2312/pgv.20191112.
///
class VTKM_CONT_EXPORT PointLocatorSparseGrid
  : public vtkm::cont::internal::PointLocatorBase<PointLocatorSparseGrid>
{
  using Superclass = vtkm::cont::internal::PointLocatorBase<PointLocatorSparseGrid>;

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

  VTKM_CONT
  vtkm::exec::PointLocatorSparseGrid PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                         vtkm::cont::Token& token) const;

private:
  bool IsRangeInvalid() const
  {
    return (this->Range[0].Max < this->Range[0].Min) || (this->Range[1].Max < this->Range[1].Min) ||
      (this->Range[2].Max < this->Range[2].Min);
  }

  friend Superclass;
  VTKM_CONT void Build();

  RangeType Range = { { 0.0, -1.0 } };
  vtkm::Id3 Dims = { 32 };

  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Id> CellLower;
  vtkm::cont::ArrayHandle<vtkm::Id> CellUpper;
};
}
}
#endif //vtk_m_cont_PointLocatorSparseGrid_h
