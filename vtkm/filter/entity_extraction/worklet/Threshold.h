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

  template <typename CellSetType, typename ValueType, typename StorageType, typename UnaryPredicate>
  vtkm::cont::CellSetPermutation<CellSetType> RunImpl(
    const CellSetType& cellSet,
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
    vtkm::cont::Field::Association fieldType,
    const UnaryPredicate& predicate,
    bool allPointsMustPass,
    bool invert)
  {
    using OutputType = vtkm::cont::CellSetPermutation<CellSetType>;

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

    if (invert)
    {
      vtkm::cont::Algorithm::Copy(
        vtkm::cont::make_ArrayHandleTransform(passFlags, vtkm::LogicalNot{}), passFlags);
    }

    vtkm::cont::Algorithm::CopyIf(
      vtkm::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()), passFlags, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet);
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
    vtkm::cont::UnknownCellSet output;
    CastAndCall(cellSet, [&](auto concrete) {
      output = vtkm::worklet::CellDeepCopy::Run(
        this->RunImpl(concrete, field, fieldType, predicate, allPointsMustPass, invert));
    });
    return output;
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetValidCellIds() const { return this->ValidCellIds; }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Threshold_h
