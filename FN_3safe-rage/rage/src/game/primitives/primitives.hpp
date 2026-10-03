// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../../dependenices/memory/memory.hpp"
#include <cmath>
#include <algorithm>
#include <numbers>
#include <d3d9.h>
#include <memory>

namespace fortnite {
	namespace uemath {
		
		class fvector
		{
		public:
			fvector( ) : x( 0.f ) , y( 0.f ) , z( 0.f ) {}

			fvector( double _x , double _y , double _z ) : x( _x ) , y( _y ) , z( _z ) {}

			~fvector( ) {}

			double x;
			double y;
			double z;

			inline double dot( fvector v )
			{
				return x * v.x + y * v.y + z * v.z;
			}
			inline double distance( fvector v )
			{
				return double( sqrtf( powf( v.x - x , 2.0 ) + powf( v.y - y , 2.0 ) + powf( v.z - z , 2.0 ) ) );
			}
			inline double length( )
			{
				return sqrt( x * x + y * y + z * z );
			}
			bool is_zero( )
			{
				return x == 0 && y == 0 && z == 0;
			}
			void normalize( )
			{
				while ( x > 180.0f ) x -= 360.0f;
				while ( x < -180.0f ) x += 360.0f;
				while ( y > 180.0f ) y -= 360.0f;
				while ( y < -180.0f ) y += 360.0f;
				z = 0;
			}
			fvector operator+( fvector v )
			{
				return fvector( x + v.x , y + v.y , z + v.z );
			}
			fvector operator-( fvector v )
			{
				return fvector( x - v.x , y - v.y , z - v.z );
			}
			fvector operator*( double number ) const
			{
				return fvector( x * number , y * number , z * number );
			}
			fvector operator/( double number ) const
			{
				return fvector( x / number , y / number , z / number );
			}
		};

		class fvector2d {
		public:
			double x , y;

			fvector2d( double x = 0.0 , double y = 0.0 ) : x( x ) , y( y ) {}

			fvector2d operator+( const fvector2d& v ) const {
				return fvector2d( x + v.x , y + v.y );
			}
			fvector2d operator-( const fvector2d& v ) const {
				return fvector2d( x - v.x , y - v.y );
			}
			fvector2d operator*( double scalar ) const {
				return fvector2d( x * scalar , y * scalar );
			}
			bool is_zero( ) const {
				return x == 0.0 && y == 0.0;
			}
		};

		class frotator {
		public:
			frotator( ) : pitch( 0 ) , yaw( 0 ) , roll( 0 ) {}
			frotator( double Pitch , double Yaw , double Roll ) : pitch( Pitch ) , yaw( Yaw ) , roll( Roll ) {}

			frotator operator + ( const frotator& other ) const { return { this->pitch + other.pitch, this->yaw + other.yaw, this->roll + other.roll }; }
			frotator operator - ( const frotator& other ) const { return { this->pitch - other.pitch, this->yaw - other.yaw, this->roll - other.roll }; }
			frotator operator * ( double offset ) const { return { this->pitch * offset, this->yaw * offset, this->roll * offset }; }
			frotator operator / ( double offset ) const { return { this->pitch / offset, this->yaw / offset, this->roll / offset }; }

			frotator& operator = ( const double other ) { this->pitch = other; this->yaw = other; this->roll = other; return *this; }
			frotator& operator *= ( const double other ) { this->pitch *= other; this->yaw *= other; this->roll *= other; return *this; }
			frotator& operator /= ( const double other ) { this->pitch /= other; this->yaw /= other; this->roll /= other; return *this; }

			frotator& operator = ( const frotator& other ) { this->pitch = other.pitch; this->yaw = other.yaw; this->roll = other.roll; return *this; }
			frotator& operator += ( const frotator& other ) { this->pitch += other.pitch; this->yaw += other.yaw; this->roll += other.roll; return *this; }
			frotator& operator -= ( const frotator& other ) { this->pitch -= other.pitch; this->yaw -= other.yaw; this->roll -= other.roll; return *this; }
			frotator& operator /= ( const frotator& other ) { this->pitch /= other.pitch; this->yaw /= other.yaw; this->roll /= other.roll; return *this; }

			operator bool( ) const { return this->pitch != 0 && this->yaw != 0 && this->roll != 0; }

			friend bool operator == ( const frotator& a , const frotator& b ) { return a.pitch == b.pitch && a.yaw == b.yaw && a.roll == b.roll; }
			friend bool operator != ( const frotator& a , const frotator& b ) { return !( a == b ); }

