//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TransportTagKeyedValuesIn_h
#define vtk_m_cont_arg_TransportTagKeyedValuesIn_h

#include <vtkm/cont/arg/Transport.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input values in a reduce by key.
///
/// \c TransportTagKeyedValuesIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input values. The values are
/// rearranged and grouped based on the keys they are associated with.
///
struct TransportTagKeyedValuesIn
{
};

// Specialization of Transport class for TransportTagKeyedValuesIn is
// implemented in vtkm/worklet/Keys.h. That class is not accessible from here
// due to VTK-m package dependencies.
}
}
}

#endif //vtk_m_cont_arg_TransportTagKeyedValuesIn_h
