//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/uncertainty/ContourUncertainUniformMonteCarlo.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{
namespace uniform
{
class ContourUncertainUniformMonteCarlo : public vtkm::worklet::WorkletVisitCellsWithPoints
{

public:
  ContourUncertainUniformMonteCarlo(double isovalue, int itervalue)
    : m_isovalue(isovalue)
    , m_numsample(itervalue)
  {
  }
  using ControlSignature = void(CellSetIn,
                                FieldInPoint,
                                FieldInPoint,
                                FieldInCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell);
  using ExecutionSignature = void(_2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  template <typename InPointFieldMinType,
            typename InPointFieldMaxType,
            typename InRandomNumbersType,
            typename OutCellFieldType1,
            typename OutCellFieldType2,
            typename OutCellFieldType3>

  VTKM_EXEC void operator()(const InPointFieldMinType& inPointFieldVecMin,
                            const InPointFieldMaxType& inPointFieldVecMax,
                            const InRandomNumbersType& randomNumbers,
                            OutCellFieldType1& outNonCrossProb,
                            OutCellFieldType2& outCrossProb,
                            OutCellFieldType3& outEntropyProb) const
  {
    vtkm::IdComponent numPoints = inPointFieldVecMin.GetNumberOfComponents();

    if (numPoints != 8)
    {
      this->RaiseError("This is the 3D version for 8 vertices\n");
      return;
    }

    vtkm::FloatDefault minV = 0.0;
    vtkm::FloatDefault maxV = 0.0;
    vtkm::FloatDefault uniformDistValue = 0.0;
    vtkm::IdComponent numSample = this->m_numsample;
    vtkm::FloatDefault numCrossing = 0;
    vtkm::FloatDefault crossProb = 0;

    vtkm::IdComponent zeroFlag;
    vtkm::IdComponent oneFlag;
    vtkm::Float64 base = 2.0;
    vtkm::Float64 totalSum = 0.0;
    vtkm::IdComponent nonZeroCase = 0;
    vtkm::FloatDefault entropyValue = 0;
    vtkm::FloatDefault templog = 0;
    vtkm::FloatDefault value = 0.0;
    vtkm::IdComponent k = 0;

    constexpr vtkm::IdComponent totalNumCases = 256;
    vtkm::Vec<vtkm::FloatDefault, totalNumCases> probHistogram;

    for (vtkm::IdComponent j = 0; j < totalNumCases; j++)
    {
      probHistogram[j] = 0;
    }

    for (vtkm::IdComponent i = 0; i < numSample; ++i)
    {
      zeroFlag = 0;
      oneFlag = 0;
      totalSum = 0.0;
      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
      {
        minV = static_cast<vtkm::FloatDefault>(inPointFieldVecMin[pointIndex]);
        maxV = static_cast<vtkm::FloatDefault>(inPointFieldVecMax[pointIndex]);

        auto tmp = randomNumbers[k];

        uniformDistValue = minV + tmp * (maxV - minV);

        if (uniformDistValue <= this->m_isovalue)
        {
          zeroFlag = 1;
        }
        else
        {
          oneFlag = 1;
          totalSum += vtkm::Pow(base, pointIndex);
        }

        k += 1;
      }

      if ((oneFlag == 1) && (zeroFlag == 1))
      {
        numCrossing += 1;
      }

      if ((totalSum >= 0) && (totalSum <= 255))
      {
        probHistogram[static_cast<vtkm::IdComponent>(totalSum)] += 1;
      }
    }

    for (vtkm::IdComponent i = 0; i < totalNumCases; i++)
    {
      templog = 0;
      value = static_cast<vtkm::FloatDefault>(probHistogram[i] /
                                              static_cast<vtkm::FloatDefault>(numSample));
      if (probHistogram[i] > 0.00001)
      {
        nonZeroCase++;
        templog = vtkm::Log2(value);
      }
      entropyValue = entropyValue + (-value) * templog;
    }

    crossProb = numCrossing / static_cast<vtkm::FloatDefault>(numSample);
    outNonCrossProb = static_cast<vtkm::FloatDefault>(nonZeroCase);
    outCrossProb = crossProb;
    outEntropyProb = entropyValue;
  }

private:
  double m_isovalue;
  int m_numsample;
};
}
}
}

namespace vtkm
{
namespace filter
{
namespace uncertainty
{
ContourUncertainUniformMonteCarlo::ContourUncertainUniformMonteCarlo()
{
  this->SetCrossProbabilityName("cross_probability");
}
VTKM_CONT vtkm::cont::DataSet ContourUncertainUniformMonteCarlo::DoExecute(
  const vtkm::cont::DataSet& input)
{
  vtkm::cont::Field minField = this->GetFieldFromDataSet(0, input);
  vtkm::cont::Field maxField = this->GetFieldFromDataSet(1, input);

  vtkm::cont::UnknownArrayHandle crossProbability;
  vtkm::cont::UnknownArrayHandle nonCrossProbability;
  vtkm::cont::UnknownArrayHandle entropyProbability;

  if (!input.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>())
  {
    throw vtkm::cont::ErrorBadType("Uncertain contour only works for CellSetStructured<3>.");
  }
  vtkm::cont::CellSetStructured<3> cellSet;
  input.GetCellSet().AsCellSet(cellSet);

  auto resolveType = [&](auto concreteMinField) {
    using ArrayType = std::decay_t<decltype(concreteMinField)>;
    using ValueType = typename ArrayType::ValueType;
    ArrayType concreteMaxField;
    vtkm::cont::ArrayCopyShallowIfPossible(maxField.GetData(), concreteMaxField);

    vtkm::cont::ArrayHandle<ValueType> concreteCrossProb;
    vtkm::cont::ArrayHandle<ValueType> concreteNonCrossProb;
    vtkm::cont::ArrayHandle<ValueType> concreteEntropyProb;

    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::FloatDefault> randomArray(
      cellSet.GetNumberOfCells() * this->IterValue * 8, { 0xceed });

    this->Invoke(
      vtkm::worklet::uniform::ContourUncertainUniformMonteCarlo{ this->IsoValue, this->IterValue },
      cellSet,
      concreteMinField,
      concreteMaxField,
      vtkm::cont::make_ArrayHandleGroupVecVariable(
        randomArray,
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(
          0, this->IterValue * 8, cellSet.GetNumberOfCells() + 1)),
      concreteNonCrossProb,
      concreteCrossProb,
      concreteEntropyProb);

    crossProbability = concreteCrossProb;
    nonCrossProbability = concreteNonCrossProb;
    entropyProbability = concreteEntropyProb;
  };
  this->CastAndCallScalarField(minField, resolveType);
  vtkm::cont::DataSet result = this->CreateResult(input);
  result.AddCellField(this->GetCrossProbabilityName(), crossProbability);
  result.AddCellField(this->GetNumberNonzeroProbabilityName(), nonCrossProbability);
  result.AddCellField(this->GetEntropyName(), entropyProbability);
  return result;
}
}
}
}
