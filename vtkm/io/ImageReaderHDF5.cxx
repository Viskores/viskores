//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/io/ErrorIO.h>
#include <vtkm/io/ImageReaderHDF5.h>
#include <vtkm/io/PixelTypes.h>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace vtkm
{
namespace io
{

ImageReaderHDF5::~ImageReaderHDF5() noexcept = default;

void ImageReaderHDF5::Read()
{
  // need to find width, height and pixel type.
  auto fileid = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

  const auto fieldName = this->PointFieldName.c_str();
  if (!H5IMis_image(fileid, fieldName))
  {
    throw vtkm::io::ErrorIO{ "Not an HDF5 image file" };
  }

  hsize_t width, height, nplanes;
  hssize_t npals;
  char interlace[16];
  if (H5IMget_image_info(fileid, fieldName, &width, &height, &nplanes, interlace, &npals) < 0)
  {
    throw vtkm::io ::ErrorIO{ "Can not get image info" };
  }

  // We don't use the H5IMread_image() since it only supports 8 bit pixel.
  hid_t did;
  if ((did = H5Dopen2(fileid, fieldName, H5P_DEFAULT)) < 0)
  {
    throw vtkm::io::ErrorIO{ "Can not open image dataset" };
  }

  if (strncmp(interlace, "INTERLACE_PIXEL", 15) != 0)
  {
    std::string message = "Unsupported interlace mode: ";
    message += interlace;
    message +=
      ". Currently, only the INTERLACE_PIXEL mode is supported. See "
      "https://portal.hdfgroup.org/display/HDF5/HDF5+Image+and+Palette+Specification%2C+Version+1.2"
      " for more details on the HDF5 image convention.";
    throw vtkm::io::ErrorIO{ message };
  }

  std::vector<unsigned char> buffer;
  auto type_size = H5LDget_dset_type_size(did, nullptr);
  buffer.resize(width * height * 3 * type_size);
  switch (type_size)
  {
    case 1:
      H5Dread(did, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
      break;
    case 2:
      H5Dread(did, H5T_NATIVE_UINT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
      break;
    default:
      throw vtkm::io::ErrorIO{ "Unsupported pixel type" };
  }

  H5Dclose(did);
  H5Fclose(fileid);

  // convert PixelType to Vec4f_32
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> pixelArray;
  pixelArray.Allocate(width * height);
  auto portal = pixelArray.WritePortal();
  vtkm::Id vtkmIndex = 0;
  for (vtkm::Id yIndex = 0; yIndex < static_cast<vtkm::Id>(height); yIndex++)
  {
    for (vtkm::Id xIndex = 0; xIndex < static_cast<vtkm::Id>(width); xIndex++)
    {
      vtkm::Id hdfIndex = static_cast<vtkm::Id>(yIndex * width + xIndex);
      if (type_size == 1)
      {
        portal.Set(vtkmIndex, vtkm::io::RGBPixel_8(buffer.data(), hdfIndex).ToVec4f());
      }
      else
      {
        portal.Set(vtkmIndex, vtkm::io::RGBPixel_16(buffer.data(), hdfIndex).ToVec4f());
      }
      vtkmIndex++;
    }
  }

  this->InitializeImageDataSet(width, height, pixelArray);
} // Read()

}
}
