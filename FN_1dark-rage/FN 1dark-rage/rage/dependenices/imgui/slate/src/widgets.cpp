// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#define NOMINMAX
#include "../include/slate.h"
#include "../../imgui_freetype.h"
#include <thread>
#pragma comment(lib, "winmm.lib")

using namespace ImGui;

vec2::operator ImVec2( ) const {
	return ImVec2{ x, y } ds;
}

extern c_image logo;
void c_font::setup( unsigned char* data, size_t data_size, std::vector< float > sizes, const ImWchar* ranges ) {
	auto config = ImFontConfig( );
	config.FontDataOwnedByAtlas = false;

	for ( auto& sz : sizes ) {
		fonts.push_back( { sz, ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF( data, data_size, sz * g_style->dpi_scale, &config, ranges ) } );
	}
}

bool c_widgets::button( const ptext& label, ImVec2 size, c_buttonstyle style ) 
{
    struct s {
        float hover;
        float held;
    }; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
    
    auto* window = GetCurrentWindow( );
    bool pressed = InvisibleButton( label.str.data( ), CalcItemSize( size, GImGui->Style.FramePadding.x * 2 + CalcTextSize( label.translate( ).data( ), 0, 1 ).x + ( ( style.iconsize + 8 ) ds ) * bool( style.icon ), GetFrameHeight( ) ) );
    bool hovered = IsItemHovered( ), held = IsItemActive( );
    ImRect& bb = GImGui->LastItemData.Rect;
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.held = anim( obj.held, 0.f, 1.f, held );
    
    pcolor iconcolor{ 0.f, 0.f, 0.f, 0.f }, labelcolor{ 0.f, 0.f, 0.f, 0.f };
    switch ( style.style ) {
        case button_red:
        iconcolor = pcolor{ 0, 0, 0, 0 };
        labelcolor = g_style->col( pcol_bg2 );
        
        window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( g_style->col( pcol_scheme ), pcolor{ 229, 69, 69, 0.80f }, obj.hover ), pcolor{ 229, 69, 69, 0.60f }, obj.held ), 3 ds );
        break;
        case button_blue:
        iconcolor = pcolor{ 0, 0, 0, 0 };
        labelcolor = g_style->col( pcol_text );
        
        window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( pcolor{ 69, 96, 229, 1.00f }, pcolor{ 69, 96, 229, 0.80f }, obj.hover ), pcolor{ 69, 96, 229, 0.60f }, obj.held ), 3 ds );
        break;
        case button_default:
        iconcolor = pcolor{ 0, 0, 0, 0 };
        labelcolor = g_style->col( pcol_text );
        
        window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.hover ), g_style->col( pcol_bg4 ), obj.held ), 3 ds );
        break;
        
    }
    
    float spacing = style.iconsize + 8;
    ImVec2 label_pos = bb.GetCenter( ) - CalcTextSize( label.translate( ).data( ), 0, 1 ) / 2;
    if ( style.icon )
    {
        label_pos.x += ( spacing / 2 ) ds;
        g_draw->text( icons, style.iconsize, { label_pos.x - spacing ds, bb.GetCenter( ).y - ( style.iconsize ds ) / 2 }, iconcolor, style.icon );
    }
    
    window->DrawList->AddText( label_pos, labelcolor, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
    
    return pressed;
}

bool c_widgets::checkbox( const ptext& label, bool* v, c_checkboxstyle style ) 
{
    struct s {
        float anim;
        float hover;
        float enabled;
        ImVec2 optsize;
    }; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
    
    auto window = GetCurrentWindow( );
    
    float square_sz = 16 ds;
    
    bool pressed = InvisibleButton( label.str.data( ), { CalcItemWidth( ), 16 } );
    bool hovered = IsItemHovered( ), held = IsItemActive( );
    ImRect total_bb = GImGui->LastItemData.Rect;
    ImRect bb{ { total_bb.Min.x, total_bb.Max.y - 16 ds }, { total_bb.Min.x + 16 ds, total_bb.Max.y } };
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.enabled = anim( obj.enabled, 0.f, 1.f, *v );
    obj.anim = anim( obj.anim, 0.f, 1.f, hovered || *v );
    
    if ( pressed ) {
        *v = !*v;
    }
    
    auto bgcol = col_anim( col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.hover ), g_style->col( pcol_scheme ), obj.enabled );
    auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.enabled );
    
    window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.hover ), g_style->col( pcol_scheme ), obj.enabled ), 1 ds );
    window->DrawList->AddRect( bb.Min, bb.Max, g_style->col( pcol_border ), 1 ds );
    window->DrawList->AddText( total_bb.Min + vec2{ 24, 0 }, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
    RenderCheckMark( window->DrawList, bb.GetCenter( ) - vec2{ 4, 4 }, g_style->col( pcol_bg2, obj.enabled ), 8 ds );
    
    
    auto pos = window->DC.CursorPos;
    auto posprev = window->DC.CursorPosPrevLine;
    
    window->DC.CursorPos = ImVec2{ total_bb.Max.x - obj.optsize.x, bb.GetCenter( ).y - obj.optsize.y / 2 };
    char temp[128];
    ImFormatString( temp, sizeof( temp ), "%s opt", label.str.data( ) );
    PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 1, 1 } );
    BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground );
    {
        if ( style.key ) {
            ImFormatString( temp, sizeof( temp ), "##%s key", label.str.data( ) );
            widgets->binder( temp, style.key );
            SameLine( 0, 8 ds );
        }
        if ( style.col.size( ) > 0 ) {
            for ( int i = 0; i < style.col.size( ); ++i ) {
                ImFormatString( temp, sizeof( temp ), "##%s col %d", label.str.data( ), i );
                SetCursorPosY( GetWindowHeight( ) / 2 - 6 ds );
                widgets->coloredit( temp, style.col[i], vec2{ 16, 16 } );
                SameLine( 0, 8 ds );
            }
        }
        if ( style.options ) {
            ImFormatString( temp, sizeof( temp ), "##%s settings", label.str.data( ) );
            SetCursorPosY( GetWindowHeight( ) / 2 - 8 ds );
            widgets->optionsbtn( temp, style.options );
            SameLine( 0, 8 ds );
        }
        if ( style.tooltipicon ) {
            ImFormatString( temp, sizeof( temp ), "##%s tt", label.str.data( ) );
            widgets->tooltipbtn( temp, style.tooltiptext, style.tooltipicon );
            SameLine( 0, 8 ds );
        }
        
        obj.optsize = GetWindowSize( );
    }
    EndChild( );
    PopStyleVar( );
    
    window->DC.CursorPos = pos;
    window->DC.CursorPosPrevLine = posprev;
    
    return pressed;
}

