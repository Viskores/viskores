//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_arg_Transport_h
#define viskores_cont_arg_Transport_h

#include <viskores/Types.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief Class for transporting from the control to the execution environment.
///
/// The \c Transport class is used to transport data of a certain type from the
/// control environment to the execution environment. It is used internally in
/// Viskores's dispatch mechanism.
///
/// \c Transport is a templated class with three arguments. The first argument
/// is a tag declaring the mechanism of transport. The second argument is the
/// type of data to transport. The third argument is device adapter tag for
/// the device to move the data to.
///
/// There is no generic implementation of \c Transport. There are partial
/// specializations of \c Transport for each mechanism supported. If you get a
/// compiler error about an incomplete type for \c Transport, it means you used
/// an invalid \c TransportTag or it is an invalid combination of data type or
/// device adapter.
///
template <typename TransportTag, typename ContObjectType, typename DeviceAdapterTag>
struct Transport
#ifdef VISKORES_DOXYGEN_ONLY
{
  /// \brief The type used in the execution environment.
  ///
  /// All \c Transport specializations are expected to declare a type named \c
  /// ExecObjectType that is the object type used in the execution environment.
  /// For example, for an \c ArrayHandle, the \c ExecObjectType is the portal
  /// used in the execution environment.
  ///
  using ExecObjectType = typename ContObjectType::ReadPortalType;

  /// \brief Send data to the execution environment.
  ///
  /// All `Transport` specializations are expected to have a constant
  /// parenthesis operator that takes the data in the control environment and
  /// returns an object that is accessible in the execution environment.
  ///
  /// @param object The control-side object that must be transferred to the
  ///   device indicated by the `DeviceAdapterTag` template parameter of this
  ///   struct.
  /// @param inputDomain A reference to the input domain argument. This might
  ///   have state necessary to establish the execution-side object. For some
  ///   transports, this object can be ignored.
  /// @param inputRange The size of the input domain. This can be used for
  ///   checking the size of the data to ensure it has values for each input.
  /// @param outputRange The size of the output domain. This can be used for
  ///   checking the size of the data to ensure it has has a place for each
  ///   output.
  /// @param token A reference to a token object that is used to generate
  ///   execution-side objects. The token ensures that the execution object
  ///   remains valid while it is still being used.
  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(ContObjectType& object,
                                          const InputDomainType& inputDomain,
                                          viskores::Id inputRange,
                                          viskores::Id outputRange,
                                          viskores::cont::Token& token) const;
};
#else  // VISKORES_DOXYGEN_ONLY
  ;
#endif // VISKORES_DOXYGEN_ONLY
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_Transport_h
