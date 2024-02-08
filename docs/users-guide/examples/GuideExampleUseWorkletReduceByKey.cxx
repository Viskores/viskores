//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayRangeCompute.h>

#include <vtkm/Math.h>
#include <vtkm/Range.h>

#include <vtkm/cont/testing/Testing.h>

namespace vtkm
{
namespace worklet
{

////
//// BEGIN-EXAMPLE BinScalars
////
class BinScalars
{
public:
  VTKM_EXEC_CONT
  BinScalars(const vtkm::Range& range, vtkm::Id numBins)
    : Range(range)
    , NumBins(numBins)
  {
  }

  VTKM_EXEC_CONT
  BinScalars(const vtkm::Range& range, vtkm::Float64 tolerance)
    : Range(range)
  {
    this->NumBins = vtkm::Id(this->Range.Length() / tolerance) + 1;
  }

  VTKM_EXEC_CONT
  vtkm::Id GetBin(vtkm::Float64 value) const
  {
    vtkm::Float64 ratio = (value - this->Range.Min) / this->Range.Length();
    vtkm::Id bin = vtkm::Id(ratio * this->NumBins);
    bin = vtkm::Max(bin, vtkm::Id(0));
    bin = vtkm::Min(bin, this->NumBins - 1);
    return bin;
  }

private:
  vtkm::Range Range;
  vtkm::Id NumBins;
};
////
//// END-EXAMPLE BinScalars
////

struct CreateHistogram
{
  vtkm::cont::Invoker Invoke;

  ////
  //// BEGIN-EXAMPLE IdentifyBins
  ////
  struct IdentifyBins : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn data, FieldOut bins);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    BinScalars Bins;

    VTKM_CONT
    IdentifyBins(const BinScalars& bins)
      : Bins(bins)
    {
    }

    VTKM_EXEC
    vtkm::Id operator()(vtkm::Float64 value) const { return Bins.GetBin(value); }
  };
  ////
  //// END-EXAMPLE IdentifyBins
  ////

  ////
  //// BEGIN-EXAMPLE CountBins
  ////
  struct CountBins : vtkm::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, WholeArrayOut binCounts);
    using ExecutionSignature = void(_1, ValueCount, _2);
    using InputDomain = _1;

    template<typename BinCountsPortalType>
    VTKM_EXEC void operator()(vtkm::Id binId,
                              vtkm::IdComponent numValuesInBin,
                              BinCountsPortalType& binCounts) const
    {
      binCounts.Set(binId, numValuesInBin);
    }
  };
  ////
  //// END-EXAMPLE CountBins
  ////

  template<typename InArrayHandleType>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> Run(const InArrayHandleType& valuesArray,
                                                  vtkm::Id numBins)
  {
    VTKM_IS_ARRAY_HANDLE(InArrayHandleType);

    vtkm::Range range = vtkm::cont::ArrayRangeCompute(valuesArray).ReadPortal().Get(0);
    BinScalars bins(range, numBins);

    ////
    //// BEGIN-EXAMPLE CreateKeysObject
    ////
    vtkm::cont::ArrayHandle<vtkm::Id> binIds;
    this->Invoke(IdentifyBins(bins), valuesArray, binIds);

    ////
    //// BEGIN-EXAMPLE InvokeCountBins
    ////
    vtkm::worklet::Keys<vtkm::Id> keys(binIds);
    ////
    //// END-EXAMPLE CreateKeysObject
    ////

    vtkm::cont::ArrayHandle<vtkm::Id> histogram;
    vtkm::cont::Algorithm::Copy(vtkm::cont::make_ArrayHandleConstant(0, numBins),
                                histogram);

    this->Invoke(CountBins{}, keys, histogram);
    ////
    //// END-EXAMPLE InvokeCountBins
    ////

    return histogram;
  }
};

