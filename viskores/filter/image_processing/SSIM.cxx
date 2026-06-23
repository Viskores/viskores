//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/image_processing/SSIM.h>

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>

#include <limits>
#include <type_traits>

namespace
{

struct SSIMStatistics
{
  viskores::Float64 SumPrimary = 0;
  viskores::Float64 SumSecondary = 0;
  viskores::Float64 SumPrimarySquared = 0;
  viskores::Float64 SumSecondarySquared = 0;
  viskores::Float64 SumProduct = 0;
  viskores::Float64 Weight = 0;

  VISKORES_EXEC_CONT void Add(viskores::Float64 primary,
                              viskores::Float64 secondary,
                              viskores::Float64 weight)
  {
    this->SumPrimary += primary * weight;
    this->SumSecondary += secondary * weight;
    this->SumPrimarySquared += primary * primary * weight;
    this->SumSecondarySquared += secondary * secondary * weight;
    this->SumProduct += primary * secondary * weight;
    this->Weight += weight;
  }
};

class SSIMComponentNeighborhood : public viskores::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, _3, Boundary, _4);
  using InputDomain = _1;

  VISKORES_CONT
  explicit SSIMComponentNeighborhood(viskores::IdComponent patchRadius,
                                     viskores::Float64 dynamicRange,
                                     viskores::Float64 k1,
                                     viskores::Float64 k2)
    : PatchRadius(patchRadius)
    , DynamicRange(dynamicRange)
    , K1(k1)
    , K2(k2)
  {
  }

  template <typename PrimaryFieldPortalType, typename SecondaryFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::exec::FieldNeighborhood<PrimaryFieldPortalType>& primary,
    const viskores::exec::FieldNeighborhood<SecondaryFieldPortalType>& secondary,
    const viskores::exec::BoundaryState& boundary,
    viskores::Float64& ssim) const
  {
    SSIMStatistics statistics;
    auto minIndices = boundary.MinNeighborIndices(this->PatchRadius);
    auto maxIndices = boundary.MaxNeighborIndices(this->PatchRadius);
    const viskores::IdComponent radiusSquared = this->PatchRadius * this->PatchRadius;

    for (viskores::IdComponent k = minIndices[2]; k <= maxIndices[2]; ++k)
    {
      for (viskores::IdComponent j = minIndices[1]; j <= maxIndices[1]; ++j)
      {
        for (viskores::IdComponent i = minIndices[0]; i <= maxIndices[0]; ++i)
        {
          const viskores::IdComponent distanceSquared = (i * i) + (j * j) + (k * k);
          if (distanceSquared <= radiusSquared)
          {
            statistics.Add(static_cast<viskores::Float64>(primary.Get(i, j, k)),
                           static_cast<viskores::Float64>(secondary.Get(i, j, k)),
                           this->ComputeWeight(distanceSquared));
          }
        }
      }
    }

    this->Finalize(statistics, ssim);
  }

private:
  VISKORES_EXEC viskores::Float64 ComputeWeight(viskores::IdComponent distanceSquared) const
  {
    if (this->PatchRadius <= 0)
    {
      return 1.0;
    }

    const viskores::Float64 sigma = static_cast<viskores::Float64>(this->PatchRadius) / 3.0;
    return viskores::Exp(-static_cast<viskores::Float64>(distanceSquared) / (2.0 * sigma * sigma));
  }

  VISKORES_EXEC void Finalize(const SSIMStatistics& statistics, viskores::Float64& ssim) const
  {
    if (statistics.Weight <= 0)
    {
      this->RaiseError("SSIM patch contains no samples.");
      ssim = 0;
      return;
    }

    const viskores::Float64 primaryMean = statistics.SumPrimary / statistics.Weight;
    const viskores::Float64 secondaryMean = statistics.SumSecondary / statistics.Weight;
    viskores::Float64 primaryVariance =
      (statistics.SumPrimarySquared / statistics.Weight) - (primaryMean * primaryMean);
    viskores::Float64 secondaryVariance =
      (statistics.SumSecondarySquared / statistics.Weight) - (secondaryMean * secondaryMean);
    const viskores::Float64 covariance =
      (statistics.SumProduct / statistics.Weight) - (primaryMean * secondaryMean);

    if (primaryVariance < 0)
    {
      primaryVariance = 0;
    }
    if (secondaryVariance < 0)
    {
      secondaryVariance = 0;
    }

    const viskores::Float64 c1 = (this->K1 * this->DynamicRange) * (this->K1 * this->DynamicRange);
    const viskores::Float64 c2 = (this->K2 * this->DynamicRange) * (this->K2 * this->DynamicRange);

    const viskores::Float64 luminance = (2 * primaryMean * secondaryMean) + c1;
    const viskores::Float64 contrastStructure = (2 * covariance) + c2;
    const viskores::Float64 denominator =
      ((primaryMean * primaryMean) + (secondaryMean * secondaryMean) + c1) *
      (primaryVariance + secondaryVariance + c2);

    if (denominator == 0)
    {
      this->RaiseError("SSIM denominator is zero.");
      ssim = 0;
      return;
    }

    ssim = (luminance * contrastStructure) / denominator;
  }

  viskores::IdComponent PatchRadius;
  viskores::Float64 DynamicRange;
  viskores::Float64 K1;
  viskores::Float64 K2;
};

