//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_Error_h
#define vtk_m_cont_Error_h

// Note that this class and (most likely) all of its subclasses are not
// templated.  If there is any reason to create a VTKm control library,
// this class and its subclasses should probably go there.

#include <exception>
#include <string>

#include <vtkm/cont/Logging.h>

#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace cont
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// The superclass of all exceptions thrown by any VTKm function or method.
///
class VTKM_ALWAYS_EXPORT Error : public std::exception
{
public:
//See note about GetMessage macro below.
#ifndef GetMessage
  const std::string& GetMessage() const { return this->Message; }
#endif
  const std::string& GetStackTrace() const { return this->StackTrace; }

//GetMessage is a macro defined by <windows.h> to redirrect to
//GetMessageA or W depending on if you are using ansi or unicode.
//To get around this we make our own A/W variants on windows.
#ifdef _WIN32
  const std::string& GetMessageA() const { return this->Message; }
  const std::string& GetMessageW() const { return this->Message; }
#endif

  // For std::exception compatibility:
  const char* what() const noexcept override { return this->What.c_str(); }

  /// Returns true if this exception is device independent. For exceptions that
  /// are not device independent, `vtkm::TryExecute`, for example, may try
  /// executing the code on other available devices.
  bool GetIsDeviceIndependent() const { return this->IsDeviceIndependent; }

protected:
  Error() {}
  Error(const std::string& message, bool is_device_independent = false)
    : Message(message)
    , StackTrace(vtkm::cont::GetStackTrace(1))
    , What(Message + "\n" + StackTrace)
    , IsDeviceIndependent(is_device_independent)
  {
  }

  void SetMessage(const std::string& message) { this->Message = message; }

private:
  std::string Message;
  std::string StackTrace;
  std::string What;
  bool IsDeviceIndependent;
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Error_h