template < typename T >
bool c_widgets::slider( const ptext& label, T* v, T min, T max, const char* format, c_sliderstyle style ) 
{
    struct s {
        float anim;
        float hover;
        float held;
        float val_anim;
        bool washeld = false;
    }; auto& obj = anim_obj( label.str.data( ), 1120, s{ } );
    
    char max_buf[32];
    ImFormatString( max_buf, sizeof( max_buf ), format, max );
    
    auto window = GetCurrentWindow( );
    auto id = window->GetID( label.str.data( ) );
    ImRect total_bb{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcItemWidth(), 26 ds } };
    ImRect bb{ { total_bb.Min.x, total_bb.Max.y - 6 ds }, { total_bb.Max.x, total_bb.Max.y } };
    ItemSize( total_bb );
    ItemAdd( total_bb, id );
    
    bool result = false;
    
    auto data = GImGui->LastItemData;
    
    bool hovered, held;
    bool pressed = ButtonBehavior( total_bb, id, &hovered, &held );
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.held = anim( obj.held, 0.f, 1.f, held );
    obj.anim = anim( obj.anim, 0.f, 1.f, hovered || held );
    obj.val_anim = ImLerp( obj.val_anim, ( ImClamp( *v, min, max ) - min * 1.f ) / ( max - min ) * bb.GetWidth( ), GetIO( ).DeltaTime * 17 );
    
    if ( held ) {
        *v = ImClamp( T( min + ( GetIO( ).MousePos.x - bb.Min.x ) / bb.GetWidth( ) * ( max - min ) ), min, max );   
        if ( !obj.washeld ) {
            obj.washeld = true;
        }
        } else { 
        if ( obj.washeld ) {
            obj.washeld = false;
            result = true;
        }
    }
    
    window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.hover ), g_style->col( pcol_bg4 ), obj.held ), 3 ds );
    window->DrawList->AddRectFilled( bb.Min, { bb.Min.x + obj.val_anim, bb.Max.y }, g_style->col( pcol_scheme ), 3 ds );
    window->DrawList->AddText( total_bb.Min, g_style->col( pcol_text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
    
    
    char buf[32];
    ImFormatString( buf, sizeof( buf ), format, *v );
    
    auto pos = window->DC.CursorPos;
    window->DC.CursorPos = ImVec2{ total_bb.Max.x - CalcTextSize( buf ).x, total_bb.Min.y };
    char temp[128];
    ImFormatString( temp, sizeof( temp ), "##%s", label );
    BeginChild( temp, CalcTextSize( buf ), 0, ImGuiWindowFlags_NoBackground );
    PushStyleColor( ImGuiCol_FrameBg, GetColorU32( ImGuiCol_FrameBg, 0 ) );
    PushStyleColor( ImGuiCol_Text, g_style->col( pcol_text2 ).vec4( ) );
    PushStyleVar( ImGuiStyleVar_FramePadding, { 0, 0 } );
    if ( InputTextEx( temp, "", buf, sizeof( buf ), CalcTextSize( buf ), ImGuiInputTextFlags_NoHorizontalScroll ) ) {
        DataTypeApplyFromText( buf, std::is_same< T, int >::value ? ImGuiDataType_S32 : ImGuiDataType_Float, v, format );
        *v = ImClamp( *v, min, max );
    }
    PopStyleVar( );
    PopStyleColor( 2 );
    EndChild( );
    window->DC.CursorPos = pos;
    
    GImGui->LastItemData = data;
    
    return result;
}

bool c_widgets::sliderint( const ptext& label, int* v, int min, int max, const char* format ) 
{
	bool result = slider( label, v, min, max, format );
	keybinds.popup( label.str, v, ht_sliderint, min, max );

	return result;
}
bool c_widgets::sliderfloat( const ptext& label, float* v, float min, float max, const char* format ) 
{
	bool result = slider( label, v, min, max, format );
	keybinds.popup( label.str, v, ht_sliderfloat, min, max );

	return result;
}

bool c_widgets::coloredit( const ptext& label, float* col, ImVec2 size )
{
	struct s {
		float anim;
		float hover;
		float open_anim;
		bool open;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	if ( size == ImVec2{ 0, 0 } ) size = vec2{ 16, 16 };

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect total_bb{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcTextSize( label.str.data( ), 0, 1 ).x > 0 ? CalcItemWidth( ) : size.x, size.y } };
	ImRect bb{ total_bb.Max - size, total_bb.Max };
	ItemSize( total_bb );
	ItemAdd( total_bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );
	bool value_changed = false;

	if ( pressed ) {
		obj.open = !obj.open;
	}

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || obj.open );

	window->DrawList->AddText( { total_bb.Min.x, total_bb.GetCenter( ).y - GImGui->FontSize / 2 }, GetColorU32( ImGuiCol_Text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, pcolor{ col[0], col[1], col[2], 1.f }, 1 );

	if ( popup.begin( label.str, obj.open_anim, bb.Min, vec2{ 14, 14 } ) ) {
		if ( ( !IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) || ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) ) || FindWindowDisplayIndex( GImGui->HoveredWindow ) < FindWindowDisplayIndex( GetCurrentWindow( ) ) ) && IsMouseClicked( 0 ) ) {
			obj.open = false;
		}

		PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 6, 6 } );
		colorpicker.draw( label.str, col );
		PopStyleVar( );

		popup.end( );
	}

	return value_changed;
}
bool c_widgets::optionsbtn( const std::string_view& str_id, std::function< void( ) > options, ImVec2 windowpos, vec2 padding )
{
	struct s {
		float anim;
		float hover;
		float open_anim;
		bool open = false;
		bool closed = false;
	}; auto& obj = anim_obj( str_id.data( ), 12210, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id.data( ) );
	ImRect bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };

	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || obj.open );

	if ( pressed && !obj.closed ) {
		obj.open = true;
	}

	if ( !IsMouseDown( 0 ) ) {
		obj.closed = false;
	}

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.open_anim );
	g_draw->rotatestart( );
	g_draw->text( icons, 16, bb.Min, pcolor{ col }, I_SETTINGS );
	g_draw->rotateend( IM_PI / 2 - IM_PI * obj.open_anim, g_draw->rotationcenter( ) );

	if ( windowpos == ImVec2{ 0, 0 } ) {
		windowpos = ImVec2{ bb.Min.x, bb.Max.y + 8 ds };
	}

	if ( popup.begin( str_id, obj.open_anim, windowpos, padding ) ) {
		if ( ( !IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) || ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) ) ) && IsMouseClicked( 0 ) ) {
			obj.open = false;
			obj.closed = true;
		}

		PushItemWidth( 180 ds );
		PushItemFlag( ImGuiItemFlags_NoNav, true );
		options( );
		PopItemFlag( );
		PopItemWidth( );
		popup.end( );
	}

	return pressed;
}
bool c_widgets::iconbutton( const char* str_id, const char* icon )
{
	struct s {
		float anim = 0;
		float hover = 0;
		float held = 0;
	}; auto& obj = anim_obj( str_id, 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.held = anim( obj.held, 0.f, 1.f, held );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || held );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text3, 0.6f ), obj.held );

	g_draw->text( icons, 16, bb.Min, col, icon );

	return pressed;
}
bool c_widgets::tooltipbtn( const std::string_view& str_id, const ptext& tooltip, const char* icon )
{
	struct s {
		float anim;
		float hover;
	}; auto& obj = anim_obj( str_id.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id.data( ) );
	ImRect bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };

	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );

	auto col = col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover );
	g_draw->text( icons, 16, bb.Min, pcolor{ col }, icon );

	if ( obj.hover > 0.05f ) {
		GetForegroundDrawList( )->AddRectFilled( { bb.Min.x, bb.Max.y + 4 ds }, ImVec2{ bb.Min.x, bb.Max.y } + vec2{ 16, 20 } + CalcTextSize( tooltip.translate( ).data( ) ), g_style->col( pcol_bg3, obj.hover ), 3 ds );
		GetForegroundDrawList( )->AddRect( { bb.Min.x, bb.Max.y + 4 ds }, ImVec2{ bb.Min.x, bb.Max.y } + vec2{ 16, 20 } + CalcTextSize( tooltip.translate( ).data( ) ), g_style->col( pcol_border, obj.hover ), 3 ds );
		GetForegroundDrawList( )->AddText( ImVec2{ bb.Min.x, bb.Max.y } + vec2{ 8, 12 }, g_style->col( pcol_text, obj.hover ), tooltip.translate( ).data( ) );
	}

	return pressed;
}

bool c_widgets::comboex( const ptext& label, const std::string_view& preview, bool should_close, c_combostyle style ) 
{
    struct s {
        float anim;
        float hover;
        float held;
        float open_anim;
        bool open;
        float w;
    }; auto& obj = anim_obj( label.str.data( ), 123123443, s{ } );
    
    bool result = false;
    
    obj.w = ImLerp( obj.w, g_draw->textsize( font, 16, preview.data( ) ).x, GetIO( ).DeltaTime * 17 );
    
    ImVec2 padding{ 8 ds, 6 ds };
    
    auto window = GetCurrentWindow( );
    auto id = window->GetID( label.str.data( ) );
    ImRect total_bb{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcItemWidth( ), 48 ds } };
    ImRect bb{ { total_bb.Min.x, total_bb.Max.y - 28 ds }, { total_bb.Max.x, total_bb.Max.y } };
    
    ItemSize( total_bb );
    ItemAdd( total_bb, id );
    
    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );
    
    if ( pressed ) {
        obj.open = !obj.open;
    }
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.held = anim( obj.held, 0.f, 1.f, held );
    obj.anim = anim( obj.anim, 0.f, 1.f, obj.open || hovered );
    obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );
    
    window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.hover ), g_style->col( pcol_bg4 ), obj.open_anim ), 3 ds );
    window->DrawList->AddRect( bb.Min, bb.Max, pcolor{ 255, 255, 255, 0.02f }, 3 ds );
    window->DrawList->AddText( total_bb.Min, g_style->col( pcol_text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
    window->DrawList->AddText( bb.Min + padding, g_style->col( pcol_text ), preview.data( ) );
    if ( style.rotate ) g_draw->rotatestart( );
    GetWindowDrawList( )->AddText( fonts[icons].get( 12 ), fonts[icons].get( 12 )->FontSize, { bb.Max.x - 12 ds - padding.x, bb.GetCenter( ).y - fonts[icons].get( 12 )->FontSize / 2 }, g_style->col( pcol_scheme ), style.icon );
    if ( style.rotate ) g_draw->rotateend( IM_PI / 2 - IM_PI * obj.open_anim, g_draw->rotationcenter( ) );
    
    
    if ( obj.open_anim > 0.05f ) {
        char temp[64];
        ImFormatString( temp, sizeof( temp ), "%s popup", label.str.data( ) );
        PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 0, 4 } );
        PushStyleVar( ImGuiStyleVar_WindowRounding, style.rounding );
        PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1 );
        PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 0, 0 } );
        PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * obj.open_anim );
        PushStyleColor( ImGuiCol_WindowBg, g_style->col( pcol_bg3 ).vec4( ) );
        Begin( temp, 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );
        SetWindowSize( { ImMax( GetCurrentWindow( )->ContentSize.x, bb.GetWidth( ) ), ImMin( GetCurrentWindow( )->ContentSize.y + GImGui->Style.WindowPadding.y * 2 + 2 ds, 200.f ds ) * obj.open_anim } );
        SetWindowPos( { bb.Max.x - GetWindowWidth( ), bb.GetCenter( ).y - GetWindowHeight( ) / 2 } );
        BringWindowToDisplayFront( GetCurrentWindow( ) );
        BringWindowToFocusFront( GetCurrentWindow( ) );
        
        if ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) && IsMouseClicked( 0 ) && !hovered || should_close ) {
            obj.open = false;
        }   
        
        return true;
    }
    
    return false;
}

