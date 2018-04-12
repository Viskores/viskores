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
#ifndef vtk_m_cont_testing_Testing_h
#define vtk_m_cont_testing_Testing_h

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/Error.h>
#include <vtkm/testing/Testing.h>
#include <vtkm/thirdparty/diy/Configure.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/mpi.hpp)
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace testing
{

struct Testing
{
public:
  template <class Func>
  static VTKM_CONT int Run(Func function)
  {
    try
    {
      function();
    }
    catch (vtkm::testing::Testing::TestFailure& error)
    {
      std::cout << "***** Test failed @ " << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (vtkm::cont::Error& error)
    {
      std::cout << "***** Uncaught VTKm exception thrown." << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (std::exception& error)
    {
      std::cout << "***** STL exception throw." << std::endl << error.what() << std::endl;
    }
    catch (...)
    {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
    }
    return 0;
  }
};

struct Environment
{
  VTKM_CONT Environment(int* argc, char*** argv)
  {
#if defined(VTKM_ENABLE_MPI)
    int provided_threading;
    MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided_threading);

    // set the global communicator to use in VTKm.
    diy::mpi::communicator comm(MPI_COMM_WORLD);
    vtkm::cont::EnvironmentTracker::SetCommunicator(comm);
#else
    (void) argc;
    (void) argv;
#endif
  }

  VTKM_CONT ~Environment()
  {
#if defined(VTKM_ENABLE_MPI)
    MPI_Finalize();
#endif
  }
};

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_internal_Testing_h
