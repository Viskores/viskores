//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagKeysIn_h
#define vtk_m_cont_arg_TransportTagKeysIn_h

#include <vtkm/cont/arg/Transport.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for keys in a reduce by key.
///
/// \c TransportTagKeysIn is a tag used with the \c Transport class to
/// transport vtkm::worklet::Keys objects for the input domain of a
/// reduce by keys worklet.
///
struct TransportTagKeysIn
{
};

// Specialization of Transport class for TransportTagKeysIn is implemented in
// vtkm/worklet/Keys.h. That class is not accessible from here due to VTK-m
// package dependencies.
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagKeysIn_h
