/*! \file lusbk_device_list.h
* \brief structs, typedefs, enums, defines, and functions for usb device enumeration and detection.
*/

#ifndef __LUSBK_DEVICE_LIST_H
#define __LUSBK_DEVICE_LIST_H

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"

#include <PSHPACK1.H>

/*! \addtogroup lstk
* @{
*/

//!  Allocated length for all strings in a \ref KLST_DEV_INFO structure.
#define KLST_STRING_MAX_LEN 256

typedef enum _KLST_SYNC_FLAG
{
    SYNC_FLAG_NONE			= 0,
    SYNC_FLAG_UNCHANGED		= 1 << 0,
    SYNC_FLAG_ADDED			= 1 << 1,
    SYNC_FLAG_REMOVED		= 1 << 2,
    SYNC_FLAG_MASK			= (SYNC_FLAG_REMOVED - 1) | SYNC_FLAG_REMOVED,
} KLST_SYNC_FLAG;

typedef struct _KLST_SYNC_PARAMS
{
	ULONG _ununsed;
} KLST_SYNC_PARAMS;
typedef KLST_SYNC_PARAMS* PKLST_SYNC_PARAMS;

//! Common usb device information structure
typedef struct _KLST_DEV_COMMON_INFO
{
	//! VendorID parsed from \ref KLST_DEV_INFO::InstanceID
	UINT Vid;

	//! ProductID parsed from \ref KLST_DEV_INFO::InstanceID
	UINT Pid;

	//! Interface number (valid for composite devices only) parsed from \ref KLST_DEV_INFO::InstanceID
	UINT MI;

	// An ID that uniquely identifies a USB device.
	CHAR InstanceID[KLST_STRING_MAX_LEN];

} KLST_DEV_COMMON_INFO;
//! Pointer to a \c KLST_DEV_COMMON_INFO structure.
typedef KLST_DEV_COMMON_INFO* PKLST_DEV_COMMON_INFO;

//! USB device information element of a \ref KLST_DEV_LIST collection.
/*!
* Contains information about a usd device retreived from the windows
*
* \attention This structure is semi-opaque.
*
* All \ref KLST_DEV_INFO elements contain a \ref KUSB_USER_CONTEXT.
* This 32 bytes of user context space can be used by you, the developer, for any desired purpose.
*/
typedef struct _KLST_DEV_INFO
{
	//! User context area
	KUSB_USER_CONTEXT UserContext;

	//! Common usb device information
	KLST_DEV_COMMON_INFO Common;

	//! Driver id this device element is using
	LONG DrvId;

	//! Device interface GUID
	CHAR DeviceInterfaceGUID[KLST_STRING_MAX_LEN];

	//! Device instance ID
	/*!
	* A Device instance ID has the following format:
	* [enumerator]\[enumerator-specific-device-ID]\[instance-specific-ID]
	* - [enumerator]
	*   - For USB device, the enumerator is always \c USB
	* - [enumerator-specific-device-ID]
	*   - Contains the vendor and product id (VID_xxxx&PID_xxxx)
	*   - If present, contains the usbccgp (windows composite device layer) interface number (MI_xx)
	* - [instance-specific-ID]
	*   - If the device is composite, contains a unqiue interface ID generated by Windows.
	*   - If the device is not composite and has a serial number, contains the devices serial number.
	*   - If the device does not have a serial number, contains a unqiue ID generated by Windows.
	*/
	CHAR InstanceID[KLST_STRING_MAX_LEN];

	//! Class GUID
	CHAR ClassGUID[KLST_STRING_MAX_LEN];

	//! Manufaturer name as specified in the INF file
	CHAR Mfg[KLST_STRING_MAX_LEN];

	//! Device description as specified in the INF file
	CHAR DeviceDesc[KLST_STRING_MAX_LEN];

	//! Driver service name
	CHAR Service[KLST_STRING_MAX_LEN];

	//! Unique symbolic link identifier
	/*!
	* The \c SymbolicLink can be used to uniquely distinguish between device list elements.
	*/
	CHAR SymbolicLink[KLST_STRING_MAX_LEN];

	//! physical device filename.
	/*!
	* This path is used with the Windows \c CreateFile() function to obtain on opened device handle.
	*/
	CHAR DevicePath[KLST_STRING_MAX_LEN];

	//! libusb-win32 filter index id.
	DWORD LUsb0FilterIndex;

	//! Indicates the devices connection state.
	BOOL Connected;

	union
	{
		struct
		{
			KLST_SYNC_FLAG SyncFlags;
			ULONG UserFlags;
		};
		struct
		{
			unsigned Unchanged: 1;
			unsigned Added: 1;
			unsigned Removed: 1;
		};
	} SyncResults;

	//! see \ref KLST_INIT_PARAMS::EnableCompositeDeviceMode
	struct _KLST_DEV_LIST* CompositeList;

} KLST_DEV_INFO;
//! pointer to a \ref KLST_DEV_INFO
typedef KLST_DEV_INFO* PKLST_DEV_INFO;
//! pointer to a constant \ref KLST_DEV_INFO
typedef const KLST_DEV_INFO* PCKLST_DEV_INFO;

