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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ErrorControlAssert_h
#define vtk_m_cont_ErrorControlAssert_h

#include <vtkm/Types.h>
#include <vtkm/cont/ErrorControl.h>

#include <sstream>

namespace vtkm {
namespace cont {

/// This error is thrown whenever VTKM_ASSERT_CONT fails.
///
class ErrorControlAssert : public vtkm::cont::ErrorControl
{
public:
  ErrorControlAssert(const std::string &file,
                     vtkm::Id line,
                     const std::string &condition)
    : ErrorControl(), File(file), Line(line), Condition(condition)
  {
    std::stringstream message;
    message << this->File << ":" << this->Line
            << ": Assert Failed (" << this->Condition << ")";
    this->SetMessage(message.str());
  }

private:
  std::string File;
  vtkm::Id Line;
  std::string Condition;
};

}
}

#endif //vtk_m_cont_ErrorControlAssert_h