bool c_widgets::combo( const ptext& label, int* v, std::vector< std::string_view > items ) 
{
	struct s {
		bool should_close;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
	float maxw = 0;
	for ( int i = 0; i < items.size( ); ++i ) {
		maxw = ImMax( CalcTextSize( items[i].data( ), 0, 1 ).x + 48 ds, maxw );
	}

	if ( comboex( label, items[*v], obj.should_close ) ) {
		if ( obj.should_close ) obj.should_close = false;

		for ( int i = 0; i < items.size( ); ++i ) {
			if ( selectable( items[i], *v == i, { ImMax( maxw, GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2 ), 28 ds } ) ) {
				*v = i;
				obj.should_close = true;
			}
		}

		End( );
		PopStyleColor( );
		PopStyleVar( 5 );
	}

	return obj.should_close;
}
bool c_widgets::multicombo( const ptext& label, bool* v, std::vector< std::string_view > items ) 
{
	auto& style = GetStyle( );

    std::string buf;

    buf.clear( );
    for ( size_t i = 0; i < items.size( ); ++i ) {
        if ( v[i] ) {
			buf += ptext{ items[i] }.translate( );
            buf += ", ";
        }
    }

    if ( !buf.empty( ) ) {
        buf.resize( buf.size( ) - 2 );
    }

	float maxw = 0;
	for ( int i = 0; i < items.size( ); ++i ) {
		maxw = ImMax( CalcTextSize( items[i].data( ), 0, 1 ).x + 48 ds, maxw );
	}

    if ( CalcTextSize( buf.c_str( ) ).x > 100 ds ) {
        for ( int i = 0; i < buf.size( ) - 1; ++i ) {
            if ( CalcTextSize( buf.substr( 0, i + 1 ).c_str( ) ).x > 100 ds ) {
                buf.resize( i );
                if ( buf[buf.size( ) - 1] == ',' ) {
                    buf.resize( buf.size( ) - 1 );
                }
                buf.append( ".." );
            }
        }
    }

	if ( buf == "" ) { 
		buf = "None";
	}

	bool result = false;
	if ( comboex( label, buf.c_str( ) ) ) {
		for ( int i = 0; i < items.size( ); ++i ) {
			if ( selectable( items[i], v[i], { ImMax( maxw, GetWindowWidth( ) ), 28 ds } ) ) {
				v[i] = !v[i];
				result = true;
			}
		}

		End( );
		PopStyleColor( );
		PopStyleVar( 5 );
	}

	return result;
}
inline const char* get_key_name( int vk )
{
	switch ( vk )
	{
	case 0: return "NONE";
	case VK_LBUTTON: return "M1";
	case VK_RBUTTON: return "M2";
	case VK_MBUTTON: return "M3";
	case VK_XBUTTON1: return "M4";
	case VK_XBUTTON2: return "M5";
	case VK_BACK: return "BACK";
	case VK_TAB: return "TAB";
	case VK_RETURN: return "ENTER";
	case VK_SHIFT: return "SHIFT";
	case VK_CONTROL: return "CTRL";
	case VK_MENU: return "ALT";
	case VK_ESCAPE: return "ESC";
	case '0': return "0"; case '1': return "1"; case '2': return "2";
	case '3': return "3"; case '4': return "4"; case '5': return "5";
	case '6': return "6"; case '7': return "7"; case '8': return "8";
	case '9': return "9";
	case 'A': return "A"; case 'B': return "B"; case 'C': return "C";
	case 'D': return "D"; case 'E': return "E"; case 'F': return "F";
	case 'G': return "G"; case 'H': return "H"; case 'I': return "I";
	case 'J': return "J"; case 'K': return "K"; case 'L': return "L";
	case 'M': return "M"; case 'N': return "N"; case 'O': return "O";
	case 'P': return "P"; case 'Q': return "Q"; case 'R': return "R";
	case 'S': return "S"; case 'T': return "T"; case 'U': return "U";
	case 'V': return "V"; case 'W': return "W"; case 'X': return "X";
	case 'Y': return "Y"; case 'Z': return "Z";
	case VK_F1: return "F1"; case VK_F2: return "F2";
	case VK_F3: return "F3"; case VK_F4: return "F4";
	case VK_F5: return "F5"; case VK_F6: return "F6";
	case VK_F7: return "F7"; case VK_F8: return "F8";
	case VK_F9: return "F9"; case VK_F10: return "F10";
	case VK_F11: return "F11"; case VK_F12: return "F12";
	}

	return "UNKNOWN";
}
bool c_widgets::binder( const ptext& label , int* key )
{
	struct s {
		float anim;
		float hover;
		float active_anim;
		float w;
		bool active = false;
	};

	auto& obj = anim_obj( label.str.data( ) , 0 , s {} );

	std::string_view buf = obj.active ? "..." : get_key_name( *key );

	obj.w = ImLerp( obj.w ,
		g_draw->textsize( font , 12 , buf.data( ) ).x ,
		GetIO( ).DeltaTime * 17
	);

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );

	ImRect total_bb {
		window->DC.CursorPos,
		window->DC.CursorPos + ImVec2{CalcItemWidth( ), 16.f}
	};

	ImRect bb {
		total_bb.Max - ImVec2{8.f + obj.w, 16.f},
		total_bb.Max
	};

	ItemSize( total_bb );
	ItemAdd( total_bb , id );

	bool hovered , held;
	bool pressed = ButtonBehavior( bb , id , &hovered , &held );

	bool value_changed = false;

	if ( pressed )
	{
		obj.active = true;
		*key = 0;
	}

	if ( obj.active )
	{

		for ( int i = 0; i < 5; i++ )
		{
			if ( ImGui::IsMouseClicked( i ) )
			{
				switch ( i )
				{
				case 0: *key = VK_LBUTTON; break;
				case 1: *key = VK_RBUTTON; break;
				case 2: *key = VK_MBUTTON; break;
				case 3: *key = VK_XBUTTON1; break;
				case 4: *key = VK_XBUTTON2; break;
				}

				value_changed = true;
				obj.active = false;
			}
		}

		if ( !value_changed )
		{
			for ( int i = 1; i < 255; i++ )
			{
				if ( GetAsyncKeyState( i ) & 1 )
				{
					if ( i == VK_ESCAPE )
						*key = 0;
					else
						*key = i;

					value_changed = true;
					obj.active = false;
					break;
				}
			}
		}
	}

	// animation
	obj.hover = anim( obj.hover , 0.f , 1.f , hovered );
	obj.active_anim = anim( obj.active_anim , 0.f , 1.f , obj.active );
	obj.anim = anim( obj.anim , 0.f , 1.f , hovered || obj.active );

	auto bgcol = col_anim(
		g_style->col( pcol_bg3 ) ,
		g_style->col( pcol_bg4 ) ,
		obj.anim
	);

	window->DrawList->AddRectFilled( bb.Min , bb.Max , bgcol , 1.f );
	window->DrawList->AddRect( bb.Min , bb.Max , g_style->col( pcol_border ) , 1.f );

	g_draw->text( font , 12 ,
		bb.Min + vec2 { 4, 2 } ,
		g_style->col( pcol_text ) ,
		buf.data( )
	);

	window->DrawList->AddText(
		{ total_bb.Min.x,
		 total_bb.GetCenter( ).y - GImGui->FontSize / 2 } ,
		g_style->col( pcol_text ) ,
		label.translate( ).data( ) ,
		FindRenderedTextEnd( label.translate( ).data( ) )
	);

	return value_changed;
}
bool c_widgets::selector( const ptext& label, int* v, const std::vector< std::string_view >& items )
{
	struct s {
		ImVec2 size;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	bool result = false;

	GetWindowDrawList( )->AddText( GetCurrentWindow( )->DC.CursorPos + ImVec2{ 0, obj.size.y / 2 - GImGui->FontSize / 2 }, g_style->col( pcol_text ), label.translate( ).data( ) );
	Dummy( CalcTextSize( label.translate( ).data( ) ) );
	SameLine( CalcItemWidth( ) - obj.size.x + GImGui->Style.WindowPadding.x );
	PushStyleColor( ImGuiCol_ChildBg, g_style->col( pcol_bg3 ).vec4( ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 2, 2 } );
	PushStyleVar( ImGuiStyleVar_ChildRounding, 3 ds );
	BeginChild( label.str.data( ), { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border );
	{
		obj.size = GetWindowSize( );

		for ( int i = 0; i < items.size( ); ++i ) {
			if ( selectable( items[i], *v == i, { 0, CalcTextSize( ptext{ items[i] }.translate( ).data( ) ).y + 4 ds }, { .xpad = 4, .rounding = 2, .checkmark = false } ) ) {
				*v = i;
				result = true;
			}

			if ( i < items.size( ) - 1 )
				SameLine( 0, 2 ds );
		}
	}
	EndChild( );
	PopStyleVar( 2 );
	PopStyleColor( );

	return result;
}
bool c_widgets::selectable( const ptext& label, bool selected, ImVec2 size, c_selectablestyle style )
{
	struct s {
		float anim;
		float hover;
		float selected;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	style.rounding *= g_style->dpi_scale;
	style.xpad *= g_style->dpi_scale;

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + CalcItemSize( size, style.xpad * 2 + int( ( 22 ds ) * obj.selected ) * style.checkmark + 4 ds + g_draw->textsize( font, 14, label.translate( ).data( ) ).x, GetFrameHeight( ) ) };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.selected );

	GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg4 ).alpha( obj.selected ), style.rounding );
	if ( style.checkmark ) g_draw->text( icons, 14, { bb.Min.x + style.xpad, bb.GetCenter( ).y - g_draw->textsize( icons, 14, I_CHECK ).y / 2 }, g_style->col( pcol_scheme, obj.selected ), I_CHECK );
	g_draw->text( font, 16, { bb.Min.x + style.xpad + int( ( 22 ds ) * obj.selected ) * style.checkmark + ( 24 ds ) * bool( style.icon ), bb.GetCenter( ).y - GImGui->FontSize / 2 }, col, label.translate( ).data( ) );
	if ( style.icon ) {
		auto iconcol = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_scheme ), obj.selected );
		g_draw->text( icons, 16, { bb.Min.x + style.xpad, bb.GetCenter( ).y - 8 ds }, iconcol, style.icon );
	}

	return pressed;
}

