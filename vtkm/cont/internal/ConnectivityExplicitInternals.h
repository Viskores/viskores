//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ConnectivityExplicitInternals_h
#define vtk_m_cont_internal_ConnectivityExplicitInternals_h

#include <vtkm/CellShape.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename ShapesStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  using ShapesArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8, ShapesStorageTag>;
  using ConnectivityArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>;
  using OffsetsArrayType = vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>;

  ShapesArrayType Shapes;
  ConnectivityArrayType Connectivity;
  OffsetsArrayType Offsets;

  bool ElementsValid;

  VTKM_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfElements() const
  {
    VTKM_ASSERT(this->ElementsValid);

    return this->Shapes.GetNumberOfValues();
  }

  VTKM_CONT
  void ReleaseResourcesExecution()
  {
    this->Shapes.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->Offsets.ReleaseResourcesExecution();
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    if (this->ElementsValid)
    {
      out << "     Shapes: ";
      vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << "     Connectivity: ";
      vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
      out << "     Offsets: ";
      vtkm::cont::printSummary_ArrayHandle(this->Offsets, out);
    }
    else
    {
      out << "     Not Allocated" << std::endl;
    }
  }
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ConnectivityExplicitInternals_h
