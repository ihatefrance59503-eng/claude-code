// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "../../imgui.h"
#include "../../thirdparty/include/nlohmann/json.hpp"
#include "../../thirdparty/include/stb_image.h"
#include <d3d11.h>
#include <unordered_map>

/**
 * @class GlbModel
 * @brief Handles parsing, loading, and rendering of .glb 3D model files
 *
 * Features:
 * - Binary .glb file parsing with validation
 * - 3D vertex and face data storage
 * - Real-time 3D-to-2D projection with perspective
 * - Interactive rotation (X and Y axes)
 * - Zoom/scale control
 * - Auto-rotation mode
 * - Depth-sorted face rendering (painter's algorithm)
 * - Basic lighting based on face normals
 */
class GlbModel {
public:
	struct Vertex {
		float x , y , z;           // Position
		float nx , ny , nz;        // Normal (for lighting)
		float u , v;              // Texture coordinates (reserved)
	};

	struct Face {
		uint32_t v1 , v2 , v3;     // Vertex indices
		ImU32 color;               // Base Face Color
		int texture_idx = -1;      // Texture Index Map (-1 if None)
		int prim_index = 0;        // Model primitive order layer
	};

	struct RawImage {
		int w = 0 , h = 0;
		std::vector<unsigned char> pixels;
	};

	struct TextureCache {
		RawImage raw;
		ID3D11ShaderResourceView* srv = nullptr;
	};

	struct RenderState {
		float rotation_x = 0.0f;
		float rotation_y = 0.0f;
		float scale = 50.0f;
		bool auto_rotate = true;
	};

private:
	std::vector<Vertex> vertices;
	std::vector<Face> faces;
	std::unordered_map<int , TextureCache> textures;
	std::string name;
	std::string last_error;

	/**
	 * @brief Calculates the cross product of two vectors
	 */
	static ImVec2 cross2d( ImVec2 a , ImVec2 b ) {
		return ImVec2( a.x * b.y - a.y * b.x , a.x * b.y - a.y * b.x );
	}

	/**
	 * @brief Checks if a point is inside a triangle (barycentric coordinates)
	 */
	static bool pointInTriangle( ImVec2 p , ImVec2 a , ImVec2 b , ImVec2 c ) {
		ImVec2 v0 = ImVec2( c.x - a.x , c.y - a.y );
		ImVec2 v1 = ImVec2( b.x - a.x , b.y - a.y );
		ImVec2 v2 = ImVec2( p.x - a.x , p.y - a.y );

		float dot00 = v0.x * v0.x + v0.y * v0.y;
		float dot01 = v0.x * v1.x + v0.y * v1.y;
		float dot02 = v0.x * v2.x + v0.y * v2.y;
		float dot11 = v1.x * v1.x + v1.y * v1.y;
		float dot12 = v1.x * v2.x + v1.y * v2.y;

		float invDenom = 1.0f / ( dot00 * dot11 - dot01 * dot01 );
		float u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;
		float v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;

		return ( u >= 0.0f ) && ( v >= 0.0f ) && ( u + v <= 1.0f );
	}

	/**
	 * @brief Applies 3D rotation matrices (X and Y axis)
	 */
	static void rotateVertex( float& x , float& y , float& z , float rot_x , float rot_y )
	{
		float cos_x = cosf( rot_x );
		float sin_x = sinf( rot_x );

		float y1 = y * cos_x - z * sin_x;
		float z1 = y * sin_x + z * cos_x;

		float cos_y = cosf( rot_y );
		float sin_y = sinf( rot_y );

		float x1 = x * cos_y + z1 * sin_y;
		float z2 = -x * sin_y + z1 * cos_y;

		x = x1;
		y = y1;
		z = z2;
	}