void c_widgets::spacing( float px )
{
	SetCursorPosY( GetCursorPosY( ) - GImGui->Style.ItemSpacing.y + px );
}

void c_widgets::separator( bool vertical, float size )
{
	if ( vertical ) {
		if ( size == 0 ) size = GetWindowHeight( );

		GetWindowDrawList( )->AddRectFilled( { GetCurrentWindow( )->DC.CursorPos.x, GetCurrentWindow( )->DC.CursorPos.y + 8 ds - size / 2 }, { GetCurrentWindow( )->DC.CursorPos.x + 1, GetCurrentWindow( )->DC.CursorPos.y + 8 ds + size / 2 }, g_style->col( pcol_separator ) );
		Dummy( { 1, size } );
	} else {
		if ( size == 0 ) size = GetWindowWidth( ) - GImGui->Style.WindowPadding.x;

		GetWindowDrawList( )->AddRectFilled( { GetWindowPos( ).x + GetWindowWidth( ) - size, GetCurrentWindow( )->DC.CursorPos.y }, { GetWindowPos( ).x + GetWindowWidth( ), GetCurrentWindow( )->DC.CursorPos.y + 1 }, g_style->col( pcol_separator ) );
		Dummy( { CalcItemWidth( ), 1 } );
	}	
}

bool c_widgets::textinput( const ptext& label, char* buf, size_t buf_size, c_textinputstyle style )
{
	bool result = false;

	if ( !style.bgcol ) {
		style.bgcol = g_style->col( pcol_bg3 );
	}

	BeginGroup( );
	if ( CalcTextSize( label.str.data( ), 0, 1 ).x > 0 ) {
		Text( label.translate( ).data( ) );
		spacing( 8 ds );
	}

	PushStyleVar( ImGuiStyleVar_FramePadding, style.padding );
	ImVec2 size{ CalcItemSize( style.size, CalcItemWidth( ), GetFrameHeight( ) ) };
	ImRect bb{ GetCurrentWindow( )->DC.CursorPos, GetCurrentWindow( )->DC.CursorPos + size };
	GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, style.bgcol, style.rounding );
	GetWindowDrawList( )->AddRect( bb.Min, bb.Max, g_style->col( pcol_border ), style.rounding );

	PushStyleColor( ImGuiCol_FrameBg, GetColorU32( ImGuiCol_FrameBg, 0 ) );
	char temp[64];
	ImFormatString( temp, sizeof( temp ), "##%s", label.str.data( ) );

	if ( style.icon ) {
		g_draw->text( icons, 16, bb.Min + style.padding, g_style->col( pcol_text2 ), style.icon );
		SetCursorPosX( GetCursorPosX( ) + 26 ds );
	}

	PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0 );
	result = InputTextEx( temp, style.hint.data( ), buf, buf_size, size - ImVec2{ ( 26 ds ) * bool( style.icon ), 0 }, style.flags );
	PopStyleColor( );
	PopStyleVar( 2 );
	EndGroup( );

	return result;
}

///////////////////////////////////////////////////////////////////////////////


// WINDOW
void c_widgets::widget_window::begin( const std::string_view& name, ImVec2 size, ImGuiWindowFlags flags ) {
	SetNextWindowSize( size );
	Begin( name.data( ), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | flags );
}
void c_widgets::widget_window::end( ) {
	End( );
}

// CHILD

void c_widgets::widget_child::begin( const ptext& name, ImVec2 size ) {
    float padding = GImGui->Style.WindowPadding.x;
    PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );
    BeginChild( name.str.data( ), CalcItemSize( size, GetWindowWidth( ) / 2 - padding - GImGui->Style.ItemSpacing.x / 2, 0 ), 97, 0 );
    
    BeginChild( "header", { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground );
    {
        SetCursorPos( { 12 ds, 12 ds } );
        TextEx( name.translate( ).data( ), FindRenderedTextEnd( name.translate( ).data( ) ) );
    }
    EndChild( );
    PopStyleVar( );
    widgets->spacing( 0 ds );
    
    PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 12, 12 } );
    PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 14, 14 } );
    BeginChild( "content", { 0, size.y == 0 ? 0 : size.y - GetCursorPosY( ) }, 98, 152 );
    PushItemWidth( GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2 );
    smoothscroll( true );
}


void c_widgets::widget_child::end( ) {
	PopItemWidth( );
	EndChild( );
	PopStyleVar( 2 );
	EndChild( );
}

void c_widgets::widget_child::header( const std::string_view& icon, const ptext& name, bool* v, float* col, bool border )
{
	PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 14, 14 } );
	BeginChild( name.str.data( ), { 200, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground );
	{
		GetWindowDrawList( )->AddRectFilled( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), g_style->col( pcol_bg5 ), GImGui->Style.ChildRounding, border ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersTop );
		if ( border )
			GetWindowDrawList( )->AddRect( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), g_style->col( pcol_border ), GImGui->Style.ChildRounding );
		
		PushFont( fonts[icons].get( 16 ) );
		TextColored( g_style->col( pcol_scheme ), icon.data( ) );
		PopFont( );
		SameLine( 0, 24 ds );
		Text( name.translate( ).data( ) );

		char temp[128];
		if ( col ) {
			ImFormatString( temp, sizeof( temp ), "##%s col", name.str.data( ) );
			SameLine( GetWindowWidth( ) - 64 ds );
			widgets->coloredit( temp, col, vec2{ 16, 16 } );
			SameLine( 0, 10 ds );
		} else {
			SameLine( GetWindowWidth( ) - 38 ds );
		}
		if ( v ) {
			ImFormatString( temp, sizeof( temp ), "##%s toggle", name.str.data( ) );
			widgets->checkbox( temp, v );
		}
	}
	EndChild( );
	PopStyleVar( );
}
float* c_widgets::widget_child::smoothscroll( bool scrollbar, ImVec2 padding )
{
	struct s {
		ImVec2 scroll;
		ImVec2 scroll_anim;
		bool scroll_activey = false;
		bool scroll_activex = false;
		float oldscroll;
		float clickpos;

		float xanim;
		float yanim;
	}; auto& obj = anim_obj( GetCurrentWindow( )->Name, 23123, s{ } );

	obj.scroll.y = ImClamp( obj.scroll.y, 0.f, GetCurrentWindow( )->ScrollMax.y );
	obj.scroll.x = ImClamp( obj.scroll.x, 0.f, GetCurrentWindow( )->ScrollMax.x );
	obj.scroll_anim.y = ImClamp( obj.scroll_anim.y, 0.f, GetCurrentWindow( )->ScrollMax.y );
	obj.scroll_anim.x = ImClamp( obj.scroll_anim.x, 0.f, GetCurrentWindow( )->ScrollMax.x );

	if ( scrollbar ) {
		// vertical
		{
			ImRect scrollbarbb{ GetWindowPos( ) + ImVec2{ GetWindowWidth( ) - 5 ds, 4 ds + padding.y }, GetWindowPos( ) + GetWindowSize( ) - vec2{ 2 ds, 4 ds + padding.y } };
			float visiblepart = GetWindowHeight( ) / ( GetCurrentWindow( )->ContentSize.y + GImGui->Style.WindowPadding.y * 2 );

			if ( visiblepart < 1.f ) {
				float scrollh = scrollbarbb.GetHeight( ) * visiblepart;
				float invisiblepart = 1.f - visiblepart;
				float scrolloffset = ( obj.scroll.y / GetCurrentWindow( )->ScrollMax.y ) * ( scrollbarbb.GetHeight( ) - scrollh );

				obj.yanim = anim( obj.yanim, 0.f, 1.f, IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) || obj.scroll_activey );
				auto col = col_anim( g_style->col( pcol_scheme ), g_style->col( pcol_scheme, 0.8f ), obj.yanim );

				GetWindowDrawList( )->AddRectFilled( scrollbarbb.Min + vec2{ 1, 0 }, scrollbarbb.Max - vec2{ 1, 0 }, g_style->col( pcol_bg3 ), 5 ds );
				GetWindowDrawList( )->PushClipRect( scrollbarbb.Min, scrollbarbb.Max );
				GetWindowDrawList( )->AddRectFilled( scrollbarbb.Min + ImVec2{ 0, scrolloffset }, { scrollbarbb.Max.x, scrollbarbb.Min.y + scrollh + scrolloffset }, col, 5 ds );
				GetWindowDrawList( )->PopClipRect( );

				if ( IsMouseClicked( 0 ) && IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) && !obj.scroll_activey ) {
					obj.scroll_activey = true;
					obj.oldscroll = obj.scroll.y;
					obj.clickpos = ( GetIO( ).MousePos.y - scrollbarbb.Min.y ) / scrollbarbb.GetHeight( );
				} if ( !IsMouseDown( 0 ) ) obj.scroll_activey = false;

				if ( obj.scroll_activey ) {
					GetCurrentWindow( )->Flags |= ImGuiWindowFlags_NoMove;

					float newscroll = ( GetIO( ).MousePos.y - scrollbarbb.Min.y ) / scrollbarbb.GetHeight( );
					float diff = ( newscroll - obj.clickpos ) / invisiblepart;

					obj.scroll.y = obj.oldscroll + diff * GetCurrentWindow( )->ScrollMax.y;
					obj.scroll.y = ImClamp( obj.scroll.y, 0.f, GetCurrentWindow( )->ScrollMax.y );
				}
			}
		}
		// horizontal
		{
			ImRect scrollbarbb{ GetWindowPos( ) + ImVec2{ 3 ds + padding.x ds, GetWindowHeight( ) - 5 ds }, GetWindowPos( ) + GetWindowSize( ) - vec2{ 3 + padding.x, 2 } };
			float visiblepart = GetWindowWidth( ) / ( GetCurrentWindow( )->ContentSize.x + GImGui->Style.WindowPadding.x * 2 );

			if ( visiblepart < 1.f ) {
				float scrollh = scrollbarbb.GetWidth( ) * visiblepart;
				float invisiblepart = 1.f - visiblepart;
				float scrolloffset = ( obj.scroll.x / GetCurrentWindow( )->ScrollMax.x ) * ( scrollbarbb.GetWidth( ) - scrollh );

				obj.xanim = anim( obj.xanim, 0.f, 1.f, IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) || obj.scroll_activex );
				auto col = col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_text2 ), obj.xanim );

				GetWindowDrawList( )->PushClipRect( scrollbarbb.Min, scrollbarbb.Max );
				GetWindowDrawList( )->AddRectFilled( scrollbarbb.Min + ImVec2{ scrolloffset, 0 }, { scrollbarbb.Min.x + scrollh + scrolloffset, scrollbarbb.Max.y }, col, 5 ds );
				GetWindowDrawList( )->PopClipRect( );

				if ( IsMouseClicked( 0 ) && IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) && !obj.scroll_activex ) {
					obj.scroll_activex = true;
					obj.oldscroll = obj.scroll.x;
					obj.clickpos = ( GetIO( ).MousePos.x - scrollbarbb.Min.x ) / scrollbarbb.GetWidth( );
				} if ( !IsMouseDown( 0 ) ) obj.scroll_activex = false;

				if ( obj.scroll_activex ) {
					GetCurrentWindow( )->Flags |= ImGuiWindowFlags_NoMove;

					float newscroll = ( GetIO( ).MousePos.x - scrollbarbb.Min.x ) / scrollbarbb.GetWidth( );
					float diff = ( newscroll - obj.clickpos ) / invisiblepart;

					obj.scroll.x = obj.oldscroll + diff * GetCurrentWindow( )->ScrollMax.x;
					obj.scroll.x = ImClamp( obj.scroll.x, 0.f, GetCurrentWindow( )->ScrollMax.x );
				}
			}
		}
	}

	GetCurrentWindow( )->Scroll.y = obj.scroll_anim.y < 0.5f ? 0.f : obj.scroll_anim.y > ( GetCurrentWindow( )->ScrollMax.y - 0.5f ) ? GetCurrentWindow( )->ScrollMax.y : ImLerp( GetCurrentWindow( )->Scroll.y, obj.scroll.y, GetIO( ).DeltaTime * 40 );
	GetCurrentWindow( )->Scroll.x = ImLerp( GetCurrentWindow( )->Scroll.x, obj.scroll.x, GetIO( ).DeltaTime * 40 );

	ImGuiWindow* wheeling_window = nullptr;
	if ( GImGui->HoveredWindow ) {
		if ( GImGui->HoveredWindow->Flags & ImGuiWindowFlags_ChildWindow ) {
			for ( ImGuiWindow* window = GImGui->HoveredWindow; window->Flags & ImGuiWindowFlags_ChildWindow; window = window->ParentWindow ) {
				if ( window->ScrollMax[ImGuiAxis_Y] == 0 )
					continue;

				wheeling_window = window;
			}
		} else {
			wheeling_window = GImGui->HoveredWindow;
		}
	}

	if ( wheeling_window == GetCurrentWindow( ) ) {
		if ( !IsKeyDown( ImGuiKey_LeftCtrl ) )
			obj.scroll.y = ImClamp( obj.scroll.y - GetIO( ).MouseWheel * 80, 0.f, GetCurrentWindow( )->ScrollMax.y );
		else
			obj.scroll.x = obj.scroll.x - GetIO( ).MouseWheel * 80;
	}

	obj.scroll_anim.y = ImLerp( obj.scroll_anim.y, obj.scroll.y, GetIO( ).DeltaTime * 40 );
	obj.scroll_anim.x = ImLerp( obj.scroll_anim.x, obj.scroll.x, GetIO( ).DeltaTime * 40 );

	return &obj.scroll.y;
}

