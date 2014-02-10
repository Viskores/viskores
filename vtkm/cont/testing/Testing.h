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
#ifndef vtkm_cont_testing_Testing_h
#define vtkm_cont_testing_Testing_h

#include <vtkm/cont/Error.h>

#include <vtkm/testing/Testing.h>

namespace vtkm {
namespace cont {
namespace testing {

struct Testing
{
public:
  template<class Func>
  static VTKM_CONT_EXPORT int Run(Func function)
  {
    try
      {
      function();
      }
    catch (vtkm::testing::Testing::TestFailure error)
      {
      std::cout << "***** Test failed @ "
                << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
      }
    catch (vtkm::cont::Error error)
      {
      std::cout << "***** Uncaught VTKm exception thrown." << std::endl
                << error.GetMessage() << std::endl;
      return 1;
      }
    catch (...)
      {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
      }
    return 0;
  }
};

}
}
} // namespace vtkm::cont::testing

#endif //vtkm_cont_internal_Testing_h
