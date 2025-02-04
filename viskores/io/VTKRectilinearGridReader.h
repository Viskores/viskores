//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_io_VTKRectilinearGridReader_h
#define viskores_io_VTKRectilinearGridReader_h

#include <viskores/io/VTKDataSetReaderBase.h>

namespace viskores
{
namespace io
{

class VISKORES_IO_EXPORT VTKRectilinearGridReader : public VTKDataSetReaderBase
{
public:
  explicit VISKORES_CONT VTKRectilinearGridReader(const char* fileName);
  explicit VISKORES_CONT VTKRectilinearGridReader(const std::string& fileName);

private:
  VISKORES_CONT void Read() override;
};
}
} // namespace viskores::io

#endif // viskores_io_VTKRectilinearGridReader_h
