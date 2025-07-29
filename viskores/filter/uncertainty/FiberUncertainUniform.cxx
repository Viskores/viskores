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

//  This code is based on the algorithm presented in the following papers:
//  Hari, G., Joshi, N., Wang, Z., Gong, Q., Pugmire, D., Moreland, K.,
//  Johnson, C. R., Klasky, S., Podhorszki, N., & Athawale, T.
//  (2024). FunM^2C: A Filter for Uncertainty Visualization of MultiVariate Data
//  on Multi-Core Devices. Oak Ridge National Laboratory (ORNL),
//  Oak Ridge, TN (United States).

#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/uncertainty/FiberUncertainUniform.h>
#include <viskores/filter/uncertainty/worklet/FiberUncertainUniform.h>


namespace viskores
{
namespace filter
{
namespace uncertainty
{
VISKORES_CONT viskores::cont::DataSet FiberUncertainUniform::DoExecute(
  const viskores::cont::DataSet& input)
{
  std::string fieldName;


  viskores::cont::Field ensembleMinX = this->GetFieldFromDataSet(0, input);
  viskores::cont::Field ensembleMaxX = this->GetFieldFromDataSet(1, input);
  viskores::cont::Field ensembleMinY = this->GetFieldFromDataSet(2, input);
  viskores::cont::Field ensembleMaxY = this->GetFieldFromDataSet(3, input);

  // Output Field
  viskores::cont::UnknownArrayHandle outputProbability;


  //  For Invoker
  auto resolveType = [&](auto concreteEnsembleMinX)
  {
    //  Obtaining Type
    using ArrayType = std::decay_t<decltype(concreteEnsembleMinX)>;
    using ValueType = typename ArrayType::ValueType;

    ArrayType concreteEnsembleMaxX;
    ArrayType concreteEnsembleMinY;
    ArrayType concreteEnsembleMaxY;

    viskores::cont::ArrayCopyShallowIfPossible(ensembleMaxX.GetData(), concreteEnsembleMaxX);
    viskores::cont::ArrayCopyShallowIfPossible(ensembleMinY.GetData(), concreteEnsembleMinY);
    viskores::cont::ArrayCopyShallowIfPossible(ensembleMaxY.GetData(), concreteEnsembleMaxY);

    viskores::cont::ArrayHandle<ValueType> probability;

    if (this->Approach == ApproachEnum::MonteCarlo)
    {
      fieldName = "MonteCarlo";
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Adopt Monte Carlo with numsamples " << this->NumSamples);

      viskores::cont::ArrayHandleRandomUniformReal<ValueType> randomHandle(this->NumSamples * 2);

      viskores::worklet::detail::MultiVariateMonteCarlo worklet{ this->minAxis, 
                                                                 this->maxAxis, 
                                                                 this->NumSamples};

      this->Invoke(worklet,
                        concreteEnsembleMinX,
                        concreteEnsembleMaxX,
                        concreteEnsembleMinY,
                        concreteEnsembleMaxY,
                        probability,
                        randomHandle);
    }
    else if (this->Approach == ApproachEnum::ClosedForm)
    {
      fieldName = "ClosedForm";
      std::cout << "Adopt ClosedForm" << std::endl;
      this->Invoke(
        viskores::worklet::detail::MultiVariateClosedForm{ this->minAxis, this->maxAxis },
        concreteEnsembleMinX,
        concreteEnsembleMaxX,
        concreteEnsembleMinY,
        concreteEnsembleMaxY,
        probability);
    }
    else if (this->Approach == ApproachEnum::Mean)
    {
      fieldName = "Mean";
      std::cout << "Adopt Mean" << std::endl;
      this->Invoke(viskores::worklet::detail::MultiVariateMean{ this->minAxis, this->maxAxis },
                   concreteEnsembleMinX,
                   concreteEnsembleMaxX,
                   concreteEnsembleMinY,
                   concreteEnsembleMaxY,
                   probability);
    }
    else if (this->Approach == ApproachEnum::Truth)
    {
      fieldName = "Truth";
      std::cout << "Adopt Truth" << std::endl;
      this->Invoke(viskores::worklet::detail::MultiVariateTruth{ this->minAxis, this->maxAxis },
                   concreteEnsembleMinX,
                   concreteEnsembleMaxX,
                   concreteEnsembleMinY,
                   concreteEnsembleMaxY,
                   probability);
    }
    else
    {
      throw viskores::cont::ErrorBadValue("Unsupported uncertain fiber surface approach.");
    }

    outputProbability = probability;
  };
  this->CastAndCallScalarField(ensembleMinX, resolveType);

  return this->CreateResultFieldPoint(input, fieldName, outputProbability);
}
}
}
}
