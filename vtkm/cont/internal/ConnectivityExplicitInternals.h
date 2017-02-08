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
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>

namespace vtkm {
namespace cont {
namespace internal {

template<typename NumIndicesArrayType,
         typename IndexOffsetArrayType,
         typename DeviceAdapterTag>
void buildIndexOffsets(const NumIndicesArrayType& numIndices,
                       IndexOffsetArrayType& offsets,
                       DeviceAdapterTag,
                       std::true_type)
{
  //We first need to make sure that NumIndices and IndexOffsetArrayType
  //have the same type so we can call scane exclusive
  typedef vtkm::cont::ArrayHandleCast< vtkm::Id,
    NumIndicesArrayType > CastedNumIndicesType;

  // Although technically we are making changes to this object, the changes
  // are logically consistent with the previous state, so we consider it
  // valid under const.
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> Algorithm;
  Algorithm::ScanExclusive( CastedNumIndicesType(numIndices), offsets);
}

template<typename NumIndicesArrayType,
         typename IndexOffsetArrayType,
         typename DeviceAdapterTag>
void buildIndexOffsets(const NumIndicesArrayType&,
                       IndexOffsetArrayType&,
                       DeviceAdapterTag,
                       std::false_type)
{
  //this is a no-op as the storage for the offsets is an implicit handle
  //and should already be built. This signature exists so that
  //the compiler doesn't try to generate un-used code that will
  //try and run Algorithm::ScanExclusive on an implicit array which will
  //cause a compile time failure.
}

template<typename ArrayHandleIndices,
         typename ArrayHandleOffsets,
         typename DeviceAdapterTag>
void buildIndexOffsets(const ArrayHandleIndices& numIndices,
                       ArrayHandleOffsets offsets,
                       DeviceAdapterTag tag)
{
  typedef vtkm::cont::internal::IsWriteableArrayHandle<ArrayHandleOffsets,
                                                       DeviceAdapterTag> IsWriteable;
  buildIndexOffsets(numIndices, offsets, tag, typename IsWriteable::type());
}


template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename NumIndicesStorageTag    = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG,
         typename IndexOffsetStorageTag   = VTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  typedef vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag> ShapeArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag> NumIndicesArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> ConnectivityArrayType;
  typedef vtkm::cont::ArrayHandle<vtkm::Id, IndexOffsetStorageTag> IndexOffsetArrayType;

  ShapeArrayType Shapes;
  NumIndicesArrayType NumIndices;
  ConnectivityArrayType Connectivity;
  mutable IndexOffsetArrayType IndexOffsets;

  bool ElementsValid;
  mutable bool IndexOffsetsValid;

  VTKM_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false), IndexOffsetsValid(false) {  }

  VTKM_CONT
  vtkm::Id GetNumberOfElements() const {
    return this->Shapes.GetNumberOfValues();
  }

  VTKM_CONT
  void ReleaseResourcesExecution() {
    this->Shapes.ReleaseResourcesExecution();
    this->NumIndices.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->IndexOffsets.ReleaseResourcesExecution();
  }

  template<typename Device>
  VTKM_CONT
  void BuildIndexOffsets(Device) const
  {
    VTKM_ASSERT(this->ElementsValid);

    if(!this->IndexOffsetsValid)
    {
      buildIndexOffsets(this->NumIndices,
                        this->IndexOffsets,
                        Device());
      this->IndexOffsetsValid = true;
    }
  }

  VTKM_CONT
  void BuildIndexOffsets(vtkm::cont::DeviceAdapterTagError) const
  {
    if (!this->IndexOffsetsValid)
    {
      throw vtkm::cont::ErrorBadType(
            "Cannot build indices using the error device. Must be created previously.");
    }
  }

  VTKM_CONT
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
    out << std::endl;
  }
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ConnectivityExplicitInternals_h
