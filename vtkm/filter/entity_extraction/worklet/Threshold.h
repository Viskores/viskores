//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_Threshold_h
#define vtkm_m_worklet_Threshold_h

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UncertainCellSet.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/UnaryPredicates.h>

namespace vtkm
{
namespace worklet
{

class Threshold
{
public:
  template <typename UnaryPredicate>
  class ThresholdByPointField : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutCell passFlags);

    using ExecutionSignature = _3(_2, PointCount);

    VTKM_CONT
    ThresholdByPointField()
      : Predicate()
      , AllPointsMustPass()
    {
    }

    VTKM_CONT
    explicit ThresholdByPointField(const UnaryPredicate& predicate, bool allPointsMustPass)
      : Predicate(predicate)
      , AllPointsMustPass(allPointsMustPass)
    {
    }

    template <typename ScalarsVecType>
    VTKM_EXEC bool operator()(const ScalarsVecType& scalars, vtkm::Id count) const
    {
      bool pass = this->AllPointsMustPass ? true : false;
      for (vtkm::IdComponent i = 0; i < count; ++i)
      {
        if (this->AllPointsMustPass)
        {
          pass &= this->Predicate(scalars[i]);
        }
        else
        {
          pass |= this->Predicate(scalars[i]);
        }
      }

      return pass;
    }

  private:
    UnaryPredicate Predicate;
    bool AllPointsMustPass;
  };

  template <typename Operator>
  class CombinePassFlagsWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldInOut, FieldIn);
    using ExecitionSignature = void(_1, _2);

    VTKM_CONT
    explicit CombinePassFlagsWorklet(const Operator& combine)
      : Combine(combine)
    {
    }

    VTKM_EXEC void operator()(bool& combined, bool incoming) const
    {
      combined = this->Combine(combined, incoming);
    }

  private:
    Operator Combine;
  };

  template <typename Operator>
  void CombinePassFlags(const vtkm::cont::ArrayHandle<bool>& passFlagsIn, const Operator& combine)
  {
    if (this->PassFlags.GetNumberOfValues() == 0) // Is initialization needed?
    {
      this->PassFlags = passFlagsIn;
    }
    else
    {
      DispatcherMapField<CombinePassFlagsWorklet<Operator>> dispatcher(
        CombinePassFlagsWorklet<Operator>{ combine });
      dispatcher.Invoke(this->PassFlags, passFlagsIn);
    }
    this->PassFlagsModified = true;
  }

  // special no-op combine operator for combining `PassFlags` results of incremental runs
  struct NoOp
  {
  };

  void CombinePassFlags(const vtkm::cont::ArrayHandle<bool>& passFlagsIn, NoOp)
  {
    this->PassFlags = passFlagsIn;
    this->PassFlagsModified = true;
  }

  /// Incrementally run the worklet on the given parameters. Each run should get the
  /// same `cellSet`. An array of pass/fail flags is maintained internally. The `passFlagsCombine`
  /// operator is used to combine the current result to the incremental results. Finally, use
  /// `GenerateResultCellSet` to get the thresholded cellset.
  template <typename ValueType,
            typename StorageType,
            typename UnaryPredicate,
            typename PassFlagsCombineOp>
  void RunIncremental(const vtkm::cont::UnknownCellSet& cellSet,
                      const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
                      vtkm::cont::Field::Association fieldType,
                      const UnaryPredicate& predicate,
                      bool allPointsMustPass, // only considered when field association is `Points`
                      const PassFlagsCombineOp& passFlagsCombineOp)
  {
    vtkm::cont::ArrayHandle<bool> passFlags;

    switch (fieldType)
    {
      case vtkm::cont::Field::Association::Points:
      {
        using ThresholdWorklet = ThresholdByPointField<UnaryPredicate>;

        ThresholdWorklet worklet(predicate, allPointsMustPass);
        DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);
        break;
      }
      case vtkm::cont::Field::Association::Cells:
      {
        vtkm::cont::Algorithm::Copy(vtkm::cont::make_ArrayHandleTransform(field, predicate),
                                    passFlags);
        break;
      }
      default:
        throw vtkm::cont::ErrorBadValue("Expecting point or cell field.");
    }

    this->CombinePassFlags(passFlags, passFlagsCombineOp);
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetValidCellIds() const
  {
    if (this->PassFlagsModified)
    {
      vtkm::cont::Algorithm::CopyIf(
        vtkm::cont::ArrayHandleIndex(this->PassFlags.GetNumberOfValues()),
        this->PassFlags,
        this->ValidCellIds);
      this->PassFlagsModified = false;
    }
    return this->ValidCellIds;
  }

  vtkm::cont::UnknownCellSet GenerateResultCellSet(const vtkm::cont::UnknownCellSet& cellSet)
  {
    vtkm::cont::UnknownCellSet output;

    CastAndCall(cellSet, [&](auto concrete) {
      output = vtkm::worklet::CellDeepCopy::Run(
        vtkm::cont::make_CellSetPermutation(this->GetValidCellIds(), concrete));
    });

    return output;
  }

  // Invert the results stored in this worklet's state
  void InvertResults()
  {
    vtkm::cont::Algorithm::Copy(
      vtkm::cont::make_ArrayHandleTransform(this->PassFlags, vtkm::LogicalNot{}), this->PassFlags);
    this->PassFlagsModified = true;
  }

  template <typename ValueType, typename StorageType, typename UnaryPredicate>
  vtkm::cont::UnknownCellSet Run(
    const vtkm::cont::UnknownCellSet& cellSet,
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
    vtkm::cont::Field::Association fieldType,
    const UnaryPredicate& predicate,
    bool allPointsMustPass = false, // only considered when field association is `Points`
    bool invert = false)
  {
    this->RunIncremental(cellSet, field, fieldType, predicate, allPointsMustPass, NoOp{});
    if (invert)
    {
      this->InvertResults();
    }
    return this->GenerateResultCellSet(cellSet);
  }

private:
  vtkm::cont::ArrayHandle<bool> PassFlags;

  mutable bool PassFlagsModified = true;
  mutable vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Threshold_h
