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
#ifndef viskores_cont_SplineEvaluateStructuredGrid_h
#define viskores_cont_SplineEvaluateStructuredGrid_h

#include <viskores/exec/SplineEvaluateStructuredGrid.h>

namespace viskores
{
namespace cont
{
class VISKORES_CONT_EXPORT SplineEvaluateUniformGrid : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT SplineEvaluateUniformGrid(const viskores::cont::DataSet& dataSet,
                                          const std::string& fieldName)
    : DataSet(dataSet)
    , FieldName(fieldName)
  {
  }

  viskores::exec::SplineEvaluateUniformGrid PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const;

private:
  viskores::cont::DataSet DataSet;
  std::string FieldName;
};

class VISKORES_CONT_EXPORT SplineEvaluateRectilinearGrid
  : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT SplineEvaluateRectilinearGrid(const viskores::cont::DataSet& dataSet,
                                              const std::string& fieldName)
    : DataSet(dataSet)
    , FieldName(fieldName)
  {
  }

  viskores::exec::SplineEvaluateRectilinearGrid PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const;

private:
  viskores::cont::DataSet DataSet;
  std::string FieldName;
};

}
} // viskores::cont

#endif //viskores_cont_SplineEvaluateStructuredGrid_h
