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
                               vtkm::cont::RuntimeDeviceTracker& tracker,
                               const std::string& functorName)
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (vtkm::cont::ErrorBadAllocation& e)
  {
    VTKM_LOG_TRYEXECUTE_DISABLE("Bad allocation (" << e.GetMessage() << ")", functorName, deviceId);
    //currently we only consider OOM errors worth disabling a device for
    //than we fallback to another device
    tracker.ReportAllocationFailure(deviceId, e);
  }
  catch (vtkm::cont::ErrorBadDevice& e)
  {
    VTKM_LOG_TRYEXECUTE_DISABLE("Bad device (" << e.GetMessage() << ")", functorName, deviceId);
    tracker.ReportBadDeviceFailure(deviceId, e);
  }
  catch (vtkm::cont::ErrorBadType& e)
  {
    //should bad type errors should stop the execution, instead of
    //deferring to another device adapter?
    VTKM_LOG_TRYEXECUTE_FAIL("ErrorBadType (" << e.GetMessage() << ")", functorName, deviceId);
  }
  catch (vtkm::cont::ErrorBadValue& e)
  {
    // Should bad values be deferred to another device? Seems unlikely they will succeed.
    // Re-throw instead.
    VTKM_LOG_TRYEXECUTE_FAIL("ErrorBadValue (" << e.GetMessage() << ")", functorName, deviceId);
    throw;
  }
  catch (vtkm::cont::Error& e)
  {
    VTKM_LOG_TRYEXECUTE_FAIL(e.GetMessage(), functorName, deviceId);
    if (e.GetIsDeviceIndependent())
    {
      // re-throw the exception as it's a device-independent exception.
      throw;
    }
  }
  catch (std::exception& e)
  {
    VTKM_LOG_TRYEXECUTE_FAIL(e.what(), functorName, deviceId);
  }
  catch (...)
  {
    VTKM_LOG_TRYEXECUTE_FAIL("Unknown exception", functorName, deviceId);
    std::cerr << "unknown exception caught" << std::endl;
  }
}
}
}
}