// MENU
void c_widgets::widget_menu::begin( const std::string_view& str_id ) {

}
void c_widgets::widget_menu::end( ) {

}
bool c_widgets::widget_menu::button( const ptext& label, ImVec2 size, c_buttonstyle style ) {
	return false;
}

// NOTIFY
void c_widgets::widget_notify::add( const std::string_view& title, const std::string_view& text, notify_ status )
{
	notifications.emplace_back( c_notify{ title, text, status, 3.f } );
}
void c_widgets::widget_notify::handle( )
{
	auto draw_list = GetBackgroundDrawList( );

	float offset = 0.f;
	for ( int i = 0; i < notifications.size( ); ++i ) {
		auto& n = notifications[i];
		float alpha = n.time <= n.fade_time ? n.time / n.fade_time : n.time >= n.duration - n.fade_time ? ( n.duration - n.time ) / n.fade_time : 1.f;

		ImVec2 size{ ImMax( CalcTextSize( n.message.data( ) ).x, CalcTextSize( n.title.data( ) ).x ) + 82 ds, 62 ds };

		if ( n.pos.x == 0 ) n.pos = GetIO( ).DisplaySize - ImVec2{ 0, 20 + offset + size.y };

		n.pos.x = ImLerp( n.pos.x, GetIO( ).DisplaySize.x - size.x - 20, GetIO( ).DeltaTime * 14 );
		n.pos.y = ImLerp( n.pos.y, GetIO( ).DisplaySize.y - 20 - offset - size.y, GetIO( ).DeltaTime * 14 );

		pcolor colors[] = {
			g_style->col( pcol_scheme ).h( 70.f / 172.f ),
			g_style->col( pcol_scheme ).h( 0.14f ),
			g_style->col( pcol_scheme ).h( 0.f ),
			g_style->col( pcol_scheme ).h( 0.59f ),
		};

		draw_list->AddRectFilled( n.pos, n.pos + size, g_style->col( pcol_bg, alpha ), 4 ds );
		draw_list->AddRectFilled( n.pos + vec2{ 40, 6 }, n.pos + size - vec2{ 6, 6 }, g_style->col( pcol_bg2, alpha ), 3 ds );
		draw_list->AddRectFilled( { n.pos.x + 4 ds, n.pos.y + size.y - 1 ds }, { n.pos.x + ( size.x - 8 ds ) * ( n.time / n.duration ), n.pos.y + size.y }, colors[n.status].alpha( alpha ) );

		const char* n_icons[] = {
			I_CIRCLE__CHECK,
			I_ALERT__CIRCLE,
			I_CIRCLE__X,
			I_INFO__CIRCLE,
		};

		g_draw->text( icons, 16, n.pos + vec2{ 12, 20.5f }, colors[n.status].alpha( alpha ), n_icons[n.status], false, draw_list );
		g_draw->text( font, 16, n.pos + vec2{ 50, 14 }, g_style->col( pcol_text2, alpha ), n.title.data( ), false, draw_list );
		g_draw->text( font, 14, n.pos + vec2{ 50, 33 }, g_style->col( pcol_text, alpha ), n.message.data( ), false, draw_list );

		n.time += 1.f / GetIO( ).Framerate;
		offset += size.y + 12;
	}

	notifications.erase(
		std::remove_if( notifications.begin( ), notifications.end( ), []( const c_notify& n ) {
			return n.time >= n.duration;
		} ), 
		notifications.end( )
	);
}

// POPUP
bool c_widgets::widget_popup::begin( const std::string_view& str_id, float alpha, ImVec2 pos, ImVec2 padding )
{
	char temp[64];
	ImFormatString( temp, sizeof( temp ), "%s popup", str_id.data( ) );

	if ( alpha > 0.05f ) {
		if ( pos != ImVec2{ 0, 0 } )
			SetNextWindowPos( pos );
		PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * alpha );
		PushStyleVar( ImGuiStyleVar_WindowPadding, padding );
		PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1 );
		PushStyleVar( ImGuiStyleVar_WindowRounding, 4 ds );
		Begin( temp, 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize );

		if ( alpha > 0.95f ) {
			GetCurrentWindow( )->Flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		} else {
			GetCurrentWindow( )->Flags &= ~ImGuiWindowFlags_NoBringToFrontOnFocus;
		}

		return true;
	}

	return false;
}
void c_widgets::widget_popup::end( )
{
	End( );
	PopStyleVar( 4 );
}

bool c_widgets::widget_nav::subtab( const c_subtab& item, bool selected )
{
    struct s {
        float anim;
        float hover;
        float selected;
    }; auto& obj = anim_obj( item.label.str.data( ), 0, s{ } );
    
    auto window = GetCurrentWindow( );
    auto id = window->GetID( item.label.str.data( ) );
    ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 154 ds, 0 } };
    ItemSize( bb );
    ItemAdd( bb, id );
    
    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held );
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.selected = anim( obj.selected, 0.f, 1.f, selected );
    obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

    window->DrawList->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_scheme, obj.selected ), 10 ds );
    window->DrawList->AddText( bb.Min + vec2{ 73.25, 8 }, col_anim( col_anim( pcolor{ 255, 255, 255, 0.40f }, g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.selected ), item.label.translate( ).data( ), FindRenderedTextEnd( item.label.translate( ).data( ) ) );


    return pressed;
}



