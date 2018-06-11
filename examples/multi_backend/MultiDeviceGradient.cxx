//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define vtk_m_examples_multibackend_MultiDeviceGradient_cxx

#include "MultiDeviceGradient.h"
#include "MultiDeviceGradient.hxx"

template vtkm::cont::MultiBlock MultiDeviceGradient::PrepareForExecution<
  vtkm::filter::PolicyDefault>(const vtkm::cont::MultiBlock&,
                               const vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>&);
