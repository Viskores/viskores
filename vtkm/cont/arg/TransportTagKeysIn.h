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
