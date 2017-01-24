//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagReducedValuesIn_h
#define vtk_m_cont_arg_TransportTagReducedValuesIn_h

#include <vtkm/cont/arg/Transport.h>

namespace vtkm {
namespace cont {
namespace arg {

/// \brief \c Transport tag for input values in a reduce by key.
///
/// \c TransportTagReducedValuesIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input values that correspond to arrays
/// of reduced values. The values are passed 1-to-1 to the worklet invocations.
///
struct TransportTagReducedValuesIn {  };

// Specialization of Transport class for TransportTagReducedValuesIn is
// implemented in vtkm/worklet/Keys.h. That class is not accessible from here
// due to VTK-m package dependencies.

}
}
}

#endif //vtk_m_cont_arg_TransportTagReducedValuesIn_h