struct AccumulateWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(viskores::Float64& accumulation,
                                const viskores::Float64& newvalue) const
  {
    accumulation = accumulation + newvalue;
  }
};

struct WeightWorklet : public viskores::worklet::WorkletMapField
{
  WeightWorklet(viskores::Float64 weight)
    : Weight(weight)
  {
  }

  using ControlSignature = void(FieldInOut);

  VISKORES_EXEC void operator()(viskores::Float64& value) const { value *= this->Weight; }

  viskores::Float64 Weight;
};

void IncludeFieldRange(const viskores::cont::Field& field, viskores::Range& fullRange)
{
  viskores::cont::ArrayHandle<viskores::Range> componentRanges = field.GetRange();
  auto rangePortal = componentRanges.ReadPortal();
  for (viskores::Id componentI = 0; componentI < rangePortal.GetNumberOfValues(); ++componentI)
  {
    fullRange.Include(rangePortal.Get(componentI));
  }
}

viskores::Float64 ComputeFieldRange(const viskores::cont::Field& primaryField,
                                    const viskores::cont::Field& secondaryField)
{
  viskores::Range fullRange;
  IncludeFieldRange(primaryField, fullRange);
  IncludeFieldRange(secondaryField, fullRange);
  viskores::Float64 range = fullRange.Length();
  return (range > 0) ? range : 1.0;
}

void ValidateParameters(viskores::IdComponent patchRadius,
                        viskores::Float64 dynamicRange,
                        viskores::Float64 k1,
                        viskores::Float64 k2)
{
  if (patchRadius < 0)
  {
    throw viskores::cont::ErrorFilterExecution("SSIM patch radius must be nonnegative.");
  }
  if (dynamicRange <= 0)
  {
    throw viskores::cont::ErrorFilterExecution("SSIM dynamic range must be positive.");
  }
  if ((k1 < 0) || (k2 < 0))
  {
    throw viskores::cont::ErrorFilterExecution("SSIM constants K1 and K2 must be nonnegative.");
  }
}

void ValidateFieldPair(const viskores::cont::DataSet& input,
                       const viskores::cont::Field& primary,
                       const viskores::cont::Field& secondary)
{
  if (!primary.IsPointField() || !secondary.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution(
      "SSIM expects the primary and secondary fields to be point fields.");
  }

  if (primary.GetNumberOfValues() != secondary.GetNumberOfValues())
  {
    throw viskores::cont::ErrorFilterExecution(
      "SSIM expects the primary and secondary fields to have the same number of values.");
  }

  if (primary.GetNumberOfValues() < 1)
  {
    throw viskores::cont::ErrorFilterExecution("SSIM expects at least one field value.");
  }

  if (primary.GetNumberOfValues() != input.GetNumberOfPoints())
  {
    throw viskores::cont::ErrorFilterExecution(
      "SSIM expects point fields to have the same number of values as the input points.");
  }

  if (primary.GetData().GetNumberOfComponentsFlat() !=
      secondary.GetData().GetNumberOfComponentsFlat())
  {
    throw viskores::cont::ErrorFilterExecution(
      "SSIM expects the primary and secondary fields to have the same number of components.");
  }

  if (primary.GetData().GetNumberOfComponentsFlat() < 1)
  {
    throw viskores::cont::ErrorFilterExecution("SSIM expects at least one field component.");
  }
}

template <typename PrimaryArrayType, typename SecondaryArrayType>
void InvokeSSIMNeighborhood(const viskores::cont::UnknownCellSet& inputCellSet,
                            const PrimaryArrayType& primary,
                            const SecondaryArrayType& secondary,
                            viskores::IdComponent patchRadius,
                            viskores::Float64 dynamicRange,
                            viskores::Float64 k1,
                            viskores::Float64 k2,
                            viskores::cont::ArrayHandle<viskores::Float64>& output)
{
  viskores::cont::Invoker invoke;

  const viskores::IdComponent numberOfComponents = primary.GetNumberOfComponents();
  for (viskores::IdComponent component = 0; component < numberOfComponents; ++component)
  {
    viskores::cont::ArrayHandle<viskores::Float64> componentSSIM;
    invoke(SSIMComponentNeighborhood(patchRadius, dynamicRange, k1, k2),
           inputCellSet,
           primary.GetComponentArray(component),
           secondary.GetComponentArray(component),
           componentSSIM);

    if (component == 0)
    {
      output = componentSSIM;
    }
    else
    {
      invoke(AccumulateWorklet{}, output, componentSSIM);
    }
  }

  if (numberOfComponents > 1)
  {
    viskores::Float64 weight = 1.0 / static_cast<viskores::Float64>(numberOfComponents);
    invoke(WeightWorklet{ weight }, output);
  }
}

