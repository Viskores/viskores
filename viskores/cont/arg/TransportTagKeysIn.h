//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
/// Loads data from a `viskores::worklet::Keys` object. This transport is
/// intended for the input domain of a `viskores::worklet::WorkletReduceByKey`.
/// The returned execution object is of type
/// `viskores::exec::internal::ReduceByKeyLookup`.
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
