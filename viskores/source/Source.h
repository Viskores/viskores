//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_source_Source_h
#define viskores_source_Source_h

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>
#include <viskores/source/viskores_source_export.h>

namespace viskores
{
namespace source
{

class VISKORES_SOURCE_EXPORT Source
{
public:
  VISKORES_CONT
  virtual ~Source() = default;

  viskores::cont::DataSet Execute() const { return this->DoExecute(); }

protected:
  virtual viskores::cont::DataSet DoExecute() const = 0;

  viskores::cont::Invoker Invoke;
};

} // namespace source
} // namespace viskores

#endif // viskores_source_Source_h
