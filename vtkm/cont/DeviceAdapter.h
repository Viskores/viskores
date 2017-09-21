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
#ifndef vtk_m_cont_DeviceAdapter_h
#define vtk_m_cont_DeviceAdapter_h

// These are listed in non-alphabetical order because this is the conceptual
// order in which the sub-files are loaded.  (But the compile should still
// succeed of the order is changed.)  Turn off formatting to keep the order.

// clang-format off
#include <vtkm/cont/internal/DeviceAdapterDefaultSelection.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>
// clang-format on

namespace vtkm
{
namespace cont
{

#ifdef VTKM_DOXYGEN_ONLY
/// \brief A tag specifying the interface between the control and execution environments.
///
/// A DeviceAdapter tag specifies a set of functions and classes that provide
/// mechanisms to run algorithms on a type of parallel device. The tag
/// DeviceAdapterTag___ does not actually exist. Rather, this documentation is
/// provided to describe the interface for a DeviceAdapter. Loading the
/// vtkm/cont/DeviceAdapter.h header file will set a default device adapter
/// appropriate for the current compile environment. You can specify the
/// default device adapter by first setting the \c VTKM_DEVICE_ADAPTER macro.
/// Valid values for \c VTKM_DEVICE_ADAPTER are the following:
///
/// \li \c VTKM_DEVICE_ADAPTER_SERIAL Runs all algorithms in serial. Can be
/// helpful for debugging.
/// \li \c VTKM_DEVICE_ADAPTER_CUDA Dispatches and runs algorithms on a GPU
/// using CUDA.  Must be compiling with a CUDA compiler (nvcc).
/// \li \c VTKM_DEVICE_ADAPTER_OPENMP Dispatches an algorithm over multiple
/// CPU cores using OpenMP compiler directives.  Must be compiling with an
/// OpenMP-compliant compiler with OpenMP pragmas enabled.
/// \li \c VTKM_DEVICE_ADAPTER_TBB Dispatches and runs algorithms on multiple
/// threads using the Intel Threading Building Blocks (TBB) libraries. Must
/// have the TBB headers available and the resulting code must be linked with
/// the TBB libraries.
///
/// See the ArrayManagerExecution.h and DeviceAdapterAlgorithm.h files for
/// documentation on all the functions and classes that must be
/// overloaded/specialized to create a new device adapter.
///
struct DeviceAdapterTag___
{
};
#endif //VTKM_DOXYGEN_ONLY

namespace internal
{

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_DeviceAdapter_h