			frotator get( ) const {
				return frotator( pitch , yaw , roll );
			}
			void set( double _Pitch , double _Yaw , double _Roll ) {
				pitch = _Pitch;
				yaw = _Yaw;
				roll = _Roll;
			}
			frotator normalize( ) const {
				frotator result = get( );

				if ( std::isfinite( result.pitch ) && std::isfinite( result.yaw ) && std::isfinite( result.roll ) ) {
					result.pitch = std::clamp( result.pitch , -89.0 , 89.0 );
					result.yaw = std::clamp( result.yaw , -180.0 , 180.0 );
					result.roll = 0.0;
				}

				return result;
			}
			constexpr float deg_to_rad( float deg )
			{
				return deg * ( 3.14159265358979323846f / 180.0f );
			}
			fvector get_forward_vector( ) {
				auto pitch_degree = deg_to_rad( this->pitch );
				auto yaw_degree = deg_to_rad( this->yaw );

				auto cp = cos( pitch_degree );
				auto sp = sin( pitch_degree );
				auto cy = cos( yaw_degree );
				auto sy = sin( yaw_degree );

				return fvector( cp * cy , cp * sy , sp );
			}

			

			double length( ) const {
				return std::sqrt( pitch * pitch + yaw * yaw + roll * roll );
			}

			double dot( const frotator& V ) const { return pitch * V.pitch + yaw * V.yaw + roll * V.roll; }

			double distance( const frotator& V ) const {
				return std::sqrt( std::pow( V.pitch - this->pitch , 2.0 ) + std::pow( V.yaw - this->yaw , 2.0 ) + std::pow( V.roll - this->roll , 2.0 ) );
			}

			double pitch;
			double yaw;
			double roll;
		};
		struct fplane : fvector
		{
			double w = 0;
		};
		struct box3d
		{
			fvector corners [ 8 ];
		};
	}

	namespace ueegnine {
		struct fquat {
			float w , x , y , z;

			fquat( ) : w( 1 ) , x( 0 ) , y( 0 ) , z( 0 ) {}

			fquat( float w , float x , float y , float z ) : w( w ) , x( x ) , y( y ) , z( z ) {}
		};

		struct ftransform
		{
			uemath::fplane rot;
			uemath::fvector translation;
			char pad_38 [ 4 ];
			uemath::fvector scale = { 1.0, 1.0, 1.0 };
			char pad_58 [ 4 ];

			inline D3DMATRIX to_matrix_with_scale( ) const
			{
				D3DMATRIX m {};
				m._41 = translation.x;
				m._42 = translation.y;
				m._43 = translation.z;

				auto x2 = rot.x + rot.x , y2 = rot.y + rot.y , z2 = rot.z + rot.z;
				auto xx2 = rot.x * x2 , yy2 = rot.y * y2 , zz2 = rot.z * z2;
				m._11 = ( 1.0f - ( yy2 + zz2 ) ) * scale.x;
				m._22 = ( 1.0f - ( xx2 + zz2 ) ) * scale.y;
				m._33 = ( 1.0f - ( xx2 + yy2 ) ) * scale.z;

				auto yz2 = rot.y * z2 , wx2 = rot.w * x2;
				m._32 = ( yz2 - wx2 ) * scale.z;
				m._23 = ( yz2 + wx2 ) * scale.y;

				auto xy2 = rot.x * y2 , wz2 = rot.w * z2;
				m._21 = ( xy2 - wz2 ) * scale.y;
				m._12 = ( xy2 + wz2 ) * scale.x;

				auto xz2 = rot.x * z2 , wy2 = rot.w * y2;
				m._31 = ( xz2 + wy2 ) * scale.z;
				m._13 = ( xz2 - wy2 ) * scale.x;

				m._14 = m._24 = m._34 = 0.0f;
				m._44 = 1.0f;

				return m;
			}
		};

