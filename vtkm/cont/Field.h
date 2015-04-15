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
#ifndef vtk_m_Field_h
#define vtk_m_Field_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a logical dimension, or the whole mesh.
///
class Field
{
public:
  Field()
  {
  }

  vtkm::cont::DynamicArrayHandle &GetData()
  {
    return data;
  }

  void SetData(vtkm::cont::ArrayHandle<vtkm::FloatDefault> &newdata)
  {
    data = newdata;
  }

  /*
  void CopyIntoData(vtkm::cont::ArrayHandle<vtkm::FloatDefault> &tmp)
  {
    data = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::
      Copy(tmp, data);
  }
  */
  
private:
  //vtkm::cont::ArrayHandle<vtkm::FloatDefault> data;
  vtkm::cont::DynamicArrayHandle data;
};


} // namespace cont
} // namespace vtkm

#endif //vtk_m_CellType_h
