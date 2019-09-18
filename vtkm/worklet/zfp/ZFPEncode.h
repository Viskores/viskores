//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_encode_h
#define vtk_m_worklet_zfp_encode_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPCodec.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar>
VTKM_EXEC void PadBlock(Scalar* p, vtkm::UInt32 n, vtkm::UInt32 s)
{
  switch (n)
  {
    case 0:
      p[0 * s] = 0;
    /* FALLTHROUGH */
    case 1:
      p[1 * s] = p[0 * s];
    /* FALLTHROUGH */
    case 2:
      p[2 * s] = p[1 * s];
    /* FALLTHROUGH */
    case 3:
      p[3 * s] = p[0 * s];
    /* FALLTHROUGH */
    default:
      break;
  }
}

template <vtkm::Int32 N, typename FloatType>
inline VTKM_EXEC vtkm::Int32 MaxExponent(const FloatType* vals)
{
  FloatType maxVal = 0;
  for (vtkm::Int32 i = 0; i < N; ++i)
  {
    maxVal = vtkm::Max(maxVal, vtkm::Abs(vals[i]));
  }

  if (maxVal > 0)
  {
    vtkm::Int32 exponent;
    vtkm::Frexp(maxVal, &exponent);
    /* clamp exponent in case x is denormal */
    return vtkm::Max(exponent, 1 - get_ebias<FloatType>());
  }
  return -get_ebias<FloatType>();
}

// maximum number of bit planes to encode
inline VTKM_EXEC vtkm::Int32 precision(vtkm::Int32 maxexp, vtkm::Int32 maxprec, vtkm::Int32 minexp)
{
  return vtkm::Min(maxprec, vtkm::Max(0, maxexp - minexp + 8));
}

template <typename Scalar>
inline VTKM_EXEC Scalar quantize(Scalar x, vtkm::Int32 e)
{
  return vtkm::Ldexp(x, (CHAR_BIT * (vtkm::Int32)sizeof(Scalar) - 2) - e);
}

template <typename Int, typename Scalar, vtkm::Int32 BlockSize>
inline VTKM_EXEC void fwd_cast(Int* iblock, const Scalar* fblock, vtkm::Int32 emax)
{
  Scalar s = quantize<Scalar>(1, emax);
  for (vtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    iblock[i] = static_cast<Int>(s * fblock[i]);
  }
}

template <typename Int, vtkm::Int32 S>
inline VTKM_EXEC void fwd_lift(Int* p)
{
  Int x, y, z, w;
  x = *p;
  p += S;
  y = *p;
  p += S;
  z = *p;
  p += S;
  w = *p;
  p += S;

  /*
  ** non-orthogonal transform
  **        ( 4  4  4  4) (x)
  ** 1/16 * ( 5  1 -1 -5) (y)
  **        (-4  4  4 -4) (z)
  **        (-2  6 -6  2) (w)
  */
  x += w;
  x >>= 1;
  w -= x;
  z += y;
  z >>= 1;
  y -= z;
  x += z;
  x >>= 1;
  z -= x;
  w += y;
  w >>= 1;
  y -= w;
  w += y >> 1;
  y -= w >> 1;

  p -= S;
  *p = w;
  p -= S;
  *p = z;
  p -= S;
  *p = y;
  p -= S;
  *p = x;
}

template <typename Int, typename UInt>
inline VTKM_EXEC UInt int2uint(const Int x);

template <>
inline VTKM_EXEC vtkm::UInt64 int2uint<vtkm::Int64, vtkm::UInt64>(const vtkm::Int64 x)
{
  return (static_cast<vtkm::UInt64>(x) + (vtkm::UInt64)0xaaaaaaaaaaaaaaaaull) ^
    (vtkm::UInt64)0xaaaaaaaaaaaaaaaaull;
}

template <>
inline VTKM_EXEC vtkm::UInt32 int2uint<vtkm::Int32, vtkm::UInt32>(const vtkm::Int32 x)
{
  return (static_cast<vtkm::UInt32>(x) + (vtkm::UInt32)0xaaaaaaaau) ^ (vtkm::UInt32)0xaaaaaaaau;
}



template <typename UInt, typename Int, vtkm::Int32 BlockSize>
inline VTKM_EXEC void fwd_order(UInt* ublock, const Int* iblock)
{
  const zfp::ZFPCodec<BlockSize> codec;
  for (vtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    vtkm::UInt8 idx = codec.CodecLookup(i);
    ublock[i] = int2uint<Int, UInt>(iblock[idx]);
  }
}

template <typename Int, vtkm::Int32 BlockSize>
inline VTKM_EXEC void fwd_xform(Int* p);

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int64, 64>(vtkm::Int64* p)
{
  vtkm::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<vtkm::Int64, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<vtkm::Int64, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<vtkm::Int64, 16>(p + 1 * x + 4 * y);
}

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int32, 64>(vtkm::Int32* p)
{
  vtkm::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<vtkm::Int32, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<vtkm::Int32, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<vtkm::Int32, 16>(p + 1 * x + 4 * y);
}

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int64, 16>(vtkm::Int64* p)
{
  vtkm::UInt32 x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    fwd_lift<vtkm::Int64, 1>(p + 4 * y);
  /* transform along y */
  for (x = 0; x < 4; x++)
    fwd_lift<vtkm::Int64, 4>(p + 1 * x);
}

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int32, 16>(vtkm::Int32* p)
{
  vtkm::UInt32 x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    fwd_lift<vtkm::Int32, 1>(p + 4 * y);
  /* transform along y */
  for (x = 0; x < 4; x++)
    fwd_lift<vtkm::Int32, 4>(p + 1 * x);
}

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int64, 4>(vtkm::Int64* p)
{
  /* transform along x */
  fwd_lift<vtkm::Int64, 1>(p);
}

