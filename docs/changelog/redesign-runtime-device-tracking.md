# Redesign Runtime Device Tracking

The device tracking infrastructure in VTK-m has been redesigned to
remove multiple redundant codes paths and to simplify reasoning
about around what an instance of RuntimeDeviceTracker will modify.

