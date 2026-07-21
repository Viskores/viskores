//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#ifndef viskores_worklet_LogValues_h
#define viskores_worklet_LogValues_h

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace detail
{
template <viskores::FloatDefault LogFunc(viskores::FloatDefault)>
class LogFunWorklet : public viskores::worklet::WorkletMapField
{
  const viskores::FloatDefault MinValue;

public:
  VISKORES_CONT
  LogFunWorklet(const viskores::FloatDefault minValue)
    : MinValue(minValue)
  {
  }

  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);

  template <typename T>
  VISKORES_EXEC void operator()(const T& value, viskores::FloatDefault& log_value) const
  {
    viskores::FloatDefault f_value = static_cast<viskores::FloatDefault>(value);
    f_value = viskores::Max(MinValue, f_value);
    log_value = LogFunc(f_value);
  }
}; //class LogFunWorklet
}
}
} // namespace viskores::worklet

#endif // viskores_worklet_LogValues_h
