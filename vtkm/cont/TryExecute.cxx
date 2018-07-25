//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace cont
{
namespace detail
{

void HandleTryExecuteException(vtkm::cont::DeviceAdapterId deviceId,
                               const std::string& name,
                               vtkm::cont::RuntimeDeviceTracker& tracker)
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (vtkm::cont::ErrorBadAllocation& e)
  {
    std::cerr << "caught ErrorBadAllocation " << e.GetMessage() << std::endl;
    //currently we only consider OOM errors worth disabling a device for
    //than we fallback to another device
    tracker.ReportAllocationFailure(deviceId, name, e);
  }
  catch (vtkm::cont::ErrorBadDevice& e)
  {
    std::cerr << "caught ErrorBadDevice: " << e.GetMessage() << std::endl;
    tracker.ReportBadDeviceFailure(deviceId, name, e);
  }
  catch (vtkm::cont::ErrorBadType& e)
  {
    //should bad type errors should stop the execution, instead of
    //deferring to another device adapter?
    std::cerr << "caught ErrorBadType : " << e.GetMessage() << std::endl;
  }
  catch (vtkm::cont::ErrorBadValue& e)
  {
    //should bad value errors should stop the filter, instead of deferring
    //to another device adapter?
    std::cerr << "caught ErrorBadValue : " << e.GetMessage() << std::endl;
  }
  catch (vtkm::cont::Error& e)
  {
    if (e.GetIsDeviceIndependent())
    {
      // re-throw the exception as it's a device-independent exception.
      throw;
    }
    //general errors should be caught and let us try the next device adapter.
    std::cerr << "exception is: " << e.GetMessage() << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "caught standard exception: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception caught" << std::endl;
  }
}
}
}
}
