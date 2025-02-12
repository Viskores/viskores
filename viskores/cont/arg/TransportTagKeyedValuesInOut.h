//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_arg_TransportTagKeyedValuesInOut_h
#define viskores_cont_arg_TransportTagKeyedValuesInOut_h

#include <viskores/cont/arg/Transport.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input values in a reduce by key.
///
/// \c TransportTagKeyedValuesInOut is a tag used with the \c Transport class
/// to transport \c ArrayHandle objects for input/output values. The values are
/// rearranged and grouped based on the keys they are associated with.
///
struct TransportTagKeyedValuesInOut
{
};

// Specialization of Transport class for TransportTagKeyedValuesInOut is
// implemented in viskores/worklet/Keys.h. That class is not accessible from here
// due to Viskores package dependencies.
}
}
}

#endif //viskores_cont_arg_TransportTagKeyedValuesInOut_h
