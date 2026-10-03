// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "memory.hpp"

bool fortnite::communcations::find_driver( )
{
	driver_handle = CreateFileW( L"\\\\.\\WinKernelInterface" , GENERIC_READ | GENERIC_WRITE , 0 , nullptr , OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , nullptr );
	if ( driver_handle == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	return true;
}

uintptr_t fortnite::communcations::get_base( )
{
	uintptr_t image_address = 0;
	_BA Arguments {};

	Arguments.Security = 0x1E2E3F;
	Arguments.ProcessID = process_id;
	Arguments.Address = ( ULONGLONG* ) &image_address;

	DeviceIoControl( driver_handle , 0x1889 , &Arguments , sizeof( Arguments ) , nullptr , 0 , nullptr , nullptr );
	base_address = image_address;
	return image_address;
}
INT32 fortnite::communcations::get_process_id( LPCTSTR process_name )
{
	PROCESSENTRY32 pt;
	HANDLE hsnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
	pt.dwSize = sizeof( PROCESSENTRY32 );
	if ( Process32First( hsnap , &pt ) ) {
		do {
			if ( !lstrcmpi( pt.szExeFile , process_name ) )
			{
				CloseHandle( hsnap );
				process_id = pt.th32ProcessID;
				return pt.th32ProcessID;
			}
		} while ( Process32Next( hsnap , &pt ) );
	}

	CloseHandle( hsnap );
	return process_id;
}
bool fortnite::communcations::read_memory( PVOID address , PVOID buffer , DWORD size )
{
	_ReadWrite Arguments = { 0 };
	Arguments.Security = 0x1E2E3F;
	Arguments.Address = ( ULONGLONG ) address;
	Arguments.Buffer = ( ULONGLONG ) buffer;
	Arguments.Size = size;
	Arguments.ProcessID = process_id;
	Arguments.Write = FALSE;
	Arguments.EAC = TRUE;

	return DeviceIoControl( driver_handle , 0x1999 , &Arguments , sizeof( Arguments ) , nullptr , 0 , nullptr , nullptr );
}
bool fortnite::communcations::write_memory( PVOID address , PVOID buffer , DWORD size )
{
	_ReadWrite Arguments = { 0 };
	Arguments.Security = 0x1E2E3F;
	Arguments.Address = ( ULONGLONG ) address;
	Arguments.Buffer = ( ULONGLONG ) buffer;
	Arguments.Size = size;
	Arguments.ProcessID = process_id;
	Arguments.Write = TRUE;
	Arguments.EAC = TRUE;

	return DeviceIoControl( driver_handle , 0x1999 , &Arguments , sizeof( Arguments ) , nullptr , 0 , nullptr , nullptr );
}
bool fortnite::communcations::get_cr3( )
{
	bool Ret = false;
	_DTB Arguments = { 0 };
	Arguments.Security = 0x1E2E3F;
	Arguments.ProcessID = process_id;
	Arguments.Operation = ( bool* ) &Ret;

	return DeviceIoControl( driver_handle , 0x1299 , &Arguments , sizeof( Arguments ) , nullptr , 0 , nullptr , nullptr );
}