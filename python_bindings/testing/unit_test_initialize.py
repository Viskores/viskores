##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import viskores.cont


def main():
    assert not viskores.cont.IsInitialized()

    argv = ["python-test", "--viskores-device", "Any", "--application-flag", "value"]
    result = viskores.cont.Initialize(argv)

    assert viskores.cont.IsInitialized()
    assert isinstance(result, viskores.cont.InitializeResult)
    assert isinstance(result.Device, viskores.cont.DeviceAdapterId)
    assert result.Device.GetName().lower() == "any"
    assert argv == ["python-test", "--application-flag", "value"]

    combined = (
        viskores.cont.InitializeOptions.AddHelp
        | viskores.cont.InitializeOptions.ErrorOnBadOption
        | viskores.cont.InitializeOptions.ErrorOnBadArgument
    )
    assert combined == viskores.cont.InitializeOptions.Strict


if __name__ == "__main__":
    main()