//! Pointer to a device list handle.
/*!
* \attention This is an opaque pointer.
*/
typedef HANDLE KLST_HANDLE;

//! Initialization parameters for \ref LstK_Init
typedef struct _KLST_INIT_PARAMS
{
	//! Enable listings for the raw device interface GUID.{A5DCBF10-6530-11D2-901F-00C04FB951ED}
	BOOL EnableRawDeviceInterfaceGuid;

	//! Enable composite device list mode
	/*!
	* When \c EnableCompositeDeviceMode is TRUE, composite devices are merged into a single \ref KLST_DEV_INFO and
	* \ref KLST_DEV_INFO::CompositeList is populated with the individual composite device elements.
	*
	*/
	BOOL EnableCompositeDeviceMode;

	BOOL ShowDisconnectedDevices;

} KLST_INIT_PARAMS, *PKLST_INIT_PARAMS;

#include <POPPACK.H>


//! Enumeration callback typedef (or delegate).
/*!
* Use this typdef as a prototype for an enumeration function in \ref LstK_Enumerate.
* \param DeviceList
* The device list \c DeviceInfo belongs to
*
* \param DeviceInfo
* Device information
*
* \param Context
* User context that was passed into \ref LstK_Enumerate
*
*/
typedef BOOL KUSB_API KLST_DEV_ENUM_CB (
    __in KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo,
    __in PVOID Context);

// Pointer to a \c KLST_DEV_ENUM_CB
typedef KLST_DEV_ENUM_CB* PKLST_DEV_ENUM_CB;

#ifdef __cplusplus
extern "C" {
#endif

//! Initializes a new usb device list.
	/*!
	*
	* \c LstK_Init populates \c DeviceList with connected usb devices that can be used by libusbK.
	*
	* \note if \ref LstK_Init returns TRUE, the device list must be freed with \ref LstK_Free when it is no longer needed.
	*
	* \param DeviceList
	* Pointer reference that will receive a populated device list.
	*
	* \param InitParameters
	* Search, filter, and listing options. see \c KLST_INIT_PARAMS
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Init(
	    __deref_inout KLST_HANDLE* DeviceList,
	    __in PKLST_INIT_PARAMS InitParameters);

//! Frees a usb device list.
	/*!
	* Frees all resources that were allocated to \c DeviceList by \ref LstK_Init.
	*
	* \note if \ref LstK_Init returns TRUE, the device list must be freed with \ref LstK_Free when it is no longer needed.
	*
	* \param DeviceList
	* The \c DeviceList to free.
	*
	* \returns NONE
	*/
	KUSB_EXP VOID KUSB_API LstK_Free(
	    __deref_inout KLST_HANDLE* DeviceList);

//! Enumerates \ref KLST_DEV_INFO elements of a \ref KLST_DEV_LIST.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to enumerate.
	*
	* \param EnumDevListCB
	* Function to call for each iteration.
	*
	* \param Context
	* Optional user context pointer.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* Calls \c EnumDevListCB for each element in the device list or until \c EnumDevListCB returns FALSE.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Enumerate(
	    __in KLST_HANDLE DeviceList,
	    __in PKLST_DEV_ENUM_CB EnumDevListCB,
	    __in_opt PVOID Context);

//! Gets the \ref KLST_DEV_INFO element for the current position.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to retrieve a current \ref KLST_DEV_INFO for.
	*
	* \param DeviceInfo
	* The device information.
	*
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* After a \c DeviceList is created or after the \ref LstK_Reset method is
	* called, the \c LstK_MoveNext method must be called to advance the device
	* list enumerator to the first element of the \c DeviceList before calling
	* \c LstK_Current otherwise, \c DeviceInfo is undefined.
	*
	* \c LstK_Current returns \c FALSE and sets last error to \c
	* ERROR_NO_MORE_ITEMS if the last call to \c LstK_MoveNext returned \c
	* FALSE, which indicates the end of the \c DeviceList.
	*
	* \c LstK_Current does not move the position of the device list
	* enumerator, and consecutive calls to \c LstK_Current return the same
	* object until either \c LstK_MoveNext or \ref LstK_Reset is called.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Current(
	    __in KLST_HANDLE DeviceList,
	    __deref_out PKLST_DEV_INFO* DeviceInfo);

//! Advances the device list current \ref KLST_DEV_INFO position.
	/*!
	* \param DeviceList
	* A usb device list returned by \ref LstK_Init
	*
	* \param DeviceInfo [OPTIONAL]
	* On success, contains a pointer to the device information for the current enumerators position.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* After a \c DeviceList is created or after \ref LstK_Reset is called, an
	* enumerator is positioned before the first element of the \c DeviceList
	* and the \b first call to \c LstK_MoveNext moves the enumerator over the
	* first element of the \c DeviceList.
	*
	* If \c LstK_MoveNext passes the end of the \c DeviceList, the enumerator
	* is positioned after the last element in the \c DeviceList and \c
	* LstK_MoveNext returns \c FALSE. When the enumerator is at this position,
	* a subsequent call to \c LstK_MoveNext will reset the enumerator and it
	* continues from the beginning.
	*
	*/
	KUSB_EXP BOOL KUSB_API LstK_MoveNext(
	    __inout KLST_HANDLE DeviceList,
	    __deref_out_opt PKLST_DEV_INFO* DeviceInfo);

//! Sets the device list to its initial position, which is before the first element in the list.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to retrieve a current \ref KLST_DEV_INFO for.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP VOID KUSB_API LstK_Reset(
	    __inout KLST_HANDLE DeviceList);

