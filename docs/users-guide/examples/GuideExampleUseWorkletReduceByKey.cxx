//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/Algorithm.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayRangeCompute.h>

#include <viskores/Math.h>
#include <viskores/Range.h>

#include <viskores/cont/testing/Testing.h>

namespace viskores
{
namespace worklet
{

////
//// BEGIN-EXAMPLE BinScalars
////
class BinScalars
{
public:
  VISKORES_EXEC_CONT
  BinScalars(const viskores::Range& range, viskores::Id numBins)
    : Range(range)
    , NumBins(numBins)
  {
  }

  VISKORES_EXEC_CONT
  BinScalars(const viskores::Range& range, viskores::Float64 tolerance)
    : Range(range)
  {
    this->NumBins = viskores::Id(this->Range.Length() / tolerance) + 1;
  }

  VISKORES_EXEC_CONT
  viskores::Id GetBin(viskores::Float64 value) const
  {
    viskores::Float64 ratio = (value - this->Range.Min) / this->Range.Length();
    viskores::Id bin = viskores::Id(ratio * this->NumBins);
    bin = viskores::Max(bin, viskores::Id(0));
    bin = viskores::Min(bin, this->NumBins - 1);
    return bin;
  }

private:
  viskores::Range Range;
  viskores::Id NumBins;
};
////
//// END-EXAMPLE BinScalars
////

struct CreateHistogram
{
  viskores::cont::Invoker Invoke;

  ////
  //// BEGIN-EXAMPLE IdentifyBins
  ////
  struct IdentifyBins : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn data, FieldOut bins);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    BinScalars Bins;

    VISKORES_CONT
    IdentifyBins(const BinScalars& bins)
      : Bins(bins)
    {
    }

    VISKORES_EXEC
    viskores::Id operator()(viskores::Float64 value) const { return Bins.GetBin(value); }
  };
  ////
  //// END-EXAMPLE IdentifyBins
  ////

  ////
  //// BEGIN-EXAMPLE CountBins
  ////
  struct CountBins : viskores::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, WholeArrayOut binCounts);
    using ExecutionSignature = void(_1, ValueCount, _2);
    using InputDomain = _1;

    template<typename BinCountsPortalType>
    VISKORES_EXEC void operator()(viskores::Id binId,
                                  viskores::IdComponent numValuesInBin,
                                  BinCountsPortalType& binCounts) const
    {
      binCounts.Set(binId, numValuesInBin);
    }
  };
  ////
  //// END-EXAMPLE CountBins
  ////

  template<typename InArrayHandleType>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id> Run(
    const InArrayHandleType& valuesArray,
    viskores::Id numBins)
  {
    VISKORES_IS_ARRAY_HANDLE(InArrayHandleType);

    viskores::Range range =
      viskores::cont::ArrayRangeCompute(valuesArray).ReadPortal().Get(0);
    BinScalars bins(range, numBins);

    ////
    //// BEGIN-EXAMPLE CreateKeysObject
    ////
    viskores::cont::ArrayHandle<viskores::Id> binIds;
    this->Invoke(IdentifyBins(bins), valuesArray, binIds);

    ////
    //// BEGIN-EXAMPLE InvokeCountBins
    ////
    viskores::worklet::Keys<viskores::Id> keys(binIds);
    ////
    //// END-EXAMPLE CreateKeysObject
    ////

    viskores::cont::ArrayHandle<viskores::Id> histogram;
    viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(0, numBins),
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
  struct IdentifyBins : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn data, FieldOut bins);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    BinScalars Bins;

    VISKORES_CONT
    IdentifyBins(const BinScalars& bins)
      : Bins(bins)
    {
    }

    VISKORES_EXEC
    viskores::Id operator()(viskores::Float64 value) const { return Bins.GetBin(value); }
  };

  ////
  //// BEGIN-EXAMPLE AverageBins
  ////
  struct BinAverage : viskores::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys,
                                  ValuesIn originalValues,
                                  ReducedValuesOut averages);
    using ExecutionSignature = _3(_2);
    using InputDomain = _1;

    template<typename OriginalValuesVecType>
    VISKORES_EXEC typename OriginalValuesVecType::ComponentType operator()(
      const OriginalValuesVecType& originalValues) const
    {
      typename OriginalValuesVecType::ComponentType sum = 0;
      for (viskores::IdComponent index = 0;
           index < originalValues.GetNumberOfComponents();
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
  viskores::cont::Invoker Invoke;
  viskores::Id NumBins;

  template<typename InArrayHandleType>
  VISKORES_CONT viskores::cont::ArrayHandle<typename InArrayHandleType::ValueType> Run(
    const InArrayHandleType& inField,
    viskores::Id numBins)
  {
    VISKORES_IS_ARRAY_HANDLE(InArrayHandleType);
    using T = typename InArrayHandleType::ValueType;

    this->NumBins = numBins;

    //// RESUME-EXAMPLE
    viskores::Range range =
      viskores::cont::ArrayRangeCompute(inField).ReadPortal().Get(0);
    BinScalars bins(range, numBins);

    viskores::cont::ArrayHandle<viskores::Id> binIds;
    this->Invoke(IdentifyBins(bins), inField, binIds);

    viskores::worklet::Keys<viskores::Id> keys(binIds);

    viskores::cont::ArrayHandle<T> combinedValues;

    this->Invoke(BinAverage{}, keys, inField, combinedValues);
    ////
    //// END-EXAMPLE CombineSimilarValues
    ////

    return combinedValues;
  }
};

} // namespace worklet
} // namespace viskores

void DoWorkletReduceByKeyTest()
{
  viskores::Float64 valueBuffer[52] = {
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

  viskores::cont::ArrayHandle<viskores::Float64> valuesArray =
    viskores::cont::make_ArrayHandle(valueBuffer, 52, viskores::CopyFlag::On);

  viskores::cont::ArrayHandle<viskores::Id> histogram =
    viskores::worklet::CreateHistogram().Run(valuesArray, 10);

  std::cout << "Histogram: " << std::endl;
  viskores::cont::printSummary_ArrayHandle(histogram, std::cout, true);

  viskores::cont::ArrayHandle<viskores::Float64> combinedArray =
    viskores::worklet::CombineSimilarValues().Run(valuesArray, 60);

  std::cout << "Combined values: " << std::endl;
  viskores::cont::printSummary_ArrayHandle(combinedArray, std::cout, true);
}

int GuideExampleUseWorkletReduceByKey(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoWorkletReduceByKeyTest, argc, argv);
}
