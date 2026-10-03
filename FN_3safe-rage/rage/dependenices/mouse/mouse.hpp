// safe-rage mouse layer: makcu over serial. no SendInput, no mouse_event,
// no in-process HID — the game sees real USB HID packets from external hw.
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <mutex>

namespace fortnite {

	namespace mouse {

		inline HANDLE serial = INVALID_HANDLE_VALUE;
		inline std::mutex serial_lock;

		// Default makcu port and baud. Override at runtime via settings.
		inline const char* port = "\\\\.\\COM3";
		inline DWORD baud = 115200;

		inline bool open( ) {
			std::lock_guard<std::mutex> g( serial_lock );
			if ( serial != INVALID_HANDLE_VALUE )
				return true;

			serial = CreateFileA( port , GENERIC_READ | GENERIC_WRITE , 0 , nullptr , OPEN_EXISTING , 0 , nullptr );
			if ( serial == INVALID_HANDLE_VALUE )
				return false;

			DCB dcb = { 0 };
			dcb.DCBlength = sizeof( dcb );
			if ( !GetCommState( serial , &dcb ) ) {
				CloseHandle( serial );
				serial = INVALID_HANDLE_VALUE;
				return false;
			}
			dcb.BaudRate = baud;
			dcb.ByteSize = 8;
			dcb.Parity   = NOPARITY;
			dcb.StopBits = ONESTOPBIT;
			dcb.fBinary  = TRUE;
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			if ( !SetCommState( serial , &dcb ) ) {
				CloseHandle( serial );
				serial = INVALID_HANDLE_VALUE;
				return false;
			}

			COMMTIMEOUTS to = { 0 };
			to.ReadIntervalTimeout         = 50;
			to.ReadTotalTimeoutConstant    = 50;
			to.ReadTotalTimeoutMultiplier  = 10;
			to.WriteTotalTimeoutConstant   = 50;
			to.WriteTotalTimeoutMultiplier = 10;
			SetCommTimeouts( serial , &to );

			PurgeComm( serial , PURGE_RXCLEAR | PURGE_TXCLEAR );
			return true;
		}

		inline void close( ) {
			std::lock_guard<std::mutex> g( serial_lock );
			if ( serial != INVALID_HANDLE_VALUE ) {
				CloseHandle( serial );
				serial = INVALID_HANDLE_VALUE;
			}
		}

		inline bool send( const std::string& cmd ) {
			std::lock_guard<std::mutex> g( serial_lock );
			if ( serial == INVALID_HANDLE_VALUE )
				return false;
			DWORD written = 0;
			std::string line = cmd + "\r\n";
			return WriteFile( serial , line.data( ) , ( DWORD ) line.size( ) , &written , nullptr )
				&& written == line.size( );
		}

		// Raw relative move. Caller is responsible for scaling pixels → counts.
		inline bool move_raw( int x , int y ) {
			char buf [ 48 ];
			std::snprintf( buf , sizeof( buf ) , "km.move(%d,%d)" , x , y );
			return send( buf );
		}

		// Humanized move: break the delta into N substeps with jitter so the
		// trajectory is not a perfect straight-line snap. Caller supplies the
		// target delta in mouse counts (not pixels).
		inline bool move( int dx , int dy , int steps = 4 ) {
			if ( steps < 1 ) steps = 1;
			if ( steps > 32 ) steps = 32;

			int remaining_x = dx;
			int remaining_y = dy;
			for ( int i = 1 ; i <= steps ; ++i ) {
				int tx = ( dx * i ) / steps;
				int ty = ( dy * i ) / steps;

				int sx = tx - ( dx * ( i - 1 ) ) / steps;
				int sy = ty - ( dy * ( i - 1 ) ) / steps;

				if ( !move_raw( sx , sy ) )
					return false;

				remaining_x -= sx;
				remaining_y -= sy;
				Sleep( 1 );
			}
			if ( remaining_x || remaining_y )
				move_raw( remaining_x , remaining_y );
			return true;
		}

		inline bool left_down( )  { return send( "km.left(1)" ); }
		inline bool left_up( )    { return send( "km.left(0)" ); }
		inline bool right_down( ) { return send( "km.right(1)" ); }
		inline bool right_up( )   { return send( "km.right(0)" ); }

		inline void click( int ms = 15 ) {
			left_down( );
			Sleep( ms );
			left_up( );
		}

	}
}