		inline D3DMATRIX matrix_multiplication( const D3DMATRIX& matrix1 , const D3DMATRIX& matrix2 )
		{
			D3DMATRIX result {};

			result._11 = matrix1._11 * matrix2._11 + matrix1._12 * matrix2._21 + matrix1._13 * matrix2._31 + matrix1._14 * matrix2._41;
			result._12 = matrix1._11 * matrix2._12 + matrix1._12 * matrix2._22 + matrix1._13 * matrix2._32 + matrix1._14 * matrix2._42;
			result._13 = matrix1._11 * matrix2._13 + matrix1._12 * matrix2._23 + matrix1._13 * matrix2._33 + matrix1._14 * matrix2._43;
			result._14 = matrix1._11 * matrix2._14 + matrix1._12 * matrix2._24 + matrix1._13 * matrix2._34 + matrix1._14 * matrix2._44;

			result._21 = matrix1._21 * matrix2._11 + matrix1._22 * matrix2._21 + matrix1._23 * matrix2._31 + matrix1._24 * matrix2._41;
			result._22 = matrix1._21 * matrix2._12 + matrix1._22 * matrix2._22 + matrix1._23 * matrix2._32 + matrix1._24 * matrix2._42;
			result._23 = matrix1._21 * matrix2._13 + matrix1._22 * matrix2._23 + matrix1._23 * matrix2._33 + matrix1._24 * matrix2._43;
			result._24 = matrix1._21 * matrix2._14 + matrix1._22 * matrix2._24 + matrix1._23 * matrix2._34 + matrix1._24 * matrix2._44;

			result._31 = matrix1._31 * matrix2._11 + matrix1._32 * matrix2._21 + matrix1._33 * matrix2._31 + matrix1._34 * matrix2._41;
			result._32 = matrix1._31 * matrix2._12 + matrix1._32 * matrix2._22 + matrix1._33 * matrix2._32 + matrix1._34 * matrix2._42;
			result._33 = matrix1._31 * matrix2._13 + matrix1._32 * matrix2._23 + matrix1._33 * matrix2._33 + matrix1._34 * matrix2._43;
			result._34 = matrix1._31 * matrix2._14 + matrix1._32 * matrix2._24 + matrix1._33 * matrix2._34 + matrix1._34 * matrix2._44;

			result._41 = matrix1._41 * matrix2._11 + matrix1._42 * matrix2._21 + matrix1._43 * matrix2._31 + matrix1._44 * matrix2._41;
			result._42 = matrix1._41 * matrix2._12 + matrix1._42 * matrix2._22 + matrix1._43 * matrix2._32 + matrix1._44 * matrix2._42;
			result._43 = matrix1._41 * matrix2._13 + matrix1._42 * matrix2._23 + matrix1._43 * matrix2._33 + matrix1._44 * matrix2._43;
			result._44 = matrix1._41 * matrix2._14 + matrix1._42 * matrix2._24 + matrix1._43 * matrix2._34 + matrix1._44 * matrix2._44;

			return result;
		}

		inline D3DMATRIX create_rotation_matrix( const uemath::frotator& rotation )
		{
			auto rad_x = rotation.pitch * float( std::numbers::pi ) / 180.f;
			auto rad_y = rotation.yaw * float( std::numbers::pi ) / 180.f;
			auto rad_z = rotation.roll * float( std::numbers::pi ) / 180.f;

			auto sp = sinf( rad_x ) , cp = cosf( rad_x );
			auto sy = sinf( rad_y ) , cy = cosf( rad_y );
			auto sr = sinf( rad_z ) , cr = cosf( rad_z );

			D3DMATRIX matrix {};
			matrix.m [ 0 ][ 0 ] = cp * cy;
			matrix.m [ 0 ][ 1 ] = cp * sy;
			matrix.m [ 0 ][ 2 ] = sp;
			matrix.m [ 0 ][ 3 ] = 0.f;

			matrix.m [ 1 ][ 0 ] = sr * sp * cy - cr * sy;
			matrix.m [ 1 ][ 1 ] = sr * sp * sy + cr * cy;
			matrix.m [ 1 ][ 2 ] = -sr * cp;
			matrix.m [ 1 ][ 3 ] = 0.f;

			matrix.m [ 2 ][ 0 ] = -( cr * sp * cy + sr * sy );
			matrix.m [ 2 ][ 1 ] = cy * sr - cr * sp * sy;
			matrix.m [ 2 ][ 2 ] = cr * cp;
			matrix.m [ 2 ][ 3 ] = 0.f;

			matrix.m [ 3 ][ 0 ] = matrix.m [ 3 ][ 1 ] = matrix.m [ 3 ][ 2 ] = 0.0f;
			matrix.m [ 3 ][ 3 ] = 1.0f;

			return matrix;
		}

		struct alignas( 16 ) matrix_elements {
			double m11 , m12 , m13 , m14;
			double m21 , m22 , m23 , m24;
			double m31 , m32 , m33 , m34;
			double m41 , m42 , m43 , m44;

