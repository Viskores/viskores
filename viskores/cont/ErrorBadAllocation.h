//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_ErrorBadAllocation_h
#define viskores_cont_ErrorBadAllocation_h

#include <viskores/cont/Error.h>

namespace viskores
{
namespace cont
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when Viskores attempts to manipulate memory that it should
/// not.
///
class VISKORES_ALWAYS_EXPORT ErrorBadAllocation : public Error
{
public:
  ErrorBadAllocation(const std::string& message)
    : Error(message)
  {
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace viskores::cont

#endif //viskores_cont_ErrorBadAllocation_h
