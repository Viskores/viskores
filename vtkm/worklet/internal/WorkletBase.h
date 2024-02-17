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
#include <vtkm/exec/arg/ThreadIndicesBasic3D.h>
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
#include <vtkm/cont/arg/TypeCheckTagArrayIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayInOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagAtomicArray.h>
#include <vtkm/cont/arg/TypeCheckTagBitField.h>
#include <vtkm/cont/arg/TypeCheckTagCellSet.h>
#include <vtkm/cont/arg/TypeCheckTagExecObject.h>

#include <vtkm/cont/internal/Hints.h>

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

  /// @copydoc vtkm::exec::arg::WorkIndex
  using WorkIndex = vtkm::exec::arg::WorkIndex;

  /// @copydoc vtkm::exec::arg::InputIndex
  using InputIndex = vtkm::exec::arg::InputIndex;

  /// @copydoc vtkm::exec::arg::OutputIndex
  using OutputIndex = vtkm::exec::arg::OutputIndex;

  /// @copydoc vtkm::exec::arg::ThreadIndices
  using ThreadIndices = vtkm::exec::arg::ThreadIndices;

  /// @copydoc vtkm::exec::arg::VisitIndex
  using VisitIndex = vtkm::exec::arg::VisitIndex;

  /// @brief `ExecutionSignature` tag for getting the device adapter tag.
  ///
  /// This tag passes a device adapter tag object. This allows the worklet function
  /// to template on or overload itself based on the type of device that it is
  /// being executed on.
  struct Device : vtkm::exec::arg::ExecutionSignatureTagBase
  {
    // INDEX 0 (which is an invalid parameter index) is reserved to mean the device adapter tag.
    static constexpr vtkm::IdComponent INDEX = 0;
    using AspectTag = vtkm::exec::arg::AspectTagDefault;
  };

  /// @brief `ControlSignature` tag for execution object inputs.
  ///
  /// This tag represents an execution object that is passed directly from the
  /// control environment to the worklet. A `ExecObject` argument expects a subclass
  /// of `vtkm::exec::ExecutionObjectBase`. Subclasses of `vtkm::exec::ExecutionObjectBase`
  /// behave like a factory for objects that work on particular devices. They
  /// do this by implementing a `PrepareForExecution()` method that takes a device
  /// adapter tag and returns an object that works on that device. That device-specific
  /// object is passed directly to the worklet.
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

  /// Worklets can provide hints to the scheduler by defining a `Hints` type that
  /// resolves to a `vtkm::cont::internal::HintList`. The default hint list is empty
  /// so that scheduling uses all defaults.
  using Hints = vtkm::cont::internal::HintList<>;

  /// @brief `ControlSignature` tag for whole input arrays.
  ///
  /// The `WholeArrayIn` control signature tag specifies a `vtkm::cont::ArrayHandle`
  /// passed to the invoke of the worklet. An array portal capable of reading
  /// from any place in the array is given to the worklet.
  ///
  struct WholeArrayIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// @brief `ControlSignature` tag for whole output arrays.
  ///
  /// The `WholeArrayOut` control signature tag specifies an `vtkm::cont::ArrayHandle`
  /// passed to the invoke of the worklet. An array portal capable of writing
  /// to any place in the array is given to the worklet. Developers should take
  /// care when using writable whole arrays as introducing race conditions is possible.
  ///
  struct WholeArrayOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// @brief `ControlSignature` tag for whole input/output arrays.
  ///
  /// The `WholeArrayOut` control signature tag specifies a `vtkm::cont::ArrayHandle`
  /// passed to the invoke of the worklet.  An array portal capable of reading
  /// from or writing to any place in the array is given to the worklet. Developers
  /// should take care when using writable whole arrays as introducing race
  /// conditions is possible.
  ///
  struct WholeArrayInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = vtkm::cont::arg::TransportTagWholeArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagExecObject;
  };

  /// @brief `ControlSignature` tag for whole input/output arrays.
  ///
  /// The `AtomicArrayInOut` control signature tag specifies `vtkm::cont::ArrayHandle`
  /// passed to the invoke of the worklet. A `vtkm::exec::AtomicArray` object capable
  /// of performing atomic operations to the entries in the array is given to the
  /// worklet. Atomic arrays can help avoid race conditions but can slow down the
  /// running of a parallel algorithm.
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

  using Point = vtkm::TopologyElementTagPoint;
  using Cell = vtkm::TopologyElementTagCell;
  using Edge = vtkm::TopologyElementTagEdge;
  using Face = vtkm::TopologyElementTagFace;

  /// @brief `ControlSignature` tag for whole input topology.
  ///
  /// The `WholeCellSetIn` control signature tag specifies a `vtkm::cont::CellSet`
  /// passed to the invoke of the worklet. A connectivity object capable of finding
  /// elements of one type that are incident on elements of a different type. This
  /// can be used to global lookup for arbitrary topology information
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
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const vtkm::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic3D GetThreadIndices(
    vtkm::Id threadIndex1D,
    const vtkm::Id3& threadIndex3D,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    vtkm::Id outIndex = threadToOut.Get(threadIndex1D);
    return vtkm::exec::arg::ThreadIndicesBasic3D(
      threadIndex3D, threadIndex1D, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }
};
}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_WorkletBase_h
