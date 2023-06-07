//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

//  This code is based on the algorithm presented in the following papers:
//  Wang, J., Athawale, T., Moreland, K., Chen, J., Johnson, C., & Pugmire,
//  D. (2023). FunMC^ 2: A Filter for Uncertainty Visualization of Marching
//  Cubes on Multi-Core Devices. Oak Ridge National Laboratory (ORNL),
//  Oak Ridge, TN (United States).
//
//  Athawale, T. M., Sane, S., & Johnson, C. R. (2021, October). Uncertainty
//  Visualization of the Marching Squares and Marching Cubes Topology Cases.
//  In 2021 IEEE Visualization Conference (VIS) (pp. 106-110). IEEE.

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/uncertainty/ContourUncertainUniform.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace
{
class ClosedFormUniform : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  ClosedFormUniform(double isovalue)
    : m_isovalue(isovalue){};
  using ControlSignature =
    void(CellSetIn, FieldInPoint, FieldInPoint, FieldOutCell, FieldOutCell, FieldOutCell);

  using ExecutionSignature = void(_2, _3, _4, _5, _6);

  using InputDomain = _1;
  template <typename InPointFieldMinType,
            typename InPointFieldMaxType,
            typename OutCellFieldType1,
            typename OutCellFieldType2,
            typename OutCellFieldType3>

  VTKM_EXEC void operator()(const InPointFieldMinType& inPointFieldVecMin,
                            const InPointFieldMaxType& inPointFieldVecMax,
                            OutCellFieldType1& outCellFieldCProb,
                            OutCellFieldType2& outCellFieldNumNonzeroProb,
                            OutCellFieldType3& outCellFieldEntropy) const
  {
    vtkm::IdComponent numPoints = inPointFieldVecMin.GetNumberOfComponents();

    if (numPoints != 8)
    {
      this->RaiseError("This is the 3D version for 8 vertices\n");
      return;
    }

    vtkm::FloatDefault allPositiveProb = 1.0;
    vtkm::FloatDefault allNegativeProb = 1.0;
    vtkm::FloatDefault allCrossProb = 0.0;
    vtkm::FloatDefault positiveProb;
    vtkm::FloatDefault negativeProb;
    vtkm::Vec<vtkm::Vec2f, 8> ProbList;

    constexpr vtkm::IdComponent totalNumCases = 256;
    vtkm::Vec<vtkm::FloatDefault, totalNumCases> probHistogram;

    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      vtkm::FloatDefault minV = static_cast<vtkm::FloatDefault>(inPointFieldVecMin[pointIndex]);
      vtkm::FloatDefault maxV = static_cast<vtkm::FloatDefault>(inPointFieldVecMax[pointIndex]);

      if (this->m_isovalue <= minV)
      {
        positiveProb = 1.0;
        negativeProb = 0.0;
      }
      else if (this->m_isovalue >= maxV)
      {
        positiveProb = 0.0;
        negativeProb = 1.0;
      }
      else
      {
        positiveProb = static_cast<vtkm::FloatDefault>((maxV - (this->m_isovalue)) / (maxV - minV));
        negativeProb = 1 - positiveProb;
      }

      allNegativeProb *= negativeProb;
      allPositiveProb *= positiveProb;

      ProbList[pointIndex][0] = negativeProb;
      ProbList[pointIndex][1] = positiveProb;
    }

    allCrossProb = 1 - allPositiveProb - allNegativeProb;
    outCellFieldCProb = allCrossProb;

    TraverseBit(ProbList, probHistogram);

    vtkm::FloatDefault entropyValue = 0;
    vtkm::Id nonzeroCases = 0;
    vtkm::FloatDefault templog = 0;

    for (vtkm::IdComponent i = 0; i < totalNumCases; i++)
    {
      templog = 0;
      if (probHistogram[i] > 0.00001)
      {
        nonzeroCases++;
        templog = vtkm::Log2(probHistogram[i]);
      }
      entropyValue = entropyValue + (-probHistogram[i]) * templog;
    }

    outCellFieldNumNonzeroProb = nonzeroCases;
    outCellFieldEntropy = entropyValue;
  }

  VTKM_EXEC inline void TraverseBit(vtkm::Vec<vtkm::Vec2f, 8>& ProbList,
                                    vtkm::Vec<vtkm::FloatDefault, 256>& probHistogram) const
  {

    for (vtkm::IdComponent i = 0; i < 256; i++)
    {
      vtkm::FloatDefault currProb = 1.0;
      for (vtkm::IdComponent j = 0; j < 8; j++)
      {
        if (i & (1 << j))
        {
          currProb *= ProbList[j][1];
        }
        else
        {
          currProb *= ProbList[j][0];
        }
      }
      probHistogram[i] = currProb;
    }
  }

private:
  double m_isovalue;
};
}

namespace vtkm
{
namespace filter
{
namespace uncertainty
{
ContourUncertainUniform::ContourUncertainUniform()
{
  this->SetCrossProbabilityName("cross_probability");
}
VTKM_CONT vtkm::cont::DataSet ContourUncertainUniform::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::cont::Field minField = this->GetFieldFromDataSet(0, input);
  vtkm::cont::Field maxField = this->GetFieldFromDataSet(1, input);

  vtkm::cont::UnknownArrayHandle crossProbability;
  vtkm::cont::UnknownArrayHandle numNonZeroProbability;
  vtkm::cont::UnknownArrayHandle entropy;

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
    vtkm::cont::ArrayHandle<vtkm::Id> concreteNumNonZeroProb;
    vtkm::cont::ArrayHandle<ValueType> concreteEntropy;
    this->Invoke(ClosedFormUniform{ this->IsoValue },
                 cellSet,
                 concreteMinField,
                 concreteMaxField,
                 concreteCrossProb,
                 concreteNumNonZeroProb,
                 concreteEntropy);
    crossProbability = concreteCrossProb;
    numNonZeroProbability = concreteNumNonZeroProb;
    entropy = concreteEntropy;
  };
  this->CastAndCallScalarField(minField, resolveType);

  vtkm::cont::DataSet result = this->CreateResult(input);
  result.AddCellField(this->GetCrossProbabilityName(), crossProbability);
  result.AddCellField(this->GetNumberNonzeroProbabilityName(), numNonZeroProbability);
  result.AddCellField(this->GetEntropyName(), entropy);
  return result;
}
}
}
}
