//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_exec_arg_FetchTagCellSetIn_h
#define viskores_exec_arg_FetchTagCellSetIn_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting topology information.
///
/// \c FetchTagCellSetIn is a tag used with the \c Fetch class to retrieve
/// values from a topology object.
/// Loads data from a cell set. This fetch is used with worklet topology maps to
/// pull topology information from a cell set. When used with
/// `viskores::exec::arg::AspectTagDefault`, its `Load()` method simply returns
/// the cell shape of the given input cells, and its `Store()` method does
/// nothing. This tag is typically used with the input-domain object, and aspects
/// such as `viskores::exec::arg::AspectTagIncidentElementCount` and
/// `viskores::exec::arg::AspectTagIncidentElementIndices` are used to get more
/// detailed information.
struct FetchTagCellSetIn
{
};

template <typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTagCellSetIn,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC auto Load(const ThreadIndicesType& indices, const ExecObjectType&) const
    -> decltype(indices.GetCellShape())
  {
    return indices.GetCellShape();
  }

  template <typename ThreadIndicesType, typename ValueType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_FetchTagCellSetIn_h
