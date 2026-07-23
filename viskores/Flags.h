//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_Flags_h
#define viskores_Flags_h

namespace viskores
{

/// @brief Identifier used to specify whether a function should deep copy data.
enum struct CopyFlag
{
  Off = 0,
  On = 1
};

}

#endif // viskores_Flags_h