bool c_widgets::widget_nav::tab( c_tab tab, bool selected )
{
    auto window = GetCurrentWindow( );
    
    struct s {
        float anim;
        float hover;
        float selected;
    }; auto& obj = anim_obj( tab.label.str.data( ), 0, s{ } );
    
    bool pressed = InvisibleButton( tab.label.str.data( ), CalcTextSize( tab.label.translate( ).data( ), 0, 1 ) + ImVec2{ 24, 0 } );
    bool hovered = IsItemHovered( ), held = IsItemActive( );
    ImRect bb = GImGui->LastItemData.Rect;
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.selected = anim( obj.selected, 0.f, 1.f, selected );
    obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );
    
    g_draw->text( icons, 16, bb.Min + vec2{ 0, 0 }, col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_scheme ), obj.selected ), tab.icon );
    window->DrawList->AddText( bb.Min + vec2{ 24, 0 }, col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text ), obj.selected ), tab.label.translate( ).data( ), FindRenderedTextEnd( tab.label.translate( ).data( ) ) );
    
    
    return pressed;
}

void c_widgets::widget_nav::drawtabs( )
{
    static float w = 0;
	SetCursorPosX( GetWindowWidth( ) / 2 - w / 2 );
	float p = GetCursorPosX( );
    BeginGroup( );
    {
        for ( int i = 0; i < tabs.size( ); ++i ) {
            if ( tab( tabs[i], next == i ) && next != i ) {
                next = i;
                tab_animdest = 0;
            }
            
            SameLine( 0, 14 ds );
        }
		w = GetCursorPosX( ) - 14 ds - p;
    }
    EndGroup( );
    
    
    tab_anim = ImLerp( tab_anim, tab_animdest, GetIO( ).DeltaTime * 17 );
    subtab_anim = ImLerp( subtab_anim, subtab_animdest, GetIO( ).DeltaTime * 17 );
    
    if ( tab_anim < 0.05f ) {
        tab_animdest = 1.f;
        current = next;
    }
    
    if ( subtab_anim < 0.05f ) {
        subtab_animdest = 1.f;
        tabs[current].current = tabs[current].next;
    }
}
void c_widgets::widget_nav::drawsubtabs( )
{
    if ( tabs[current].subtabs.empty( ) )
    return;
        
    PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * tab_anim );
    SetCursorPos( vec2{ 16, 16 } );
    PushStyleColor( ImGuiCol_ChildBg, pcolor{ 25, 25, 25, 1.00f }.vec4( ) );
    PushStyleVar( ImGuiStyleVar_ChildRounding, 12 ds );
    BeginChild( "subtabs", vec2{ 648, 47 }, 1, 0 );
    {
        PopStyleColor( );
        PopStyleVar( );
        
        for ( int i = 0; i < tabs[current].subtabs.size( ); ++i ) {
            if ( subtab( tabs[current].subtabs[i], i == tabs[current].next ) && i != tabs[current].next ) {
                tabs[current].next = i;
                subtab_animdest = 0;
            }
            SameLine( 0, 6.5 ds );
        }
    }
    EndChild( );
    
    PopStyleVar( );
}
void c_widgets::widget_nav::drawpage( )
{
	if ( tabs[current].pages.size( ) <= tabs[current].current )
		return;

	tabs[current].pages[tabs[current].current]( );
}
void c_widgets::widget_nav::addpage( int tab, std::function< void( ) > code )
{
	tabs[tab].pages.push_back( code );
}

// COLORPICKER

bool c_widgets::widget_colorpicker::huebar( const char* str_id, float* h, float s, float v )
{
	float h_values[] {
		0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f
	};
	int h_size = IM_ARRAYSIZE( h_values );

	ImVec2 size{ 207 ds, int( 5 ds ) * 1.f };
	ImRect bb{ GetCurrentWindow( )->DC.CursorPos, GetCurrentWindow( )->DC.CursorPos + size };

	for ( int i = 0; i < h_size - 1; ++i ) {
		ImColor col1, col2;

		ColorConvertHSVtoRGB( h_values[i], ImClamp( s, 0.6f, 1.f ), ImClamp( v, 0.6f, 1.f ), col1.Value.x, col1.Value.y, col1.Value.z );
		ColorConvertHSVtoRGB( h_values[i + 1], ImClamp( s, 0.6f, 1.f ), ImClamp( v, 0.6f, 1.f ), col2.Value.x, col2.Value.y, col2.Value.z );
		col1.Value.w = col2.Value.w = GImGui->Style.Alpha;
		
		g_draw->gradient( GetWindowDrawList( ), { bb.Min.x + ( size.x / ( h_size - 1 ) ) * i, bb.Min.y }, { bb.Min.x + ( size.x / ( h_size - 1 ) ) * ( i + 1 ), bb.Max.y }, col1, col2, col2, col1, 4, i == 0 ? ImDrawFlags_RoundCornersLeft : ( i == h_size - 2 ) ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersNone );
	}

	GetWindowDrawList( )->AddCircle( { bb.Min.x + size.x * *h, bb.GetCenter( ).y }, 3 ds, ImColor{ 1.f, 1.f, 1.f, GImGui->Style.Alpha } );

	InvisibleButton( "hue", size );
	if ( IsItemActive( ) ) {
		*h = ImSaturate( ( GetIO( ).MousePos.x - bb.Min.x ) / size.x );

		return true;
	}

	return false;
}
bool c_widgets::widget_colorpicker::alphabar( float* col )
{
	ImVec2 size{ 6 ds, 132 ds };
	ImRect bb{ GetCurrentWindow( )->DC.CursorPos, GetCurrentWindow( )->DC.CursorPos + size };

	float square_sz = 3 ds;

	GetWindowDrawList( )->Flags &= ~ImDrawListFlags_AntiAliasedFill;
	for ( int i = 0; i < size.y / ( square_sz ) - 1; ++i ) {
		GetWindowDrawList( )->AddRectFilled( bb.Min + ImVec2{ 0, square_sz * i }, bb.Min + ImVec2{ bb.GetWidth( ) / 2, square_sz * ( i + 1 ) }, ( i + 1 ) % 2 == 0 ? ImColor{ 222, 222, 222, int( 255 * GImGui->Style.Alpha ) } : ImColor{ 155, 155, 155, int( 255 * GImGui->Style.Alpha ) }, 4, i == 0 ? ImDrawFlags_RoundCornersTopLeft : ( i == size.y / square_sz - 2 ) ? ImDrawFlags_RoundCornersBottomLeft : ImDrawFlags_RoundCornersNone );
		GetWindowDrawList( )->AddRectFilled( bb.Min + ImVec2{ bb.GetWidth( ) / 2, square_sz * i }, bb.Min + ImVec2{ bb.GetWidth( ), square_sz * ( i + 1 ) }, ( i + 1 ) % 2 != 0 ? ImColor{ 222, 222, 222, int( 255 * GImGui->Style.Alpha ) } : ImColor{ 155, 155, 155, int( 255 * GImGui->Style.Alpha ) }, 4, i == 0 ? ImDrawFlags_RoundCornersTopRight : ( i == size.y / square_sz - 2 ) ? ImDrawFlags_RoundCornersBottomRight : ImDrawFlags_RoundCornersNone );
	}

	g_draw->gradient( GetWindowDrawList( ), bb.Min, bb.Max, ImColor{ col[0], col[1], col[2], GImGui->Style.Alpha }, ImColor{ col[0], col[1], col[2], GImGui->Style.Alpha }, ImColor{ col[0], col[1], col[2], 0.f }, ImColor{ col[0], col[1], col[2], 0.f }, 4 );
	GetWindowDrawList( )->Flags |= ImDrawListFlags_AntiAliasedFill;

	GetWindowDrawList( )->AddCircleFilled( { bb.GetCenter( ).x, bb.Min.y + ( 1.f - col[3] ) * size.y }, 3.5f ds, ImColor{ 1.f, 1.f, 1.f, GImGui->Style.Alpha }, 30 );
	GetWindowDrawList( )->AddCircleFilled( { bb.GetCenter( ).x, bb.Min.y + ( 1.f - col[3] ) * size.y }, 3.5f ds, ImColor{ col[0], col[1], col[2], col[3] * GImGui->Style.Alpha }, 30 );
	GetWindowDrawList( )->AddCircle( { bb.GetCenter( ).x, bb.Min.y + ( 1.f - col[3] ) * size.y }, 3.5f ds, ImColor{ 1.f, 1.f, 1.f, GImGui->Style.Alpha }, 30 );

	InvisibleButton( "a", size );
    if ( IsItemActive( ) )
    {
        col[3] = 1.f - ImSaturate( ( GetIO( ).MousePos.y - bb.Min.y ) / size.y );

        return true;
    }

	return false;
}
bool c_widgets::widget_colorpicker::square( const char* str_id, float h, float* s, float* v )
{
	ImVec2 size{ 193 ds, 132 ds };
	ImRect bb{ GetCurrentWindow( )->DC.CursorPos, GetCurrentWindow( )->DC.CursorPos + size };

	ImColor col_white{ 1.f, 1.f, 1.f, GImGui->Style.Alpha };
	ImColor col_black{ 0.f, 0.f, 0.f, GImGui->Style.Alpha };
	ImColor col_hue;

	ColorConvertHSVtoRGB( h, 1, 1, col_hue.Value.x, col_hue.Value.y, col_hue.Value.z );
	col_hue.Value.w = GImGui->Style.Alpha;

	GetWindowDrawList( )->Flags &= ~ImDrawListFlags_AntiAliasedFill;
	g_draw->gradient( GetWindowDrawList( ), bb.Min, bb.Max, col_white, col_hue, col_hue, col_white, 4 ds, 0 );
    g_draw->gradient( GetWindowDrawList( ), bb.Min, bb.Max, 0, 0, col_black, col_black, 4 ds, 0 );
	GetWindowDrawList( )->Flags |= ImDrawListFlags_AntiAliasedFill;

	GetWindowDrawList( )->AddCircle( bb.Min + size * ImVec2{ *s, 1.f - *v }, 3 ds, col_white, 36 );

	InvisibleButton( "sv", size );
    if ( IsItemActive( ) )
    {
        *s = ImSaturate( ( GetIO( ).MousePos.x - bb.Min.x ) / size.x );
        *v = 1.f - ImSaturate( ( GetIO( ).MousePos.y - bb.Min.y ) / size.y );

        return true;
    }
}
bool c_widgets::widget_colorpicker::draw( const std::string_view& str_id, float* col )
{
	bool value_changed = false;

	struct s {
		float h, s, v;
		bool init;
	}; auto& obj = anim_obj( str_id.data( ), 2323321, s{ } );

	if ( !obj.init ) {
		ColorConvertRGBtoHSV( col[0], col[1], col[2], obj.h, obj.s, obj.v );
		obj.init = true;
	}
	
	BeginGroup( );
	value_changed |= square( str_id.data( ), obj.h, &obj.s, &obj.v );
	SameLine( );
	value_changed |= alphabar( col );
	EndGroup( );
	value_changed |= huebar( str_id.data( ), &obj.h, obj.s, obj.v );

	static char buf[7];
	static char alpha_buf[7];
	ImFormatString( buf, sizeof( buf ), "%02X%02X%02X", int( col[0] * 255 ), int( col[1] * 255 ), int( col[2] * 255 ) );
	ImFormatString( alpha_buf, sizeof( alpha_buf ), "%d%%", int( col[3] * 100 ) );

	PushStyleColor( ImGuiCol_FrameBg, GetColorU32( ImGuiCol_FrameBgHovered ) );
	PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1 );
	PushStyleVar( ImGuiStyleVar_FramePadding, { 6, 3 } );
	PushItemFlag( ImGuiItemFlags_NoNav, true );

	if ( value_changed ) {
		ColorConvertHSVtoRGB( obj.h, obj.s, obj.v, col[0], col[1], col[2] );
	}

	bool hex_changed = widgets->textinput( "##hex_input", buf, sizeof( buf ), { .padding = vec2{ 8, 8 }, .icon = I_HASH, .size = vec2{ 117, 0 } } );
	SameLine( 0, 10 ds );

	bool alpha_changed = widgets->textinput( "##a_input", alpha_buf, sizeof( alpha_buf ), { .padding = vec2{ 8, 8 }, .icon = I_DROPLET__HALF__2, .size = vec2{ 80, 0 } } );

	PopItemFlag( );
	PopStyleVar( 2 );
	PopStyleColor( );

	PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 5, 5 } );
	if ( widgets->iconbutton( "addcol", I_PLUS ) ) {
		history.push_back( { col[0], col[1], col[2], col[3] } );
	}

	SameLine( );

	char temp[128];
	for ( int i = 0; i < history.size( ); ++i ) {
		ImFormatString( temp, sizeof( temp ), "%s%d", str_id, i );
		if ( colorbutton( temp, history[i] ) ) {
			value_changed = true;
			col[0] = history[i].Value.x;
			col[1] = history[i].Value.y;
			col[2] = history[i].Value.z;
			col[3] = history[i].Value.w;
			obj.init = false;
		}

		if ( i < 8 || ( i > 8 && i < 18 ) ) {
			SameLine( );
		} else if ( i > 18 ) {
			if ( ( i - 8 ) % 10 != 0 )
				SameLine( );
		}
	}
	PopStyleVar( );

	int i[4];
	sscanf( buf, "%02X%02X%02X", ( unsigned int* )&i[0], ( unsigned int* )&i[1], ( unsigned int* )&i[2] );
	sscanf( alpha_buf, "%d%%", ( unsigned int* )&i[3] );
	if ( hex_changed ) {
		col[0] = i[0] / 255.f;
		col[1] = i[1] / 255.f;
		col[2] = i[2] / 255.f;

		ColorConvertRGBtoHSV( col[0], col[1], col[2], obj.h, obj.s, obj.v );
	}
	
	if ( alpha_changed ) {
		col[3] = i[3] / 100.f;
	}

	return value_changed;
}
bool c_widgets::widget_colorpicker::colorbutton( const char* str_id, const ImColor& color )
{
	struct s {
		float anim = 0;
		float hover = 0;
		float held = 0;
	}; auto& obj = anim_obj( str_id, 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.held = anim( obj.held, 0.f, 1.f, held );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || held );

	window->DrawList->AddCircleFilled( bb.GetCenter( ), bb.GetSize( ).x / 2, g_style->col( pcol_bg3 ), bb.GetSize( ).x * 5 );

	window->DrawList->PushClipRect( bb.GetCenter( ), bb.Max );
	window->DrawList->AddCircleFilled( bb.GetCenter( ), bb.GetSize( ).x / 2, pcolor{ 255, 255, 255 }, bb.GetSize( ).x * 5 );
	window->DrawList->PopClipRect( );

	window->DrawList->PushClipRect( bb.Min, { bb.GetCenter( ).x, bb.Max.y } );
	window->DrawList->AddCircleFilled( bb.GetCenter( ), bb.GetSize( ).x / 2, ImColor{ color.Value.x, color.Value.y, color.Value.z, GImGui->Style.Alpha }, bb.GetSize( ).x * 5 );
	window->DrawList->PopClipRect( );

	window->DrawList->PushClipRect( { bb.GetCenter( ).x, bb.Min.y }, bb.Max );
	window->DrawList->AddCircleFilled( bb.GetCenter( ), bb.GetSize( ).x / 2, ImColor{ color.Value.x, color.Value.y, color.Value.z, GImGui->Style.Alpha * color.Value.w }, bb.GetSize( ).x * 5 );
	window->DrawList->PopClipRect( );

	window->DrawList->AddCircle( bb.GetCenter( ), bb.GetSize( ).x / 2 - 1, pcolor{ 0, 0, 0, 0.3f }, bb.GetSize( ).x * 5, 2.5f );

	return pressed;
}