	/**
	 * @brief Applies perspective projection (simple depth-based scaling)
	 */
	static ImVec2 projectVertex( float x , float y , float z , ImVec2 canvas_center , float scale ) {
		// Use a large constant focal length so Z depth limits are much safer
		float focal = 2000.0f;
		float depth = focal + z;
		if ( depth < 0.1f ) depth = 0.1f; // Prevent objects from going behind the theoretical camera

		float perspective = focal / depth;
		return ImVec2(
			canvas_center.x + ( x * scale * perspective ) ,
			canvas_center.y - ( y * scale * perspective ) // Invert Y (ImGui +Y is down, GLB +Y is up)
		);
	}

public:
	/**
	 * @brief Loads a .glb model file from disk
	 *
	 * GLB Format:
	 * - Header: "GLB\0" (4 bytes)
	 * - Vertex Count: uint32_t (4 bytes)
	 * - Face Count: uint32_t (4 bytes)
	 * - Vertices: Vertex[] (32 bytes each)
	 * - Faces: Face[] (12 bytes each)
	 *
	 * @param filepath Path to the .glb file
	 * @return true if loaded successfully, false otherwise
	 */
	bool loadFromGlb( const std::string& filepath ) {
		last_error = "";
		std::ifstream file( filepath , std::ios::binary | std::ios::ate );
		if ( !file.is_open( ) ) {
			last_error = "Could not open file: " + filepath;
			return false;
		}

		std::streamsize file_size = file.tellg( );
		file.seekg( 0 , std::ios::beg );

		// Read 4 byte header
		char header [ 4 ] = { 0 };
		file.read( header , 4 );

		if ( std::string( header , 4 ) == "GLB\0" || std::string( header , 4 ) == "GLD\0" ) {
			// Read vertex and face counts
			uint32_t vertex_count = 0 , face_count = 0;
			file.read( reinterpret_cast< char* >( &vertex_count ) , sizeof( uint32_t ) );
			file.read( reinterpret_cast< char* >( &face_count ) , sizeof( uint32_t ) );

			if ( vertex_count == 0 || face_count == 0 ) {
				file.close( );
				last_error = "Invalid vertex/face count (V: " + std::to_string( vertex_count ) + ", F: " + std::to_string( face_count ) + ")";
				return false;
			}

			// Read vertices
			vertices.resize( vertex_count );
			for ( uint32_t i = 0; i < vertex_count; ++i ) {
				file.read( reinterpret_cast< char* >( &vertices [ i ] ) , sizeof( Vertex ) );
			}

			// Read faces
			faces.resize( face_count );
			for ( uint32_t i = 0; i < face_count; ++i ) {
				file.read( reinterpret_cast< char* >( &faces [ i ] ) , 12 );
				faces [ i ].color = ImGui::GetColorU32( ImVec4( 0.8f , 0.8f , 0.8f , 1.0f ) );
			}

			file.close( );
			name = filepath;
			return true;
		}
		else if ( std::string( header , 4 ) == "glTF" ) {
			// This is a standard glTF 2.0 binary file
			uint32_t version , length;
			file.read( reinterpret_cast< char* >( &version ) , 4 );
			file.read( reinterpret_cast< char* >( &length ) , 4 );

			if ( version != 2 ) {
				last_error = "Only glTF 2.0 is supported";
				return false;
			}

			// Read Chunk 0 (JSON)
			uint32_t json_chunk_len , json_chunk_type;
			file.read( reinterpret_cast< char* >( &json_chunk_len ) , 4 );
			file.read( reinterpret_cast< char* >( &json_chunk_type ) , 4 );

			if ( json_chunk_type != 0x4E4F534A ) {
				last_error = "First chunk is not JSON";
				return false;
			}

			std::vector<char> json_data( json_chunk_len + 1 , 0 );
			file.read( json_data.data( ) , json_chunk_len );

			// Read Chunk 1 (BIN)
			uint32_t bin_chunk_len = 0 , bin_chunk_type = 0;
			if ( file.tellg( ) < file_size ) {
				file.read( reinterpret_cast< char* >( &bin_chunk_len ) , 4 );
				file.read( reinterpret_cast< char* >( &bin_chunk_type ) , 4 );
				if ( bin_chunk_type != 0x004E4942 ) {
					last_error = "Second chunk is not BIN";
					return false;
				}
			}
			else {
				last_error = "No BIN chunk found";
				return false;
			}

			std::vector<uint8_t> bin_data( bin_chunk_len );
			file.read( reinterpret_cast< char* >( bin_data.data( ) ) , bin_chunk_len );
			file.close( );

			nlohmann::json j;
			try {
				j = nlohmann::json::parse( json_data.data( ) );
			}
			catch ( const std::exception& e ) {
				last_error = std::string( "JSON parse error: " ) + e.what( );
				return false;
			}

			if ( !j.contains( "meshes" ) || j [ "meshes" ].empty( ) || !j [ "meshes" ][ 0 ].contains( "primitives" ) || j [ "meshes" ][ 0 ][ "primitives" ].empty( ) ) {
				last_error = "GLB contains no meshes/primitives";
				return false;
			}

			// Process base colors per mesh primitive
			for ( size_t m = 0; m < j [ "meshes" ].size( ); ++m ) {
				auto& ms = j [ "meshes" ][ m ];
				if ( !ms.contains( "primitives" ) ) continue;

				for ( size_t p = 0; p < ms [ "primitives" ].size( ); ++p ) {
					auto& primitive = ms [ "primitives" ][ p ];

					int pos_acc_idx = -1;
					int ind_acc_idx = -1;
					int tex_acc_idx = -1;

					if ( primitive.contains( "attributes" ) ) {
						if ( primitive [ "attributes" ].contains( "POSITION" ) ) {
							pos_acc_idx = primitive [ "attributes" ][ "POSITION" ].get<int>( );
						}
						if ( primitive [ "attributes" ].contains( "TEXCOORD_0" ) ) {
							tex_acc_idx = primitive [ "attributes" ][ "TEXCOORD_0" ].get<int>( );
						}
					}

					if ( primitive.contains( "indices" ) ) {
						ind_acc_idx = primitive [ "indices" ].get<int>( );
					}

					if ( pos_acc_idx == -1 ) {
						continue; // skip primitive without position
					}

					ImU32 base_color = ImGui::GetColorU32( ImVec4( 0.8f , 0.8f , 0.8f , 1.0f ) ); // default clay
					int material_idx = primitive.value( "material" , -1 );
					int prim_texture_idx = -1;

					if ( material_idx != -1 && j.contains( "materials" ) ) {
						auto& mat = j [ "materials" ][ material_idx ];

						if ( mat.contains( "pbrMetallicRoughness" ) ) {
							auto& pbr = mat [ "pbrMetallicRoughness" ];

							// Load BaseColorFactor
							if ( pbr.contains( "baseColorFactor" ) ) {
								auto& baseColorFactor = pbr [ "baseColorFactor" ];
								float r = baseColorFactor [ 0 ].get<float>( );
								float g = baseColorFactor [ 1 ].get<float>( );
								float b = baseColorFactor [ 2 ].get<float>( );
								// Force baseline opacity (1.0f) completely overriding any translucent GLB parameters
								base_color = ImGui::GetColorU32( ImVec4( r , g , b , 1.0f ) );
							}

							// Load BaseColorTexture Image Chunks
							if ( pbr.contains( "baseColorTexture" ) ) {
								int tex_idx = pbr [ "baseColorTexture" ][ "index" ].get<int>( );
								if ( j.contains( "textures" ) && j [ "textures" ].size( ) > tex_idx ) {
									int source_idx = j [ "textures" ][ tex_idx ][ "source" ].get<int>( );
									if ( j.contains( "images" ) && j [ "images" ].size( ) > source_idx ) {
										auto& image = j [ "images" ][ source_idx ];
										if ( image.contains( "bufferView" ) ) {
											int bv_idx = image [ "bufferView" ].get<int>( );
											auto& bv = j [ "bufferViews" ][ bv_idx ];
											int bvOffset = bv.value( "byteOffset" , 0 );
											int bvLen = bv.value( "byteLength" , 0 );

											// Extract and Decompress via stb_image
											if ( textures.find( source_idx ) == textures.end( ) ) {
												int w , h , comp;
												unsigned char* img_data = stbi_load_from_memory(
													bin_data.data( ) + bvOffset , bvLen ,
													&w , &h , &comp , 4
												);
												if ( img_data ) {
													// Force all texture pixels to be completely opaque (255 Alpha) 
													// to guarantee absolutely no alpha blending or transparency artifacts!
													for ( int p = 0; p < w * h * 4; p += 4 ) {
														img_data [ p + 3 ] = 255;
													}
													textures [ source_idx ].raw.w = w;
													textures [ source_idx ].raw.h = h;
													textures [ source_idx ].raw.pixels.assign( img_data , img_data + ( w * h * 4 ) );
													stbi_image_free( img_data );
												}
											}
											prim_texture_idx = source_idx;
										}
									}
								}
							}
						}
					}

					auto readAccessorData = [ & ]( int acc_idx , int elements_per_item , auto callback ) {
						auto& acc = j [ "accessors" ][ acc_idx ];
						int count = acc [ "count" ].get<int>( );
						int type = acc [ "componentType" ].get<int>( );
						int accOffset = acc.value( "byteOffset" , 0 );
						int bufferViewIdx = acc [ "bufferView" ].get<int>( );
						auto& bv = j [ "bufferViews" ][ bufferViewIdx ];
						int bvOffset = bv.value( "byteOffset" , 0 );
						int stride = bv.value( "byteStride" , 0 );

						int element_size = ( type == 5126 ) ? 4 : ( ( type == 5123 ) ? 2 : ( ( type == 5125 ) ? 4 : 1 ) );
						int item_size = element_size * elements_per_item;
						if ( stride == 0 ) stride = item_size;

						for ( int i = 0; i < count; ++i ) {
							uint8_t* ptr = bin_data.data( ) + bvOffset + accOffset + i * stride;
							callback( i , ptr , type );
						}
						};

					int pos_count = j [ "accessors" ][ pos_acc_idx ][ "count" ].get<int>( );
					int vertex_offset = vertices.size( );
					vertices.resize( vertex_offset + pos_count );
					readAccessorData( pos_acc_idx , 3 , [ & ]( int i , uint8_t* ptr , int type ) {
						float* f = reinterpret_cast< float* >( ptr );
						vertices [ vertex_offset + i ] = { f [ 0 ] , f [ 1 ] , f [ 2 ] , 0 , 0 , 0 , 0 , 0 };
						} );

					if ( tex_acc_idx != -1 ) {
						readAccessorData( tex_acc_idx , 2 , [ & ]( int i , uint8_t* ptr , int type ) {
							if ( type == 5126 ) { // FLOAT
								float* f = reinterpret_cast< float* >( ptr );
								vertices [ vertex_offset + i ].u = f [ 0 ];
								vertices [ vertex_offset + i ].v = f [ 1 ];
							}
							else if ( type == 5123 ) { // UNSIGNED_SHORT
								uint16_t* u = reinterpret_cast< uint16_t* >( ptr );
								vertices [ vertex_offset + i ].u = u [ 0 ] / 65535.0f;
								vertices [ vertex_offset + i ].v = u [ 1 ] / 65535.0f;
							}
							else if ( type == 5121 ) { // UNSIGNED_BYTE
								uint8_t* b = reinterpret_cast< uint8_t* >( ptr );
								vertices [ vertex_offset + i ].u = b [ 0 ] / 255.0f;
								vertices [ vertex_offset + i ].v = b [ 1 ] / 255.0f;
							}
							} );
					}

					int true_prim_layer = ( int ) ( m * 100 + p );

					if ( ind_acc_idx != -1 ) {
						readAccessorData( ind_acc_idx , 1 , [ & ]( int i , uint8_t* ptr , int type ) {
							uint32_t val = 0;
							if ( type == 5123 ) val = *reinterpret_cast< uint16_t* >( ptr );
							else if ( type == 5125 ) val = *reinterpret_cast< uint32_t* >( ptr );
							else if ( type == 5121 ) val = *reinterpret_cast< uint8_t* >( ptr );
							else val = *reinterpret_cast< uint32_t* >( ptr );

							if ( i % 3 == 0 ) faces.push_back( { 0, 0, 0, base_color, prim_texture_idx, true_prim_layer } );
							if ( i % 3 == 0 ) faces.back( ).v1 = vertex_offset + val;
							else if ( i % 3 == 1 ) faces.back( ).v2 = vertex_offset + val;
							else if ( i % 3 == 2 ) faces.back( ).v3 = vertex_offset + val;
							} );
					}
					else {
						for ( int i = 0; i < pos_count; i += 3 ) {
							faces.push_back( {
								( uint32_t ) ( vertex_offset + i ) ,
								( uint32_t ) ( vertex_offset + i + 1 ) ,
								( uint32_t ) ( vertex_offset + i + 2 ),
								base_color,
								prim_texture_idx,
								true_prim_layer
								} );
						}
					}
				}
			}

			if ( faces.empty( ) ) {
				last_error = "No valid faces found across all meshes/primitives.";
				return false;
			}

			// Generate simple face normals for lighting if not present
			for ( auto& f : faces ) {
				// simple cross product normal
				ImVec2 p1( vertices [ f.v1 ].x , vertices [ f.v1 ].y );
				ImVec2 p2( vertices [ f.v2 ].x , vertices [ f.v2 ].y );
				ImVec2 p3( vertices [ f.v3 ].x , vertices [ f.v3 ].y );
				// ImGui painter's render will do lighting simply using projected vertex Z values
			}

			if ( !vertices.empty( ) ) {
				float min_x = vertices [ 0 ].x , max_x = vertices [ 0 ].x;
				float min_y = vertices [ 0 ].y , max_y = vertices [ 0 ].y;
				float min_z = vertices [ 0 ].z , max_z = vertices [ 0 ].z;

				for ( const auto& v : vertices ) {
					if ( v.x < min_x ) min_x = v.x;
					if ( v.x > max_x ) max_x = v.x;
					if ( v.y < min_y ) min_y = v.y;
					if ( v.y > max_y ) max_y = v.y;
					if ( v.z < min_z ) min_z = v.z;
					if ( v.z > max_z ) max_z = v.z;
				}

				float center_x = ( min_x + max_x ) / 2.0f;
				float center_y = ( min_y + max_y ) / 2.0f;
				float center_z = ( min_z + max_z ) / 2.0f;

				for ( auto& v : vertices ) {
					// 1. Shift model to perfect origin bounds
					v.x -= center_x;
					v.y -= center_y;
					v.z -= center_z;

					// 2. Map glTF standards (-Z forward, -X right) to Screen Left-Handed bounds (+Z in, +X right)
					v.x = -v.x;
					v.z = -v.z;
				}
			}

			name = filepath;
			return true;
		}

		// Setup debug header text
		char debug_buf [ 128 ];
		sprintf_s( debug_buf , "Bytes: %02X %02X %02X %02X ('%c%c%c%c')" ,
			( unsigned char ) header [ 0 ] , ( unsigned char ) header [ 1 ] ,
			( unsigned char ) header [ 2 ] , ( unsigned char ) header [ 3 ] ,
			header [ 0 ] >= 32 && header [ 0 ] <= 126 ? header [ 0 ] : '.' ,
			header [ 1 ] >= 32 && header [ 1 ] <= 126 ? header [ 1 ] : '.' ,
			header [ 2 ] >= 32 && header [ 2 ] <= 126 ? header [ 2 ] : '.' ,
			header [ 3 ] >= 32 && header [ 3 ] <= 126 ? header [ 3 ] : '.' );

		file.close( );
		last_error = "Invalid header. Got " + std::string( debug_buf );
		return false;
	}

