//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_internal_ArrayGetMonotonicity_h
#define viskores_cont_internal_ArrayGetMonotonicity_h

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

class UnknownArrayHandle;

namespace internal
{

enum class ArrayMonotonicity
{
  NotMonotonic,
  Increasing,
  Decreasing
};

/// Classifies an array as strictly increasing, strictly decreasing, or not monotonic.
/// Repeated values and direction changes are not monotonic.
/// Arrays with fewer than two values are classified as increasing.
VISKORES_CONT_EXPORT ArrayMonotonicity
ArrayGetStrictMonotonicity(const viskores::cont::UnknownArrayHandle& input);

} // namespace internal
} // namespace cont
} // namespace viskores

#endif // viskores_cont_internal_ArrayGetMonotonicity_h
