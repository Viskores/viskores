# Make RuntimeDeviceInformation class template independent

By making RuntimeDeviceInformation class template independent, vtkm is able to detect
device info at runtime with a runtime specified deviceId. In the past it's impossible
because the CRTP pattern does not allow function overloading(compiler would complain
that DeviceAdapterRuntimeDetector does not have Exists() function defined).



