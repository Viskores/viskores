//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_ErrorExecution_h
#define viskores_cont_ErrorExecution_h

#include <viskores/cont/Error.h>

namespace viskores
{
namespace cont
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown in the control environment whenever an error occurs in
/// the execution environment.
///
class VISKORES_ALWAYS_EXPORT ErrorExecution : public viskores::cont::Error
{
public:
  ErrorExecution(const std::string& message)
    : Error(message, true)
  {
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace viskores::cont

#endif //viskores_cont_ErrorExecution_h
