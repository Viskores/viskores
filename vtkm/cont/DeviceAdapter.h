//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DeviceAdapter_h
#define vtk_m_cont_DeviceAdapter_h

// These are listed in non-alphabetical order because this is the conceptual
// order in which the sub-files are loaded.  (But the compile should still
// succeed of the order is changed.)  Turn off formatting to keep the order.

// clang-format off
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DeviceAdapterTag.h>
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
/// vtkm/cont/DeviceAdapter.h header file will import all device adapters
/// appropriate for the current compile environment.
///
/// \li \c vtkm::cont::DeviceAdapterTagSerial Runs all algorithms in serial. Can be
/// helpful for debugging.
/// \li \c vtkm::cont::DeviceAdapterTagCuda Dispatches and runs algorithms on a GPU
/// using CUDA.  Must be compiling with a CUDA compiler (nvcc).
/// \li \c vtkm::cont::DeviceAdapterTagOpenMP Dispatches an algorithm over multiple
/// CPU cores using OpenMP compiler directives.  Must be compiling with an
/// OpenMP-compliant compiler with OpenMP pragmas enabled.
/// \li \c vtkm::cont::DeviceAdapterTagTBB Dispatches and runs algorithms on multiple
/// threads using the Intel Threading Building Blocks (TBB) libraries. Must
/// have the TBB headers available and the resulting code must be linked with
/// the TBB libraries.
///
/// To execute algorithms on any device, see Algorithm.h which allows
/// for abitrary device execution.
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