	/**
	 * @brief Renders the 3D model to a 2D canvas
	 *
	 * @param draw_list ImGui draw list for rendering
	 * @param canvas_pos Top-left position of the rendering area
	 * @param canvas_size Size of the rendering area
	 * @param rot_x Rotation around X axis (radians)
	 * @param rot_y Rotation around Y axis (radians)
	 * @param scale Scale factor for the model
	 */
	static ImU32 sampleTexture( const RawImage& img , float u , float v )
	{
		if ( img.pixels.empty( ) || img.w == 0 || img.h == 0 )
			return IM_COL32_WHITE;

		// 🔥 FIX: proper wrapping
		u = u - floorf( u );
		v = v - floorf( v );

		// 🔥 FIX: FLIP V (THIS is why your textures are wrong)
		v = 1.0f - v;

		int x = ( int ) ( u * ( img.w - 1 ) );
		int y = ( int ) ( v * ( img.h - 1 ) );

		// 🔥 Clamp to prevent crashes
		x = std::clamp( x , 0 , img.w - 1 );
		y = std::clamp( y , 0 , img.h - 1 );

		int idx = ( y * img.w + x ) * 4;

		return IM_COL32(
			img.pixels [ idx + 0 ] ,
			img.pixels [ idx + 1 ] ,
			img.pixels [ idx + 2 ] ,
			img.pixels [ idx + 3 ]
		);
	}
	struct ZPixel {
		float depth;
		ImU32 color;
	};
	void render( ImDrawList* draw_list , ImVec2 canvas_pos , ImVec2 canvas_size ,
		float rot_x , float rot_y , float scale , ID3D11Device* device = nullptr )
	{
		if ( vertices.empty( ) || faces.empty( ) || !draw_list )
			return;

		int width = ( int ) canvas_size.x;
		int height = ( int ) canvas_size.y;

		if ( width <= 0 || height <= 0 )
			return;

		std::vector<float> zbuffer( width * height , 1e9f );
		std::vector<ImU32> framebuffer( width * height , 0 );

		ImVec2 center(
			canvas_pos.x + canvas_size.x * 0.5f ,
			canvas_pos.y + canvas_size.y * 0.5f
		);

		std::vector<Vertex> rv( vertices.size( ) );
		std::vector<ImVec2> pv( vertices.size( ) );
		std::vector<float> inv_w( vertices.size( ) ); // 🔥 FIXED

		const float focal = 2000.0f;

		// --- Transform ---
		for ( size_t i = 0; i < vertices.size( ); ++i ) {
			float x = vertices [ i ].x;
			float y = vertices [ i ].y;
			float z = vertices [ i ].z;

			rotateVertex( x , y , z , rot_x , rot_y );

			float depth = focal + z;
			if ( depth < 0.1f ) depth = 0.1f;

			inv_w [ i ] = 1.0f / depth;

			rv [ i ] = { x, y, z };
			pv [ i ] = projectVertex( x , y , z , center , scale );
		}

		// --- Raster ---
		for ( size_t fi = 0; fi < faces.size( ); ++fi ) {
			const Face& f = faces [ fi ];

			const Vertex& v1 = rv [ f.v1 ];
			const Vertex& v2 = rv [ f.v2 ];
			const Vertex& v3 = rv [ f.v3 ];

			ImVec2 p1 = pv [ f.v1 ];
			ImVec2 p2 = pv [ f.v2 ];
			ImVec2 p3 = pv [ f.v3 ];

			// --- Normal ---
			float dx1 = v2.x - v1.x;
			float dy1 = v2.y - v1.y;
			float dz1 = v2.z - v1.z;

			float dx2 = v3.x - v1.x;
			float dy2 = v3.y - v1.y;
			float dz2 = v3.z - v1.z;

			float nx = dy1 * dz2 - dz1 * dy2;
			float ny = dz1 * dx2 - dx1 * dz2;
			float nz = dx1 * dy2 - dy1 * dx2;

			float len = sqrtf( nx * nx + ny * ny + nz * nz );
			if ( len == 0.0f ) continue;

			nx /= len; ny /= len; nz /= len;

			// Backface culling
			if ( nz >= 0.0f )
				continue;

			float brightness = 0.4f + 0.6f * ( -nz );
			brightness = ImClamp( brightness , 0.0f , 1.0f );

			ImVec4 base_col = ImGui::ColorConvertU32ToFloat4( f.color );

			// --- Bounding box ---
			float minxf = min( p1.x , min( p2.x , p3.x ) );
			float maxxf = max( p1.x , max( p2.x , p3.x ) );
			float minyf = min( p1.y , min( p2.y , p3.y ) );
			float maxyf = max( p1.y , max( p2.y , p3.y ) );

			int minX = ( int ) max( 0.0f , floorf( minxf - canvas_pos.x ) );
			int maxX = ( int ) min( ( float ) width - 1 , ceilf( maxxf - canvas_pos.x ) );
			int minY = ( int ) max( 0.0f , floorf( minyf - canvas_pos.y ) );
			int maxY = ( int ) min( ( float ) height - 1 , ceilf( maxyf - canvas_pos.y ) );

			float denom = ( p2.y - p3.y ) * ( p1.x - p3.x ) + ( p3.x - p2.x ) * ( p1.y - p3.y );
			if ( fabsf( denom ) < 1e-6f )
				continue;

			// --- Raster loop ---
			for ( int py = minY; py <= maxY; ++py ) {
				for ( int px = minX; px <= maxX; ++px ) {

					float sx = canvas_pos.x + px;
					float sy = canvas_pos.y + py;

					float w1 = ( ( p2.y - p3.y ) * ( sx - p3.x ) + ( p3.x - p2.x ) * ( sy - p3.y ) ) / denom;
					float w2 = ( ( p3.y - p1.y ) * ( sx - p3.x ) + ( p1.x - p3.x ) * ( sy - p3.y ) ) / denom;
					float w3 = 1.0f - w1 - w2;

					if ( w1 < 0.0f || w2 < 0.0f || w3 < 0.0f )
						continue;

					float depth =
						w1 * v1.z +
						w2 * v2.z +
						w3 * v3.z;

					int idx = py * width + px;

					if ( depth < zbuffer [ idx ] ) {

						zbuffer [ idx ] = depth;

						ImU32 out_col;

						// --- TEXTURE (FIXED) ---
						if ( f.texture_idx != -1 )
						{
							auto it = textures.find( f.texture_idx );
							if ( it != textures.end( ) )
							{
								const RawImage& tex = it->second.raw;

								float w1p = inv_w [ f.v1 ];
								float w2p = inv_w [ f.v2 ];
								float w3p = inv_w [ f.v3 ];

								float denom_w = ( w1 * w1p + w2 * w2p + w3 * w3p );

								// Perspective-correct UV interpolation
								float u =
									( w1 * vertices [ f.v1 ].u * w1p +
										w2 * vertices [ f.v2 ].u * w2p +
										w3 * vertices [ f.v3 ].u * w3p ) / denom_w;

								float v =
									1.0f - (
										( w1 * vertices [ f.v1 ].v * w1p +
											w2 * vertices [ f.v2 ].v * w2p +
											w3 * vertices [ f.v3 ].v * w3p ) / denom_w
										);

								ImU32 tex_col = sampleTexture( tex , u , v );

								ImVec4 tcol = ImGui::ColorConvertU32ToFloat4( tex_col );

								// Lighting
								tcol.x *= brightness;
								tcol.y *= brightness;
								tcol.z *= brightness;

								out_col = ImGui::ColorConvertFloat4ToU32( tcol );
							}
							else
							{
								base_col.x *= brightness;
								base_col.y *= brightness;
								base_col.z *= brightness;

								out_col = ImGui::ColorConvertFloat4ToU32( base_col );
							}
						}
						else {
							base_col.x *= brightness;
							base_col.y *= brightness;
							base_col.z *= brightness;
							out_col = ImGui::ColorConvertFloat4ToU32( base_col );
						}

						framebuffer [ idx ] = out_col;
					}
				}
			}
		}

		// --- DRAW ---
		for ( int y = 0; y < height; ++y ) {
			for ( int x = 0; x < width; ++x ) {
				ImU32 col = framebuffer [ y * width + x ];
				if ( col != 0 ) {
					draw_list->AddRectFilled(
						ImVec2( canvas_pos.x + x , canvas_pos.y + y ) ,
						ImVec2( canvas_pos.x + x + 1 , canvas_pos.y + y + 1 ) ,
						col
					);
				}
			}
		}
	}

	/**
	 * @brief Gets the name/path of the currently loaded model
	 */
	const std::string& getModelName( ) const {
		return name;
	}

	/**
	 * @brief Checks if a model is currently loaded
	 */
	bool isLoaded( ) const {
		return !vertices.empty( ) && !faces.empty( );
	}

	const std::string& getLastError( ) const {
		return last_error;
	}

	/**
	 * @brief Gets vertex count
	 */
	size_t getVertexCount( ) const {
		return vertices.size( );
	}

	/**
	 * @brief Gets face count
	 */
	size_t getFaceCount( ) const {
		return faces.size( );
	}

	/**
	 * @brief Clears the model data and safely drops Shader Resource Views
	 */
	void clear( ) {
		vertices.clear( );
		faces.clear( );
		name.clear( );
		for ( auto& pair : textures ) {
			if ( pair.second.srv ) {
				pair.second.srv->Release( );
				pair.second.srv = nullptr;
			}
		}
		textures.clear( );
	}

	~GlbModel( ) {
		clear( );
	}
};