//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_internal_WorkletBase_h
#define vtk_m_worklet_internal_WorkletBase_h

#include <vtkm/TopologyElementTag.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/FetchTagExecObject.h>
#include <vtkm/exec/arg/FetchTagWholeCellSetIn.h>
#include <vtkm/exec/arg/InputIndex.h>
#include <vtkm/exec/arg/OutputIndex.h>
#include <vtkm/exec/arg/ThreadIndices.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/arg/VisitIndex.h>
#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagAtomicArray.h>
#include <vtkm/cont/arg/TransportTagBitField.h>
#include <vtkm/cont/arg/TransportTagCellSetIn.h>
#include <vtkm/cont/arg/TransportTagExecObject.h>
#include <vtkm/cont/arg/TransportTagWholeArrayIn.h>
#include <vtkm/cont/arg/TransportTagWholeArrayInOut.h>
#include <vtkm/cont/arg/TransportTagWholeArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagArray.h>
#include <vtkm/cont/arg/TypeCheckTagAtomicArray.h>
#include <vtkm/cont/arg/TypeCheckTagBitField.h>
#include <vtkm/cont/arg/TypeCheckTagCellSet.h>
#include <vtkm/cont/arg/TypeCheckTagExecObject.h>

#include <vtkm/worklet/MaskNone.h>
#include <vtkm/worklet/ScatterIdentity.h>
#include <vtkm/worklet/internal/Placeholders.h>

namespace vtkm
{
namespace worklet
{
namespace internal
{

/// Base class for all worklet classes. Worklet classes are subclasses and a
/// operator() const is added to implement an algorithm in VTK-m. Different
/// worklets have different calling semantics.
///
class VTKM_ALWAYS_EXPORT WorkletBase : public vtkm::exec::FunctorBase
{
public:
  using _1 = vtkm::placeholders::Arg<1>;
  using _2 = vtkm::placeholders::Arg<2>;
  using _3 = vtkm::placeholders::Arg<3>;
  using _4 = vtkm::placeholders::Arg<4>;
  using _5 = vtkm::placeholders::Arg<5>;
  using _6 = vtkm::placeholders::Arg<6>;
  using _7 = vtkm::placeholders::Arg<7>;
  using _8 = vtkm::placeholders::Arg<8>;
  using _9 = vtkm::placeholders::Arg<9>;
  using _10 = vtkm::placeholders::Arg<10>;
  using _11 = vtkm::placeholders::Arg<11>;
  using _12 = vtkm::placeholders::Arg<12>;
  using _13 = vtkm::placeholders::Arg<13>;
  using _14 = vtkm::placeholders::Arg<14>;
  using _15 = vtkm::placeholders::Arg<15>;
  using _16 = vtkm::placeholders::Arg<16>;
  using _17 = vtkm::placeholders::Arg<17>;
  using _18 = vtkm::placeholders::Arg<18>;
  using _19 = vtkm::placeholders::Arg<19>;
  using _20 = vtkm::placeholders::Arg<20>;

  /// \c ExecutionSignature tag for getting the work index.
  ///
  using WorkIndex = vtkm::exec::arg::WorkIndex;

  /// \c ExecutionSignature tag for getting the input index.
  ///
  using InputIndex = vtkm::exec::arg::InputIndex;

  /// \c ExecutionSignature tag for getting the output index.
  ///
  using OutputIndex = vtkm::exec::arg::OutputIndex;

  /// \c ExecutionSignature tag for getting the thread indices.
  ///
  using ThreadIndices = vtkm::exec::arg::ThreadIndices;

  /// \c ExecutionSignature tag for getting the visit index.
  ///
  using VisitIndex = vtkm::exec::arg::VisitIndex;

  /// \c ExecutionSignature tag for getting the device adapter tag.
  ///
  struct Device : vtkm::exec::arg::ExecutionSignatureTagBase
  {
    // INDEX 0 (which is an invalid parameter index) is reserved to mean the device adapter tag.
    static constexpr vtkm::IdComponent INDEX = 0;
    using AspectTag = vtkm::exec::arg::AspectTagDefault;
  };

  /// \c ControlSignature tag for execution object inputs.
  struct ExecObject : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagExecObject;
    using TransportTag = vtkm::cont::arg::TransportTagExecObject;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  using InputDomain = _1;

  /// All worklets must define their scatter operation. The scatter defines
  /// what output each input contributes to. The default scatter is the
  /// identity scatter (1-to-1 input to output).
  using ScatterType = vtkm::worklet::ScatterIdentity;

  /// All worklets must define their mask operation. The mask defines which
  /// outputs are generated. The default mask is the none mask, which generates
  /// everything in the output domain.
  using MaskType = vtkm::worklet::MaskNone;

  /// \c ControlSignature tag for whole input arrays.
  ///
  /// The \c WholeArrayIn control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted
  /// to an \c ArrayPortal object and passed to the appropriate worklet
  /// operator argument with one of the default args.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct WholeArrayIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole output arrays.
  ///
  /// The \c WholeArrayOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// an \c ArrayPortal object and passed to the appropriate worklet operator
  /// argument with one of the default args. Care should be taken to not write
  /// a value in one instance that will be overridden by another entry.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct WholeArrayOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input/output arrays.
  ///
  /// The \c WholeArrayOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// an \c ArrayPortal object and passed to the appropriate worklet operator
  /// argument with one of the default args. Care should be taken to not write
  /// a value in one instance that will be read by or overridden by another
  /// entry.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct WholeArrayInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input/output arrays.
  ///
  /// The \c AtomicArrayInOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// a \c vtkm::exec::AtomicArray object and passed to the appropriate worklet
  /// operator argument with one of the default args. The provided atomic
  /// operations can be used to resolve concurrency hazards, but have the
  /// potential to slow the program quite a bit.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct AtomicArrayInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagAtomicArray;
    using TransportTag = vtkm::cont::arg::TransportTagAtomicArray;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tags for whole BitFields.
  ///
  /// When a BitField is passed in to a worklet expecting this ControlSignature
  /// type, the appropriate BitPortal is generated and given to the worklet's
  /// execution.
  ///
  /// Be aware that this data structure is especially prone to race conditions,
  /// so be sure to use the appropriate atomic methods when necessary.
  /// @{
  ///
  struct BitFieldIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagBitField;
    using TransportTag = vtkm::cont::arg::TransportTagBitFieldIn;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };
  struct BitFieldOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagBitField;
    using TransportTag = vtkm::cont::arg::TransportTagBitFieldOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };
  struct BitFieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagBitField;
    using TransportTag = vtkm::cont::arg::TransportTagBitFieldInOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };
  /// @}

  /// \c ControlSignature tag for whole input topology.
  ///
  /// The \c WholeCellSetIn control signature tag specifies an \c CellSet
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// a \c vtkm::exec::Connectivity* object and passed to the appropriate worklet
  /// operator argument with one of the default args. This can be used to
  /// global lookup for arbitrary topology information

  using Point = vtkm::TopologyElementTagPoint;
  using Cell = vtkm::TopologyElementTagCell;
  using Edge = vtkm::TopologyElementTagEdge;
  using Face = vtkm::TopologyElementTagFace;
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSet;
    using TransportTag = vtkm::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>;
    using FetchTag = vtkm::exec::arg::FetchTagWholeCellSetIn;
  };

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const T& globalThreadIndexOffset = 0) const
  {
    vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};
}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_WorkletBase_h
