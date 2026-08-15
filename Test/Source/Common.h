#pragma once

#include <wx/defs.h>

enum
{
	ID_Exit = wxID_HIGHEST + 1,
	ID_Timer,
	ID_Generate,
	ID_Clear,
	ID_Save,
	ID_Load,
	ID_DRAW_SPLINES,
	ID_DRAW_POLYGONS,
	ID_DRAW_NODE_POINTS,
	ID_DRAW_WIREFRAME,
	ID_GenerateImzadiAsset,
};

#define DF_SPLINES					0x00000001
#define DF_POLYGONS					0x00000002
#define DF_NODE_POINTS				0x00000004
#define DF_WIREFRAME				0x00000008