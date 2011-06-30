/*!
#
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
*/
#include "examples.h"
BOOL Examples_GetTestDevice( __deref_out KLST_HANDLE* DeviceList,
                             __deref_out PKLST_DEV_INFO* DeviceInfo,
                             __in int argc,
                             __in char* argv[])
{
	return Examples_GetTestDeviceEx(DeviceList,
	                                DeviceInfo,
	                                argc,
	                                argv,
	                                NULL);

}
BOOL Examples_GetTestDeviceEx( __deref_out KLST_HANDLE* DeviceList,
                               __deref_out PKLST_DEV_INFO* DeviceInfo,
                               __in int argc,
                               __in char* argv[],
                               __in_opt PKLST_INIT_PARAMS InitParams)
{
	ULONG vidArg = EXAMPLE_VID;
	ULONG pidArg = EXAMPLE_PID;
	ULONG deviceCount = 0;
	int argPos;
	KLST_HANDLE deviceList = NULL;
	PKLST_DEV_INFO deviceInfo = NULL;

	// init
	*DeviceList = NULL;
	*DeviceInfo = NULL;

	// Get the test device vid/pid from the command line (if specified)
	for (argPos = 1; argPos < argc; argPos++)
	{
		sscanf(argv[argPos], "vid=%04x", &vidArg);
		sscanf(argv[argPos], "pid=%04x", &pidArg);
	}

	// Get the device list
	if (!LstK_Init(&deviceList, InitParams))
	{
		printf("Error initializing device list.\n");
		return FALSE;
	}

	LstK_Count(deviceList, &deviceCount);
	if (!deviceCount)
	{
		printf("No device not connected.\n");
		SetLastError(ERROR_DEVICE_NOT_CONNECTED);

		// If LstK_Init returns TRUE, the list must be freed.
		LstK_Free(&deviceList);

		return FALSE;
	}

	printf("Looking for device vid/pid %04X/%04X..\n", vidArg, pidArg);

	LstK_FindByVidPid(deviceList, vidArg, pidArg, &deviceInfo);

	if (deviceInfo)
	{
		// This function returns the device list and the device info
		// element which matched.  The caller is responsible for freeing
		// this list when it is no longer needed.
		*DeviceList = deviceList;
		*DeviceInfo = deviceInfo;

		// Report the connection state of the example device
		printf("Using %04X:%04X (%s): %s - %s\n",
		       deviceInfo->Common.Vid,
		       deviceInfo->Common.Pid,
		       deviceInfo->Common.InstanceID,
		       deviceInfo->DeviceDesc,
		       deviceInfo->Mfg);

		return TRUE;
	}
	else
	{
		// Display some simple usage information for the example applications.
		CHAR programPath[MAX_PATH] = {0};
		PCHAR programExe = programPath;
		GetModuleFileNameA(GetModuleHandleA(NULL), programPath, sizeof(programPath));
		while(strpbrk(programExe, "\\/")) programExe = strpbrk(programExe, "\\/") + 1;
		printf("Device vid/pid %04X/%04X not found.\n\n", vidArg, pidArg);
		printf("USAGE: %s vid=%04X pid=%04X\n\n", programExe, vidArg, pidArg);

		// If LstK_Init returns TRUE, the list must be freed.
		LstK_Free(&deviceList);

		return FALSE;
	}
}