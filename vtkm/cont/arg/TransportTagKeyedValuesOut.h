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
#ifndef vtk_m_cont_arg_TransportTagKeyedValuesOut_h
#define vtk_m_cont_arg_TransportTagKeyedValuesOut_h

#include <vtkm/cont/arg/Transport.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input values in a reduce by key.
///
/// \c TransportTagKeyedValuesOut is a tag used with the \c Transport class
/// to transport \c ArrayHandle objects for output values. The values are
/// rearranged and grouped based on the keys they are associated with.
///
struct TransportTagKeyedValuesOut
{
};

// Specialization of Transport class for TransportTagKeyedValuesOut is
// implemented in vtkm/worklet/Keys.h. That class is not accessible from here
// due to VTK-m package dependencies.
}
}
}

#endif //vtk_m_cont_arg_TransportTagKeyedValuesOut_h
