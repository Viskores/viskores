//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR
#define BOOST_SP_DISABLE_THREADS

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include <vtkm/cont/testing/TestingDeviceAdapter.h>
#include <vtkm/cont/cuda/internal/testing/Testing.h>

namespace vtkm {
namespace cont {
namespace testing {

template<>
struct CopyInto<vtkm::cont::DeviceAdapterTagCuda>
{
  template<typename T, typename StorageTagType>
  VTKM_CONT_EXPORT
  void operator()( vtkm::cont::internal::ArrayManagerExecution<
                    T,
                    StorageTagType,
                    vtkm::cont::DeviceAdapterTagCuda>& manager,
                 T* start)
  {
    typedef vtkm::cont::internal::Storage< T, StorageTagType > StorageType;
    StorageType outputArray;
    std::cout << "now calling RetrieveOutputData: " << std::endl;
    manager.RetrieveOutputData( outputArray );

    vtkm::cont::ArrayPortalToIterators<
                typename StorageType::PortalConstType>
      iterators(outputArray.GetPortalConst());
     std::copy(iterators.GetBegin(), iterators.GetEnd(), start);
  }
};


}
}
}

int UnitTestDeviceAdapterCuda(int, char *[])
{
  int result =  vtkm::cont::testing::TestingDeviceAdapter
      <vtkm::cont::DeviceAdapterTagCuda>::Run();
  return vtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}