template <>
inline VTKM_EXEC void fwd_xform<vtkm::Int32, 4>(vtkm::Int32* p)
{
  /* transform along x */
  fwd_lift<vtkm::Int32, 1>(p);
}

template <vtkm::Int32 BlockSize, typename PortalType, typename Int>
VTKM_EXEC void encode_block(BlockWriter<BlockSize, PortalType>& stream,
                            vtkm::Int32 maxbits,
                            vtkm::Int32 maxprec,
                            Int* iblock)
{
  using UInt = typename zfp_traits<Int>::UInt;

  fwd_xform<Int, BlockSize>(iblock);

  UInt ublock[BlockSize];
  fwd_order<UInt, Int, BlockSize>(ublock, iblock);

  vtkm::UInt32 intprec = CHAR_BIT * (vtkm::UInt32)sizeof(UInt);
  vtkm::UInt32 kmin =
    intprec > (vtkm::UInt32)maxprec ? intprec - static_cast<vtkm::UInt32>(maxprec) : 0;
  vtkm::UInt32 bits = static_cast<vtkm::UInt32>(maxbits);
  vtkm::UInt32 i, m;
  vtkm::UInt32 n = 0;
  vtkm::UInt64 x;
  /* encode one bit plane at a time from MSB to LSB */
  for (vtkm::UInt32 k = intprec; bits && k-- > kmin;)
  {
    /* step 1: extract bit plane #k to x */
    x = 0;
    for (i = 0; i < BlockSize; i++)
    {
      x += (vtkm::UInt64)((ublock[i] >> k) & 1u) << i;
    }
    /* step 2: encode first n bits of bit plane */
    m = vtkm::Min(n, bits);
    bits -= m;
    x = stream.write_bits(x, m);
    /* step 3: unary run-length encode remainder of bit plane */
    for (; n < BlockSize && bits && (bits--, stream.write_bit(!!x)); x >>= 1, n++)
    {
      for (; n < BlockSize - 1 && bits && (bits--, !stream.write_bit(x & 1u)); x >>= 1, n++)
      {
      }
    }
  }
}


template <vtkm::Int32 BlockSize, typename Scalar, typename PortalType>
inline VTKM_EXEC void zfp_encodef(Scalar* fblock,
                                  vtkm::Int32 maxbits,
                                  vtkm::UInt32 blockIdx,
                                  PortalType& stream)
{
  using Int = typename zfp::zfp_traits<Scalar>::Int;
  zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, vtkm::Id(blockIdx));
  vtkm::Int32 emax = zfp::MaxExponent<BlockSize, Scalar>(fblock);
  //  std::cout<<"EMAX "<<emax<<"\n";
  vtkm::Int32 maxprec =
    zfp::precision(emax, zfp::get_precision<Scalar>(), zfp::get_min_exp<Scalar>());
  vtkm::UInt32 e = vtkm::UInt32(maxprec ? emax + zfp::get_ebias<Scalar>() : 0);
  /* encode block only if biased exponent is nonzero */
  if (e)
  {

    const vtkm::UInt32 ebits = vtkm::UInt32(zfp::get_ebits<Scalar>()) + 1;
    blockWriter.write_bits(2 * e + 1, ebits);

    Int iblock[BlockSize];
    zfp::fwd_cast<Int, Scalar, BlockSize>(iblock, fblock, emax);

    encode_block<BlockSize>(blockWriter, maxbits - vtkm::Int32(ebits), maxprec, iblock);
  }
}

// helpers so we can do partial template instantiation since
// the portal type could be on any backend
template <vtkm::Int32 BlockSize, typename Scalar, typename PortalType>
struct ZFPBlockEncoder
{
};

template <vtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, vtkm::Float32, PortalType>
{
  VTKM_EXEC void encode(vtkm::Float32* fblock,
                        vtkm::Int32 maxbits,
                        vtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    zfp_encodef<BlockSize>(fblock, maxbits, blockIdx, stream);
  }
};

template <vtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, vtkm::Float64, PortalType>
{
  VTKM_EXEC void encode(vtkm::Float64* fblock,
                        vtkm::Int32 maxbits,
                        vtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    zfp_encodef<BlockSize>(fblock, maxbits, blockIdx, stream);
  }
};

template <vtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, vtkm::Int32, PortalType>
{
  VTKM_EXEC void encode(vtkm::Int32* fblock,
                        vtkm::Int32 maxbits,
                        vtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    using Int = typename zfp::zfp_traits<vtkm::Int32>::Int;
    zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, vtkm::Id(blockIdx));
    encode_block<BlockSize>(blockWriter, maxbits, get_precision<vtkm::Int32>(), (Int*)fblock);
  }
};

template <vtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, vtkm::Int64, PortalType>
{
  VTKM_EXEC void encode(vtkm::Int64* fblock,
                        vtkm::Int32 maxbits,
                        vtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    using Int = typename zfp::zfp_traits<vtkm::Int64>::Int;
    zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, vtkm::Id(blockIdx));
    encode_block<BlockSize>(blockWriter, maxbits, get_precision<vtkm::Int64>(), (Int*)fblock);
  }
};
}
}
} // namespace vtkm::worklet::zfp
#endif
