#include <conio.h>  
#include <stdio.h>  
#include <iostream>
#pragma comment(lib, "hid.lib")

#include <windows.h>
#include <mmsystem.h>  
#include <hidsdi.h>

union { unsigned long word; unsigned char data[4]; } message;
HMIDIOUT device;
unsigned int lastValue = -1;

void Sustain(HMIDIOUT device, char velocity) {
	message.data[0] = 0xb0;
	message.data[1] = 64;
	message.data[2] = velocity;
	message.data[3] = 0;
	midiOutShortMsg(device, message.word);
}


LRESULT CALLBACK winproc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	if (msg == WM_INPUT) {
		UINT bufferSize;
		GetRawInputData((HRAWINPUT)lp, RID_INPUT, 0, &bufferSize, sizeof(RAWINPUTHEADER));
	
		RAWINPUT rawInput;
		GetRawInputData((HRAWINPUT)lp, RID_INPUT, &rawInput, &bufferSize, sizeof(RAWINPUTHEADER));

		GetRawInputDeviceInfo(rawInput.header.hDevice, RIDI_PREPARSEDDATA, 0, &bufferSize);
		PHIDP_PREPARSED_DATA preparsedData = (PHIDP_PREPARSED_DATA)malloc(bufferSize);

		GetRawInputDeviceInfo(rawInput.header.hDevice, RIDI_PREPARSEDDATA, preparsedData, &bufferSize);

		HIDP_CAPS caps;
		HidP_GetCaps(preparsedData, &caps);
		USHORT axisCount = caps.NumberInputValueCaps;
		PHIDP_VALUE_CAPS valueCaps = (PHIDP_VALUE_CAPS)malloc(sizeof(HIDP_VALUE_CAPS)*axisCount);
		HidP_GetValueCaps(HidP_Input, valueCaps, &axisCount, preparsedData);

		int axis_id = 3;
		ULONG value = 0;
		HidP_GetUsageValue(HidP_Input, valueCaps[axis_id].UsagePage, 0, valueCaps[axis_id].Range.UsageMin, &value, preparsedData, (PCHAR)rawInput.data.hid.bRawData, rawInput.data.hid.dwSizeHid);

		int val = (134.f - (float)value) / 134.f * 77.f + 50.f;
		if (val != lastValue) {
			Sustain(device, val);
			lastValue = val;
		}

		free(valueCaps);
		free(preparsedData);
	}
	return 0;
}


int main(int argc, char** argv) {
	ShowWindow(GetConsoleWindow(), false);
	MMRESULT result = midiOutOpen(&device, 1, 0, 0, CALLBACK_NULL);
	
	while (result != 0) {
		result = midiOutOpen(&device, 1, 0, 0, CALLBACK_NULL);
		Sleep(5000);
	}

	HWND dummy_wnd = CreateWindowEx(0, "STATIC", "Shit", 0, 0, 0, 100, 100, 0, 0, 0, 0);

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 1;
	rid.usUsage = 4;
	rid.dwFlags = RIDEV_INPUTSINK;
	rid.hwndTarget = dummy_wnd;
	
	SetWindowLong(dummy_wnd, GWL_WNDPROC, (LONG)winproc);

	while (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		Sleep(5000);
	}
		

	MSG msg;
	while (GetMessage(&msg, dummy_wnd, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	

	midiOutReset(device);

	midiOutClose(device);

	return 0;
}