			matrix_elements( ) : m11( 0 ) , m12( 0 ) , m13( 0 ) , m14( 0 ) ,
				m21( 0 ) , m22( 0 ) , m23( 0 ) , m24( 0 ) ,
				m31( 0 ) , m32( 0 ) , m33( 0 ) , m34( 0 ) ,
				m41( 0 ) , m42( 0 ) , m43( 0 ) , m44( 0 ) {}
		};

		struct alignas( 16 ) dbl_matrix {
			union {
				matrix_elements elements;
				double m [ 4 ][ 4 ];
			};

			dbl_matrix( ) : elements( ) {}

			double& operator()( size_t row , size_t col ) { return m [ row ][ col ]; }
			const double& operator()( size_t row , size_t col ) const { return m [ row ][ col ]; }
		};

		struct alignas( 16 ) fmatrix : public dbl_matrix {
			uemath::fplane x_plane;
			uemath::fplane y_plane;
			uemath::fplane z_plane;
			uemath::fplane w_plane;

			fmatrix( ) : dbl_matrix( ) , x_plane( ) , y_plane( ) , z_plane( ) , w_plane( ) {}
		};

		template< class t >
		class tarray
		{
		public:
			tarray( ) : data( ) , count( ) , max_count( ) {}
			tarray( t* data , uint32_t count , uint32_t max_count ) :data( data ) , count( count ) , max_count( max_count ) {}

			t get( uintptr_t idx ) const
			{
				return communcations::read< t >( reinterpret_cast< uintptr_t >( data ) + ( idx * sizeof( t ) ) );
			}

			std::vector<t> get_all( ) const
			{
				if ( count <= 0 || count > max_count )
				{
					return {};
				}

				try
				{
					std::vector<t> buffer( count );

					communcations::read_memory( reinterpret_cast< PVOID >( data ) , buffer.data( ) , sizeof( t ) * count );

					return buffer;
				}
				catch ( const std::bad_alloc& )
				{
					return {};
				}
			}

			uintptr_t get_address( ) const
			{
				return reinterpret_cast< uintptr_t >( data );
			}

			uint32_t get_count( ) const
			{
				return count;
			};

			uint32_t get_max_count( ) const
			{
				return max_count;
			};

			bool is_valid( ) const
			{
				return data != nullptr;
			};

			t& operator[]( int i )
			{
				return this->data [ i ];
			};

			t* data;
			uint32_t count;
			uint32_t max_count;
		};
		struct fvector4
		{
			float X;
			float Y;
			float Z;
			float W;

			fvector4( ) : X( 0.0f ), Y( 0.0f ), Z( 0.0f ), W( 0.0f ) {}
			fvector4( float x, float y, float z, float w ) : X( x ), Y( y ), Z( z ), W( w ) {}
		};

		struct flinear_color
		{
			float R;
			float G;
			float B;
			float A;

			flinear_color( ) : R( 0.0f ) , G( 0.0f ) , B( 0.0f ) , A( 0.0f ) {}
			flinear_color( float r , float g , float b , float a ) : R( r ) , G( g ) , B( b ) , A( a ) {}

			// Helper to convert from 0-255 to 0-1 range
			static flinear_color FromRGB( uint8_t r , uint8_t g , uint8_t b , uint8_t a = 255 )
			{
				return flinear_color( r / 255.0f , g / 255.0f , b / 255.0f , a / 255.0f );
			}
		};
		class ftext {
		public:
			uint64_t input;
			ftext( ) : input( 0 ) {}
			ftext( uint64_t addr ) : input( addr ) {}
			uint64_t data( ) {
				return communcations::read<uint64_t>( input + 0x20 );
			}

			int32_t length( ) {
				return  communcations::read<int32_t>( input + 0x28 );
			}

			std::string get( ) {
				if ( !input )
					return "";
				auto src = data( );
				if ( !src )
					return "";
				int size = length( );
				if ( size <= 0 || size >= 100 )
					return "";
				std::unique_ptr<wchar_t []> dst( new wchar_t [ size ] );
				communcations::read_memory( ( PVOID ) src , dst.get( ) , size * sizeof( wchar_t ) );
				std::wstring wstr( dst.get( ) , dst.get( ) + size );
				std::string str( wstr.begin( ) , wstr.end( ) );
				str.erase( str.begin( ) , std::find_if( str.begin( ) , str.end( ) , [ ]( unsigned char ch ) { return !std::isspace( ch ); } ) );
				str.erase( std::find_if( str.rbegin( ) , str.rend( ) , [ ]( unsigned char ch ) { return !std::isspace( ch ) && ch != '\0'; } ).base( ) , str.end( ) );

				return str;
			}
		};


	}
}