struct CombineSimilarValues
{
  ////
  //// BEGIN-EXAMPLE CombineSimilarValues
  ////
  struct IdentifyBins : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn data, FieldOut bins);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    BinScalars Bins;

    VTKM_CONT
    IdentifyBins(const BinScalars& bins)
      : Bins(bins)
    {
    }

    VTKM_EXEC
    vtkm::Id operator()(vtkm::Float64 value) const { return Bins.GetBin(value); }
  };

  ////
  //// BEGIN-EXAMPLE AverageBins
  ////
  struct BinAverage : vtkm::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys,
                                  ValuesIn originalValues,
                                  ReducedValuesOut averages);
    using ExecutionSignature = _3(_2);
    using InputDomain = _1;

    template<typename OriginalValuesVecType>
    VTKM_EXEC typename OriginalValuesVecType::ComponentType operator()(
      const OriginalValuesVecType& originalValues) const
    {
      typename OriginalValuesVecType::ComponentType sum = 0;
      for (vtkm::IdComponent index = 0; index < originalValues.GetNumberOfComponents();
           index++)
      {
        sum = sum + originalValues[index];
      }
      return sum / originalValues.GetNumberOfComponents();
    }
  };
  ////
  //// END-EXAMPLE AverageBins
  ////

  //
  // Later in the associated Filter class...
  //

  //// PAUSE-EXAMPLE
  vtkm::cont::Invoker Invoke;
  vtkm::Id NumBins;

  template<typename InArrayHandleType>
  VTKM_CONT vtkm::cont::ArrayHandle<typename InArrayHandleType::ValueType> Run(
    const InArrayHandleType& inField,
    vtkm::Id numBins)
  {
    VTKM_IS_ARRAY_HANDLE(InArrayHandleType);
    using T = typename InArrayHandleType::ValueType;

    this->NumBins = numBins;

    //// RESUME-EXAMPLE
    vtkm::Range range = vtkm::cont::ArrayRangeCompute(inField).ReadPortal().Get(0);
    BinScalars bins(range, numBins);

    vtkm::cont::ArrayHandle<vtkm::Id> binIds;
    this->Invoke(IdentifyBins(bins), inField, binIds);

    vtkm::worklet::Keys<vtkm::Id> keys(binIds);

    vtkm::cont::ArrayHandle<T> combinedValues;

    this->Invoke(BinAverage{}, keys, inField, combinedValues);
    ////
    //// END-EXAMPLE CombineSimilarValues
    ////

    return combinedValues;
  }
};

} // namespace worklet
} // namespace vtkm

void DoWorkletReduceByKeyTest()
{
  vtkm::Float64 valueBuffer[52] = {
    3.568802153, 2.569206462, 3.369894868, 3.05340034,  3.189916551, 3.021942381,
    2.146410817, 3.369740333, 4.034567259, 4.338713076, 3.120994598, 2.448715191,
    2.296382644, 2.26980974,  3.610078207, 1.590680158, 3.820785828, 3.291345926,
    2.888019663, 3.653905802, 2.670358133, 2.937653941, 4.442601425, 2.041263284,
    1.877340015, 3.791255574, 2.064493023, 3.850323345, 5.093379708, 2.303811786,
    3.473126279, 3.284056471, 2.892983179, 2.044613478, 2.892095399, 2.317791183,
    2.885776085, 3.048176117, 2.973250571, 2.034521666, 2.524893933, 2.558984374,
    3.928186666, 3.735811764, 3.527816797, 3.293986156, 2.418477242, 3.63490149,
    4.500478394, 3.762309474, 0.0,         6.0
  };

  vtkm::cont::ArrayHandle<vtkm::Float64> valuesArray =
    vtkm::cont::make_ArrayHandle(valueBuffer, 52, vtkm::CopyFlag::On);

  vtkm::cont::ArrayHandle<vtkm::Id> histogram =
    vtkm::worklet::CreateHistogram().Run(valuesArray, 10);

  std::cout << "Histogram: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(histogram, std::cout, true);

  vtkm::cont::ArrayHandle<vtkm::Float64> combinedArray =
    vtkm::worklet::CombineSimilarValues().Run(valuesArray, 60);

  std::cout << "Combined values: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(combinedArray, std::cout, true);
}

int GuideExampleUseWorkletReduceByKey(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoWorkletReduceByKeyTest, argc, argv);
}
