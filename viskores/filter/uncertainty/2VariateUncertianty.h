//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_uncertainty_Fiber_separate_h
#define viskores_uncertainty_Fiber_separate_h

#include <viskores/filter/Filter.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{
class FiberMean : public viskores::filter::Filter
{
  viskores::Pair<viskores::Float64, viskores::Float64> minAxis;
  viskores::Pair<viskores::Float64, viskores::Float64> maxAxis;
  std::string Approach = "ClosedForm"; //MonteCarlo or ClosedForm
  viskores::Id NumSamples = 500;

public:
  VISKORES_CONT void SetMinAxis(const viskores::Pair<viskores::Float64, viskores::Float64>& minCoordinate)
  {
    this->minAxis = minCoordinate;
  }

  VISKORES_CONT void SetMaxAxis(const viskores::Pair<viskores::Float64, viskores::Float64>& maxCoordinate)
  {
    this->maxAxis = maxCoordinate;
  }
  VISKORES_CONT void SetMinX(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetMaxX(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetMinY(const std::string& fieldName)
  {
    this->SetActiveField(2, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetMaxY(const std::string& fieldName)
  {
    this->SetActiveField(3, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetMinZ(const std::string& fieldName)
  {
    this->SetActiveField(4, fieldName, viskores::cont::Field::Association::Points);
  }
  VISKORES_CONT void SetMaxZ(const std::string& fieldName)
  {
    this->SetActiveField(5, fieldName, viskores::cont::Field::Association::Points);
  }

  VISKORES_CONT void SetNumSamples(const viskores::Id& numSamples)
  {
    this->NumSamples=numSamples;
  }

   VISKORES_CONT void SetApproach(const std::string& approach)
  {
    this->Approach=approach;
  } 


private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
}
}
}
#endif
