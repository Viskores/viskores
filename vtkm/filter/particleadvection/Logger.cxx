//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/filter/particleadvection/Logger.h>

#include <vtkm/thirdparty/diy/diy.h>

namespace vtkm
{
namespace filter
{

std::map<std::string, vtkm::filter::Logger*> vtkm::filter::Logger::Loggers;

Logger::Logger(const std::string& name)
{
  std::stringstream logName;
  logName << name;
#ifdef VTKM_ENABLE_MPI
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  logName << "." << Comm.rank();
#endif
  logName << ".log";

  this->Stream.open(logName.str().c_str(), std::ofstream::out);
  if (!this->Stream.is_open())
    std::cout << "Warning: could not open the vtkh log file\n";
}

Logger::~Logger()
{
  if (this->Stream.is_open())
    this->Stream.close();
}
}
} // vtkm::filter
