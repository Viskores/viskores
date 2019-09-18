//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_connectivity_InnerJoin_h
#define vtk_m_worklet_connectivity_InnerJoin_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{
class InnerJoin
{
public:
  struct Merge : vtkm::worklet::WorkletMapField
  {
    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, WholeArrayIn, FieldOut, FieldOut, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, VisitIndex, _4, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    // TODO: type trait for array portal?
    template <typename KeyType, typename ValueType1, typename InPortalType, typename ValueType2>
    VTKM_EXEC void operator()(KeyType key,
                              ValueType1 value1,
                              vtkm::Id lowerBounds,
                              vtkm::Id visitIndex,
                              const InPortalType& value2,
                              vtkm::Id& keyOut,
                              ValueType1& value1Out,
                              ValueType2& value2Out) const
    {
      auto v2 = value2.Get(lowerBounds + visitIndex);
      keyOut = key;
      value1Out = value1;
      value2Out = v2;
    }
  };

  using Algorithm = vtkm::cont::Algorithm;

  // TODO: not mutating input keys and values?
  template <typename Key, typename Value1, typename Value2>
  void Run(vtkm::cont::ArrayHandle<Key>& key1,
           vtkm::cont::ArrayHandle<Value1>& value1,
           vtkm::cont::ArrayHandle<Key>& key2,
           vtkm::cont::ArrayHandle<Value2>& value2,
           vtkm::cont::ArrayHandle<Key>& keyOut,
           vtkm::cont::ArrayHandle<Value1>& value1Out,
           vtkm::cont::ArrayHandle<Value2>& value2Out) const
  {
    Algorithm::SortByKey(key1, value1);
    Algorithm::SortByKey(key2, value2);

    vtkm::cont::ArrayHandle<vtkm::Id> lbs;
    vtkm::cont::ArrayHandle<vtkm::Id> ubs;
    Algorithm::LowerBounds(key2, key1, lbs);
    Algorithm::UpperBounds(key2, key1, ubs);

    vtkm::cont::ArrayHandle<vtkm::Id> counts;
    Algorithm::Transform(ubs, lbs, counts, vtkm::Subtract());

    vtkm::worklet::ScatterCounting scatter{ counts };
    vtkm::worklet::DispatcherMapField<Merge> mergeDisp(scatter);
    mergeDisp.Invoke(key1, value1, lbs, value2, keyOut, value1Out, value2Out);
  }
};
}
}
} // vtkm::worklet::connectivity

#endif //vtk_m_worklet_connectivity_InnerJoin_h
