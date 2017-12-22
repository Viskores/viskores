//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_InnerJoin_h
#define vtk_m_worklet_InnerJoin_h



template <typename DeviceAdapter>
class InnerJoin
{
public:
  struct Merge : vtkm::worklet::WorkletMapField
  {
    typedef void ControlSignature(FieldIn<vtkm::Id>,
                                  FieldIn<vtkm::Id>,
                                  FieldIn<vtkm::Id>,
                                  WholeArrayIn<vtkm::Id>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1, _2, _3, VisitIndex, _4, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    VTKM_CONT
    Merge(const ScatterType& scatter)
      : Scatter(scatter)
    {
    }

    template <typename InPortalType>
    VTKM_EXEC void operator()(vtkm::Id key,
                              vtkm::Id value1,
                              vtkm::Id lowerBounds,
                              vtkm::Id visitIndex,
                              const InPortalType& value2,
                              vtkm::Id& keyOut,
                              vtkm::Id& value1Out,
                              vtkm::Id& value2Out) const
    {
      auto v2 = value2.Get(lowerBounds + visitIndex);
      keyOut = key;
      value1Out = value1;
      value2Out = v2;
    }

  private:
    ScatterType Scatter;
  };

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

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

    vtkm::cont::ArrayHandle<vtkm::Id> output_offset;
    Algorithm::ScanExclusive(counts, output_offset);

    vtkm::worklet::ScatterCounting scatter{ counts, DeviceAdapter() };
    Merge merge(scatter);
    vtkm::worklet::DispatcherMapField<Merge, DeviceAdapter> mergeDisp(merge);
    mergeDisp.Invoke(key1, value1, lbs, value2, keyOut, value1Out, value2Out);
  };
};
#endif //vtk_m_worklet_InnerJoin_h