//! Find a device by vendor and product id
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to retrieve a current \ref KLST_DEV_INFO for.
	*
	* \param Vid
	* ID is used in conjunction with the \c Pid to uniquely identify USB
	* devices, providing traceability to the OEM.
	*
	* \param Pid
	* ID is used in conjunction with the \c Pid to uniquely identify USB
	* devices, providing traceability to the OEM.
	*
	* \param DeviceInfo
	* On success, the device information pointer, otherwise NULL.
	*
	* \returns
	* - TRUE if the device was found
	* - FALSE if the device was \b not found or an error occured.
	*   - Sets last error to \c ERROR_NO_MORE_ITEMS if the device was \b not found.
	*
	* Searches all elements in \c DeviceList for usb device matching the specified.
	*/
	KUSB_EXP BOOL KUSB_API LstK_FindByVidPid(
	    __in KLST_HANDLE DeviceList,
	    __in UINT Vid,
	    __in UINT Pid,
	    __deref_out PKLST_DEV_INFO* DeviceInfo);

//! Locks access to the device list.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to lock.
	*
	* \param wait
	* If FALSE, do not wait for the lock to acquire, instead return eith FALSE.
	*
	* \returns TRUE of the device list was locked, Otherwise FALSE.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Lock(
	    __in KLST_HANDLE DeviceList,
	    __in BOOL wait);

//! Unlocks access to the device list.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to lock.
	*
	* \returns On success, TRUE. Otherwise FALSE.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Unlock(
	    __in KLST_HANDLE DeviceList);

	KUSB_EXP BOOL KUSB_API LstK_Count(
	    __in KLST_HANDLE DeviceList,
	    __inout PULONG Count);

	KUSB_EXP BOOL KUSB_API LstK_Sync(
	    __inout KLST_HANDLE MasterList,
	    __in KLST_HANDLE SlaveList,
	    __in_opt PKLST_SYNC_PARAMS SyncParams);

//! Creates a copy of an existing device list.
	/*!
	*
	* \param SrcList
	* The device list to copy.
	*
	* \param DstList
	* Reference to a pointer that receives the cloned device list.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Clone(
	    __in KLST_HANDLE SrcList,
	    __out KLST_HANDLE* DstList);

	KUSB_EXP BOOL KUSB_API LstK_CloneDevInfo(
	    __in PKLST_DEV_INFO SrcInfo,
	    __deref_inout PKLST_DEV_INFO* DstInfo);

	KUSB_EXP BOOL KUSB_API LstK_RemoveDevInfo(
	    __inout KLST_HANDLE DeviceList,
	    __in PKLST_DEV_INFO DeviceInfo);

	KUSB_EXP BOOL KUSB_API LstK_AddDevInfo(
	    __inout KLST_HANDLE DeviceList,
	    __in PKLST_DEV_INFO DeviceInfo);

	KUSB_EXP BOOL KUSB_API LstK_FreeDevInfo(
	    __deref_inout PKLST_DEV_INFO* DeviceInfo);

#ifdef __cplusplus
}
#endif

/*! @} */
#endif
