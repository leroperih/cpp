
// Wireless Mouse Handle = 0000000001570903



#include <windows.h>
#include <vector>
#include <iostream>

void ListMice() {
    UINT numDevices = 0;
    // Get the number of raw input devices
    if (GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)) != 0) return;

    std::vector<RAWINPUTDEVICELIST> deviceList(numDevices);
    // Populate the device list
    if (GetRawInputDeviceList(deviceList.data(), &numDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) return;

    for (const auto& device : deviceList)
    {
        if (device.dwType == RIM_TYPEMOUSE)
        {
            UINT nameSize = 0;
            // Get the size of the device name string
            GetRawInputDeviceInfo(device.hDevice, RIDI_DEVICENAME, NULL, &nameSize);

            if (nameSize > 0) {
                std::vector<TCHAR> deviceName(nameSize);
                if (GetRawInputDeviceInfo(device.hDevice, RIDI_DEVICENAME, deviceName.data(), &nameSize) != (UINT)-1) {
                    std::wcout << L"Mouse Handle: " << device.hDevice << L" | Name: " << deviceName.data() << std::endl;
                }
            }
        }
    }
}




int main()
{

    std::cout << std::endl;


    ListMice();


    system("pause");
    return 0;
}