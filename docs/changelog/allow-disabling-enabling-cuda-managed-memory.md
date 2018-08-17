Allow disabling/enabling of CUDA managed memory through a environment variable

By setting the environment variable "VTKM_MANAGEDMEMO_DISABLED" to be 1,
users are able to disable CUDA managed memory even though the hardware is capable
of doing so.

