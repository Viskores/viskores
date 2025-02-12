//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_arg_TransportTagKeysIn_h
#define viskores_cont_arg_TransportTagKeysIn_h

#include <viskores/cont/arg/Transport.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for keys in a reduce by key.
///
/// \c TransportTagKeysIn is a tag used with the \c Transport class to
/// transport viskores::worklet::Keys objects for the input domain of a
/// reduce by keys worklet.
///
struct TransportTagKeysIn
{
};

// Specialization of Transport class for TransportTagKeysIn is implemented in
// viskores/worklet/Keys.h. That class is not accessible from here due to Viskores
// package dependencies.
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagKeysIn_h
