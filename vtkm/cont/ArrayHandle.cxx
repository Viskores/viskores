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

#define vtkm_cont_ArrayHandle_cxx
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace cont {

template class VTKM_CONT_EXPORT ArrayHandle<char, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int8, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt8, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int16, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt16, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int32, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt32, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Int64, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::UInt64, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Float32, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle<vtkm::Float64, StorageTagBasic>;

template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Int64,2>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Int32,2>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Float32,2>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Float64,2>, StorageTagBasic>;

template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Int64,3>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Int32,3>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Float32,3>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Float64,3>, StorageTagBasic>;

template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<char,4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<Int8,4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<UInt8,4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Float32,4>, StorageTagBasic>;
template class VTKM_CONT_EXPORT ArrayHandle< vtkm::Vec<vtkm::Float64,4>, StorageTagBasic>;

}
}