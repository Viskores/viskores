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
#ifndef vtkm_cont_ErrorControl_h
#define vtkm_cont_ErrorControl_h

#include <vtkm/cont/Error.h>

namespace vtkm {
namespace cont {

/// The superclass of all exceptions thrown from within the VTKm control
/// environment.
///
class ErrorControl : public vtkm::cont::Error
{
protected:
  ErrorControl() { }
  ErrorControl(const std::string message) : Error(message) { }
};

}
} // namespace vtkm::cont

#endif //vtkm_cont_ErrorControl_h
