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
#ifndef vtk_m_cont_internal_ConnectivityExplicitInternals_h
#define vtk_m_cont_internal_ConnectivityExplicitInternals_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {
namespace internal {

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename NumIndicesStorageTag    = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG,
         typename IndexOffsetStorageTag   = VTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  typedef vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> ShapeArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, NumIndicesStorageTag> NumIndicesArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> ConnectivityArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, IndexOffsetStorageTag> IndexOffsetArrayType;

  ShapeArrayType Shapes;
  NumIndicesArrayType NumIndices;
  ConnectivityArrayType Connectivity;
  IndexOffsetArrayType IndexOffsets;

  bool ElementsValid;
  bool IndexOffsetsValid;

  VTKM_CONT_EXPORT
  ConnectivityExplicitInternals()
    : ElementsValid(false), IndexOffsetsValid(false) {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {
    return this->Shapes.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void ReleaseResourcesExecution() {
    this->Shapes.ReleaseResourcesExecution();
    this->NumIndices.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->IndexOffsets.ReleaseResourcesExecution();
  }

  template<typename Device>
  VTKM_CONT_EXPORT
  void BuildIndexOffsets(Device) const
  {
    VTKM_ASSERT_CONT(this->ElementsValid);
    if (!this->IndexOffsetsValid)
    {
      // Although technically we are making changes to this object, the changes
      // are logically consistent with the previous state, so we consider it
      // valid under const.
      vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
            this->NumIndices,
            const_cast<IndexOffsetArrayType&>(this->IndexOffsets));
      const_cast<bool&>(this->IndexOffsetsValid) = true;
    }
    else
    {
      // Index offsets already built. Nothing to do.
    }
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
    out <<"     Shapes: ";
    vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
    out << std::endl;
    out << "     NumIndices: ";
    vtkm::cont::printSummary_ArrayHandle(this->NumIndices, out);
    out << std::endl;
    out << "     Connectivity: ";
    vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
    out << std::endl;
    out << "     IndexOffsets: ";
    vtkm::cont::printSummary_ArrayHandle(this->IndexOffsets, out);
  }
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ConnectivityExplicitInternals_h
