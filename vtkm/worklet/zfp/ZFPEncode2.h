//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_encode2_h
#define vtk_m_worklet_zfp_encode2_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPEncode.h>
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
VTKM_EXEC inline void GatherPartial2(Scalar* q,
                                     const PortalType& scalars,
                                     vtkm::Id offset,
                                     vtkm::Int32 nx,
                                     vtkm::Int32 ny,
                                     vtkm::Int32 sx,
                                     vtkm::Int32 sy)
{
  vtkm::Id x, y;
  for (y = 0; y < ny; y++, offset += sy - nx * sx)
  {
    for (x = 0; x < nx; x++, offset += 1)
      q[4 * y + x] = scalars.Get(offset);
    PadBlock(q + 4 * y, vtkm::UInt32(nx), 1);
  }
  for (x = 0; x < 4; x++)
    PadBlock(q + x, vtkm::UInt32(ny), 4);
}

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Gather2(Scalar* fblock,
                              const PortalType& scalars,
                              vtkm::Id offset,
                              int sx,
                              int sy)
{
  vtkm::Id counter = 0;

  for (vtkm::Id y = 0; y < 4; y++, offset += sy - 4 * sx)
    for (vtkm::Id x = 0; x < 4; x++, offset += sx)
    {
      fblock[counter] = scalars.Get(offset);
      counter++;
    }
}

struct Encode2 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id2 Dims;       // field dims
  vtkm::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  vtkm::Id2 ZFPDims;    // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block

public:
  Encode2(const vtkm::Id2 dims, const vtkm::Id2 paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <class InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    vtkm::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    vtkm::Id2 logicalStart = zfpBlock * vtkm::Id(4);
    vtkm::Id offset = logicalStart[1] * Dims[0] + logicalStart[0];

    constexpr vtkm::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];

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
      GatherPartial2(fblock, scalars, offset, nx, ny, 1, static_cast<vtkm::Int32>(Dims[0]));
    }
    else
    {
      Gather2(fblock, scalars, offset, 1, static_cast<vtkm::Int32>(Dims[0]));
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(fblock, vtkm::Int32(MaxBits), vtkm::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
