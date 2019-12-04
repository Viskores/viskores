//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_decode2_h
#define vtk_m_worklet_zfp_decode2_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPDecode.h>
#include <vtkm/worklet/zfp/ZFPFunctions.h>
#include <vtkm/worklet/zfp/ZFPStructs.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void ScatterPartial2(const Scalar* q,
                                      PortalType& scalars,
                                      const vtkm::Id2 dims,
                                      vtkm::Id offset,
                                      vtkm::Int32 nx,
                                      vtkm::Int32 ny)
{
  vtkm::Id x, y;
  for (y = 0; y < ny; y++, offset += dims[0] - nx, q += 4 - nx)
  {
    for (x = 0; x < nx; x++, offset++, q++)
    {
      scalars.Set(offset, *q);
    }
  }
}

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Scatter2(const Scalar* q,
                               PortalType& scalars,
                               const vtkm::Id2 dims,
                               vtkm::Id offset)
{
  for (vtkm::Id y = 0; y < 4; y++, offset += dims[0] - 4)
  {
    for (vtkm::Id x = 0; x < 4; x++, ++offset)
    {
      scalars.Set(offset, *q++);
    } // x
  }   // y
}

struct Decode2 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id2 Dims;       // field dims
  vtkm::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  vtkm::Id2 ZFPDims;    // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block
public:
  Decode2(const vtkm::Id2 dims, const vtkm::Id2 paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr vtkm::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];
    // clear
    for (vtkm::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, vtkm::Int32(MaxBits), static_cast<vtkm::UInt32>(blockIdx), stream);

    vtkm::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    vtkm::Id2 logicalStart = zfpBlock * vtkm::Id(4);

    vtkm::Id offset = logicalStart[0] + logicalStart[1] * Dims[0];
    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (partial)
    {
      const vtkm::Int32 nx =
        logicalStart[0] + 4 > Dims[0] ? vtkm::Int32(Dims[0] - logicalStart[0]) : vtkm::Int32(4);
      const vtkm::Int32 ny =
        logicalStart[1] + 4 > Dims[1] ? vtkm::Int32(Dims[1] - logicalStart[1]) : vtkm::Int32(4);
      ScatterPartial2(fblock, scalars, Dims, offset, nx, ny);
    }
    else
    {
      Scatter2(fblock, scalars, Dims, offset);
    }
  }
};
}
}
} // namespace vtkm::worklet::zfp
#endif