viskores::cont::ArrayHandle<viskores::Float64> ComputeSSIMArray(
  const viskores::cont::DataSet& input,
  const viskores::cont::Field& primaryField,
  const viskores::cont::Field& secondaryField,
  viskores::IdComponent patchRadius,
  viskores::Float64 dynamicRange,
  viskores::Float64 k1,
  viskores::Float64 k2)
{
  ValidateFieldPair(input, primaryField, secondaryField);
  ValidateParameters(patchRadius, dynamicRange, k1, k2);

  auto inputCellSet =
    input.GetCellSet().ResetCellSetList<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>();

  viskores::cont::ArrayHandle<viskores::Float64> output;
  const auto& primaryData = primaryField.GetData();
  const auto& secondaryData = secondaryField.GetData();
  auto resolvePrimary = [&](const auto& primaryArray)
  {
    using PrimaryArrayType = std::decay_t<decltype(primaryArray)>;
    using ComponentType = typename PrimaryArrayType::ValueType::ComponentType;

    if (secondaryData.IsBaseComponentType<ComponentType>())
    {
      InvokeSSIMNeighborhood(inputCellSet,
                             primaryArray,
                             secondaryData.ExtractArrayFromComponents<ComponentType>(),
                             patchRadius,
                             dynamicRange,
                             k1,
                             k2,
                             output);
    }
    else
    {
      const viskores::cont::UnknownArrayHandle primaryFloat = primaryField.GetDataAsDefaultFloat();
      const viskores::cont::UnknownArrayHandle secondaryFloat =
        secondaryField.GetDataAsDefaultFloat();
      InvokeSSIMNeighborhood(inputCellSet,
                             primaryFloat.ExtractArrayFromComponents<viskores::FloatDefault>(),
                             secondaryFloat.ExtractArrayFromComponents<viskores::FloatDefault>(),
                             patchRadius,
                             dynamicRange,
                             k1,
                             k2,
                             output);
    }
  };
  primaryData.CastAndCallWithExtractedArray(resolvePrimary);

  return output;
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace image_processing
{

//-----------------------------------------------------------------------------
SSIM::SSIM()
{
  this->SetPrimaryField("image-1");
  this->SetSecondaryField("image-2");
  this->SetOutputFieldName("ssim");
}

//-----------------------------------------------------------------------------
viskores::Float64 SSIM::GetRealDynamicRange(const viskores::cont::Field& primaryField,
                                            const viskores::cont::Field& secondaryField) const
{
  viskores::Float64 dynamicRange;
  switch (this->RangeType)
  {
    case DynamicRangeType::Default:
    {
      dynamicRange = 0;
      bool foundIntegralRange = false;
      auto checkFieldType = [&](auto type)
      {
        using Type = decltype(type);
        if (!foundIntegralRange && primaryField.GetData().IsBaseComponentType<Type>())
        {
          if constexpr (std::is_integral_v<Type>)
          {
            dynamicRange = static_cast<viskores::Float64>(std::numeric_limits<Type>::max()) -
              static_cast<viskores::Float64>(std::numeric_limits<Type>::min());
            foundIntegralRange = true;
          }
        }
      };
      viskores::ListForEach(checkFieldType, viskores::TypeListBaseC{});

      if (!foundIntegralRange)
      {
        dynamicRange = ComputeFieldRange(primaryField, secondaryField);
      }
    }
    break;
    case DynamicRangeType::Explicit:
      dynamicRange = this->DynamicRange;
      break;
    case DynamicRangeType::Range:
      dynamicRange = ComputeFieldRange(primaryField, secondaryField);
      break;
  }

  return dynamicRange;
}

//-----------------------------------------------------------------------------
viskores::Float64 SSIM::ComputeMetric(const viskores::cont::DataSet& input) const
{
  const auto& primaryField = this->GetFieldFromDataSet(input);
  const auto& secondaryField = this->GetFieldFromDataSet(1, input);

  const auto ssimArray = ComputeSSIMArray(input,
                                          primaryField,
                                          secondaryField,
                                          this->PatchRadius,
                                          this->GetRealDynamicRange(primaryField, secondaryField),
                                          this->K1,
                                          this->K2);
  return viskores::cont::Algorithm::Reduce(ssimArray, 0.0) /
    static_cast<viskores::Float64>(ssimArray.GetNumberOfValues());
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet SSIM::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& primaryField = this->GetFieldFromDataSet(input);
  const auto& secondaryField = this->GetFieldFromDataSet(1, input);

  auto ssimArray = ComputeSSIMArray(input,
                                    primaryField,
                                    secondaryField,
                                    this->PatchRadius,
                                    this->GetRealDynamicRange(primaryField, secondaryField),
                                    this->K1,
                                    this->K2);

  std::string outputFieldName = this->GetOutputFieldName();
  if (outputFieldName.empty())
  {
    outputFieldName = "ssim";
  }
  return this->CreateResultFieldPoint(input, outputFieldName, ssimArray);
}

} // namespace image_processing
} // namespace filter
} // namespace viskores
