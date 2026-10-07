//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_arg_TransportTagCellSetIn_h
#define viskores_cont_arg_TransportTagCellSetIn_h

#include <viskores/Types.h>

#include <viskores/cont/CellSet.h>

#include <viskores/cont/arg/Transport.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// Loads data from a `viskores::cont::CellSet` object. `TransportTagCellSetIn`
/// is a templated class with two parameters: the "visit" topology and the
/// "incident" topology. The returned execution object is a connectivity object.
template <typename VisitTopology, typename IncidentTopology>
struct TransportTagCellSetIn
{
};

template <typename VisitTopology,
          typename IncidentTopology,
          typename ContObjectType,
          typename Device>
struct Transport<viskores::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>,
                 ContObjectType,
                 Device>
{
  VISKORES_IS_CELL_SET(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(
    Device(),
    VisitTopology(),
    IncidentTopology(),
    std::declval<viskores::cont::Token&>()));

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(const ContObjectType& object,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    return object.PrepareForInput(Device(), VisitTopology(), IncidentTopology(), token);
  }
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagCellSetIn_h