// MODAL
void c_widgets::widget_modal::add( std::function< void( ) > code )
{
	modals.push_back( { code } );
}
void c_widgets::widget_modal::close( )
{
	modals[modals.size( ) - 1].anim_dest = 0;
}
void c_widgets::widget_modal::handle( )
{
	int i = 0;
	for ( auto& modal : modals ) {
		modal.anim = ImLerp( modal.anim, modal.anim_dest, GetIO( ).DeltaTime * 17 );

		if ( modal.anim < 0.05f && modal.anim_dest == 0 ) {
			modals.erase( modals.begin( ) + ( modals.size( ) - 1 ) );
		}

		SetCursorPos( vec2{ 8, 8 } );
		char temp[64];
		ImFormatString( temp, sizeof( temp ), "modal %d", i );
		PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * modal.anim );
		PushStyleColor( ImGuiCol_ChildBg, GetColorU32( ImGuiCol_WindowBg, 0.92f ) );
		BeginChild( temp, -vec2{ 8, 8 }, 0, ImGuiWindowFlags_NoBackground );
		{
			PopStyleColor( );

			GetWindowDrawList( )->AddRectFilled( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), GetColorU32( ImGuiCol_ChildBg ), GImGui->Style.ChildRounding );
			GetWindowDrawList( )->AddRect( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), GetColorU32( ImGuiCol_Border ), GImGui->Style.ChildRounding );

			if ( IsWindowHovered( ) && IsMouseClicked( 0 ) ) {
				close( );
			}

			if ( modal.code )
				modal.code( );
		}
		EndChild( );
		PopStyleVar( );

		i++;
	}
}
bool c_widgets::widget_modal::widget( const ptext& label, std::function< void( ) > code )
{
	ImGuiWindow* window = GetCurrentWindow( );
	bool pressed = InvisibleButton( label.str.data( ), { CalcItemWidth( ), GImGui->FontSize } );
	bool hovered = IsItemHovered( ), held = IsItemActive( );
	ImRect bb = GImGui->LastItemData.Rect;

	struct s {
		float hover = 0;
	}; auto& obj = anim_obj( label.str.data( ), 123444320, s{ } );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );

	auto col = col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover );
	window->DrawList->AddText( bb.Min, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
	g_draw->text( icons, 12, { bb.Max.x - 12 ds, bb.GetCenter( ).y - 6 ds }, col, I_CHEVRON__RIGHT );

	if ( pressed ) {
		add( code );
	}

	return pressed;
}

// BINDER

