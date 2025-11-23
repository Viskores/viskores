//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Timer.h>
#include "./2VariateUncertianty.h"
#include "./worklet/2VariateUncertianty.h"

namespace viskores
{
  namespace filter
  {
    namespace uncertainty
    {
      VISKORES_CONT viskores::cont::DataSet FiberMean::DoExecute(const viskores::cont::DataSet &input)
      {
        std::string FieldName;

        // viskores::cont::Timer timer;
        // std::cout << "detailed timer device: " << timer.GetDevice().GetName() << std::endl;
        // timer.Start();
        //  Input Field
        viskores::cont::Field EnsembleMinX = this->GetFieldFromDataSet(0, input);
        viskores::cont::Field EnsembleMaxX = this->GetFieldFromDataSet(1, input);
        viskores::cont::Field EnsembleMinY = this->GetFieldFromDataSet(2, input);
        viskores::cont::Field EnsembleMaxY = this->GetFieldFromDataSet(3, input);

        // Output Field
        viskores::cont::UnknownArrayHandle OutputProbability;

        // timer.Stop();
        // std::cout << "filter 1 " << timer.GetElapsedTime() << std::endl;
        //  For Invoker
        auto resolveType = [&](auto ConcreteEnsembleMinX)
        {
          // timer.Start();
          //  Obtaining Type
          using ArrayType = std::decay_t<decltype(ConcreteEnsembleMinX)>;
          using ValueType = typename ArrayType::ValueType;

          // Temporary Input Variable to add input values
          ArrayType ConcreteEnsembleMaxX;
          ArrayType ConcreteEnsembleMinY;
          ArrayType ConcreteEnsembleMaxY;

          viskores::cont::ArrayCopyShallowIfPossible(EnsembleMaxX.GetData(), ConcreteEnsembleMaxX);
          viskores::cont::ArrayCopyShallowIfPossible(EnsembleMinY.GetData(), ConcreteEnsembleMinY);
          viskores::cont::ArrayCopyShallowIfPossible(EnsembleMaxY.GetData(), ConcreteEnsembleMaxY);

          // Temporary Output Variable
          viskores::cont::ArrayHandle<ValueType> Probability;
          // timer.Stop();
          // std::cout << "filter 2 " << timer.GetElapsedTime() << std::endl;
          // timer.Start();

          // Invoker

          if (this->Approach == "MonteCarlo")
          {
            FieldName = "MonteCarlo";
            std::cout << "Adopt monte carlo with numsamples " << this->NumSamples << std::endl;
            this->Invoke(viskores::worklet::detail::MultiVariateMonteCarlo{this->minAxis, this->maxAxis, this->NumSamples},
                         ConcreteEnsembleMinX,
                         ConcreteEnsembleMaxX,
                         ConcreteEnsembleMinY,
                         ConcreteEnsembleMaxY,
                         Probability);
          }
          else if (this->Approach == "ClosedForm")
          {
            FieldName = "ClosedForm";
            std::cout << "Adopt ClosedForm" << std::endl;
            this->Invoke(viskores::worklet::detail::MultiVariateClosedForm{this->minAxis, this->maxAxis},
                         ConcreteEnsembleMinX,
                         ConcreteEnsembleMaxX,
                         ConcreteEnsembleMinY,
                         ConcreteEnsembleMaxY,
                         Probability);
          }
          else if (this->Approach == "Mean")
          {
            FieldName = "Mean";
            std::cout << "Adopt Mean" << std::endl;
            this->Invoke(viskores::worklet::detail::MultiVariateMean{this->minAxis, this->maxAxis},
                         ConcreteEnsembleMinX,
                         ConcreteEnsembleMaxX,
                         ConcreteEnsembleMinY,
                         ConcreteEnsembleMaxY,
                         Probability);
          }
          else if (this->Approach == "Truth")
          {
            FieldName = "Truth";
            std::cout << "Adopt Truth" << std::endl;
            this->Invoke(viskores::worklet::detail::MultiVariateTruth{this->minAxis, this->maxAxis},
                         ConcreteEnsembleMinX,
                         ConcreteEnsembleMaxX,
                         ConcreteEnsembleMinY,
                         ConcreteEnsembleMaxY,
                         Probability);
          }
          else
          {
            throw std::runtime_error("unsupported approach:" + this->Approach);
          }

          // From Temporary Output Variable to Output Variable
          OutputProbability = Probability;
          // timer.Stop();
          // std::cout << "filter 3 " << timer.GetElapsedTime() << std::endl;
        };
        this->CastAndCallScalarField(EnsembleMinX, resolveType);

        // Creating Result
        // timer.Start();
        viskores::cont::DataSet result = this->CreateResult(input);
        result.AddPointField(FieldName, OutputProbability);
        // timer.Stop();
        // std::cout << "filter 4 " << timer.GetElapsedTime() << std::endl;

        return result;
      }
    }
  }
}
