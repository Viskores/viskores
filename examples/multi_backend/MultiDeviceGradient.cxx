//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtk_m_examples_multibackend_MultiDeviceGradient_cxx

#include "MultiDeviceGradient.h"
#include "MultiDeviceGradient.hxx"

template vtkm::cont::PartitionedDataSet MultiDeviceGradient::PrepareForExecution<
  vtkm::filter::PolicyDefault>(const vtkm::cont::PartitionedDataSet&,
                               const vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>&);