void c_widgets::widget_binder::popup( const std::string_view& label, void* v, ht_ type, float min, float max, std::vector< std::string_view > items )
{
	struct s {
		float open_anim;
		bool open = false;
		float settings_anim;
	}; auto& obj = anim_obj( label.data( ), 04312, s{ } );

	auto it = hotkeys.find( label );
	if ( it == hotkeys.end( ) ) {
		hotkeys.insert( { label, { { }, type, v } } );
		it = hotkeys.find( label );
	}
	auto& item = it->second;

	if ( IsItemClicked( 1 ) ) {
		obj.open = true;
		SetNextWindowPos( GetIO( ).MousePos );
	}

	obj.settings_anim = anim( obj.settings_anim, 0.f, 1.f, item.selected < item.binds.size( ) && !item.binds.empty( ) );

	obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );

	PushItemFlag( ImGuiItemFlags_NoNav, true );
	if ( widgets->popup.begin( label, obj.open_anim, { 0, 0 }, vec2{ 8, 8 } ) ) {
		PushItemWidth( 120 ds );

		if ( ( !IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) || ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) ) ) && ( IsMouseClicked( 0 ) || ( IsMouseClicked( 1 ) && obj.open_anim > 0.9f ) ) ) {
			obj.open = false;
		}

		char temp[64];

		PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 4, 4 } );
		g_style->push( pcol_bg4, g_style->col( pcol_bg3 ) );
		for ( int i = 0; i < item.binds.size( ); ++i ) {
			ImFormatString( temp, sizeof( temp ), "%s##%d", keys[item.binds[i].key], i );
			if ( widgets->selectable( temp, item.selected == i, { GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2, 0 }, { .rounding = 3 } ) ) {
				item.selected = i;
			}
		}
		g_style->pop( );

		if ( widgets->button( "Add", vec2{ 0, 30 }, { .icon = I_PLUS } ) ) {
			if ( item.binds.size( ) < 5 )
				item.binds.push_back( { } );
		}

		widgets->separator( );

		if ( widgets->button( "Delete all", vec2{ 0, 30 }, { .icon = I_TRASH, .color = g_style->col( pcol_bg2 ), .textcolor = g_style->col( pcol_scheme ).h( 0 ), .iconcolor = g_style->col( pcol_scheme ).h( 0 ), .lightenval = 1.2f } ) ) {
			item.binds.clear( );
		}
		PopStyleVar( );

		if ( widgets->popup.begin( "settings", obj.settings_anim, GetCurrentWindow( )->Pos + ImVec2{ GetCurrentWindow( )->Size.x + 8 ds, 0 }, vec2{ 8, 8 } ) ) {
			PushItemWidth( 180 ds );
			widgets->binder( "Binder", &item.binds[item.selected].key );

			if ( type != ht_button ) {
				widgets->selector( "Mode", &item.binds[item.selected].mode, { "Hold", "Toggle" } );

				switch ( type ) {
				case ht_checkbox: {
					if ( !item.value.has_value( ) ) item.value = false;
					widgets->checkbox( "Value", &std::any_cast< bool& >( item.value ) ); 
				} break;
				case ht_sliderint: {
					if ( !item.value.has_value( ) ) item.value = ( int )min;
					widgets->sliderint( "Value", &std::any_cast< int& >( item.value ), ( int )min, ( int )max );
				} break;
				case ht_sliderfloat: {
					if ( !item.value.has_value( ) ) item.value = min;
					widgets->sliderfloat( "Value", &std::any_cast< float& >( item.value ), min, max );
				} break;
				case ht_combo: {
					if ( !item.value.has_value( ) ) item.value = 0;
					widgets->combo( "Value", &std::any_cast< int& >( item.value ), items );
				} break;
				}
			}
			
			widgets->separator( );
			if ( widgets->button( "Delete", vec2{ 0, 30 }, { .icon = I_TRASH, .color = g_style->col( pcol_bg2 ), .textcolor = g_style->col( pcol_scheme ).h( 0 ), .iconcolor = g_style->col( pcol_scheme ).h( 0 ), .lightenval = 1.2f } ) ) {
				item.binds.erase( item.binds.begin( ) + item.selected );
				item.selected = 0;
			}
			PopItemWidth( );
			widgets->popup.end( );
		}

		PopItemWidth( );
		widgets->popup.end( );
	}
	PopItemFlag( );
}

// TABLE

void c_widgets::widget_table::begin( const char* str_id, const std::vector< ptext >& columns )
{
	this->id = str_id;
	this->columns = columns;
	this->colsizes.resize( columns.size( ) );
	
	float wp = GImGui->Style.WindowPadding.x;

	PushStyleColor( ImGuiCol_ChildBg, g_style->col( pcol_bg3 ).vec4( ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );
	BeginChild( str_id, { GetWindowWidth( ) - wp * 2, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border );
	PopStyleVar( );
	PopStyleColor( );

	char temp[64];
	ImFormatString( temp, sizeof( temp ), "%s header", str_id );
	PushStyleVar( ImGuiStyleVar_WindowPadding, { GImGui->Style.WindowPadding.x, 11 ds } );
	BeginChild( temp, vec2{ 0, 36 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground );
	{
		for ( int i = 0; i < columns.size( ); ++i ) {
			colsizes[i] = ImMax( CalcTextSize( columns[i].translate( ).data( ) ).x, colsizes[i] );
			Text( columns[i].translate( ).data( ) );

			if ( i == columns.size( ) - 2 ) {
				SameLine( GetWindowWidth( ) - GImGui->Style.WindowPadding.x - CalcTextSize( columns[i].translate( ).data( ) ).x );
			} else {
				SameLine( 0, colsizes[i] - CalcTextSize( columns[i].translate( ).data( ) ).x + gap ds );
			}
		}
	}
	EndChild( );
	PopStyleVar( );
	widgets->spacing( 0 );

	ImFormatString( temp, sizeof( temp ), "%s content", str_id );
	BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border );
}
void c_widgets::widget_table::end( )
{
	EndChild( );
	EndChild( );
	currow = 0;
}

void c_widgets::widget_table::beginrow( )
{
	char temp[128];
	ImFormatString( temp, sizeof( temp ), "%s%d", id, currow );
	BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
}
void c_widgets::widget_table::endrow( )
{
	EndChild( );
	currow++;
	curcol = 0;
	colpos = 0;
}

void c_widgets::widget_table::begincolumn( float w )
{
	char temp[128];
	ImFormatString( temp, sizeof( temp ), "%s%d%d", id, currow, curcol );
	BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
	if ( curcol == columns.size( ) - 1 && w < colsizes[curcol] ) {
		SetCursorPosX( colsizes[curcol] - w );
	}
}
void c_widgets::widget_table::endcolumn( )
{
	colsizes[curcol] = ImMax( colsizes[curcol], GetCurrentWindow( )->ContentSize.x );
	float cellw = GetCurrentWindow( )->ContentSize.x;
	EndChild( );
	if ( curcol == columns.size( ) - 2 ) {
		SameLine( GetWindowWidth( ) - colsizes[curcol + 1] );
	} else {
		SameLine( 0, colsizes[curcol] - cellw + gap ds );
	}
	curcol++;
}

void c_widgets::widget_table::setcolumnsizes( const std::vector< float >& sizes )
{
	for ( int i = 0; i < sizes.size( ); ++i ) {
		colsizes[i] = sizes[i];
	}
}

// TABBAR

c_nav& c_widgets::c_tabbar::draw( const char* str_id, const std::vector< const char* > list )
{
	PushStyleColor( ImGuiCol_ChildBg, g_style->col( pcol_bg3 ).vec4( ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 2, 2 } );
	BeginChild( str_id, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding );
	{
		for ( int i = 0; i < list.size( ); ++i ) {
			if ( tab( list[i], nav.next == i ) && nav.next != i ) {
				nav.next = i;
				nav.animdest = 0;
			}
			SameLine( 0, 2 ds );
		}
	}
	EndChild( );
	PopStyleVar( );
	PopStyleColor( );

	nav.handle( );
	return nav;
}
bool c_widgets::c_tabbar::tab( const ptext& label, bool selected )
{
	ImGuiWindow* window = GetCurrentWindow( );
	bool pressed = InvisibleButton( label.translate( ).data( ), CalcTextSize( label.translate( ).data( ), 0, 1 ) + GImGui->Style.FramePadding * 2 );
	bool hovered = IsItemHovered( ), held = IsItemActive( );
	ImRect bb = GImGui->LastItemData.Rect;

	struct s {
		float anim = 0;
		float hover = 0;
		float selected = 0;
	}; auto& obj = anim_obj( label.translate( ).data( ), 123444320, s{ } );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg2, obj.selected ), GImGui->Style.FrameRounding );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text2 ), obj.hover ), g_style->col( pcol_text ), obj.selected );
	window->DrawList->AddText( bb.Min + GImGui->Style.FramePadding, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	return pressed;
}


bool c_widgets::config( const char* name, const bool& selected, const ImVec2& size_arg ) {
    auto* window = GetCurrentWindow( );
    
    struct s {
        float hover;
        float selected;
    }; auto& obj = anim_obj( name, 14232221, s{ } );
    
    bool pressed = InvisibleButton( name, CalcItemSize( size_arg, 358, 28 ) );
    bool hovered = IsItemHovered( );
    ImRect& bb = GImGui->LastItemData.Rect;
    
    obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
    obj.selected = anim( obj.selected, 0.f, 1.f, selected );
    
    window->DrawList->AddRectFilled( bb.Min, bb.Max, col_anim( col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.hover ), g_style->col( pcol_bg4 ), obj.selected ), 3 ds );
    window->DrawList->AddRect( bb.Min, bb.Max, pcolor{ 255, 255, 255, 0.02f }, 3 ds );
    g_draw->text( icons, 12, bb.Min + vec2{ 338, 8 }, col_anim( pcolor{ 229, 69, 69, 0.00f }, g_style->col( pcol_scheme ), obj.selected ), I_CHECK );
    g_draw->text( icons, 12, bb.Min + vec2{ 328, 8 } + vec2{ 10, 0 } * obj.hover, col_anim( col_anim( pcolor{ 77, 79, 84, 0.00f }, pcolor{ 77, 79, 84, 1.00f }, obj.hover ), pcolor{ 77, 79, 84, 0.00f }, obj.selected ), I_CHEVRON__RIGHT );
    window->DrawList->AddText( bb.Min + vec2{ 8, 6 }, col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text ), obj.hover ), g_style->col( pcol_text ), obj.selected ), name );
    
    
    return pressed;
}


