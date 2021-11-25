#pragma once
#include "Engine.h"
#include <memory>
#include <unordered_map>
#include "PizdaVector.h"
#include "PizdaFont.h"

class CBasePlayer;
struct UVTable;
typedef struct D3DTLVERTEX
{
	float x;
	float y;
	float z;
	D3DCOLOR dxColor;
	float tu;
	float tv;
} *PD3DTLVERTEX;

struct PizdaDrawCmd {
	DWORD														_VertexBufferSize;
	DWORD														_IndexBufferSize;
	LPDIRECT3DTEXTURE9											_TexturePtr;
	RECT														_ScissorRect = { -8192,-8192,8192,8192 };
};

class PizdaDrawList {
public:
	PizdaVector<PizdaDrawCmd>									commands;
	PizdaVector<DWORD>											IndexBuffer;
	PizdaVector<D3DTLVERTEX>									VertexBuffer;
	PDWORD														_IdxPtr;
	PD3DTLVERTEX												_VtxPtr;
	//vector<PizdaDrawCommand>									DrawCommands;
	PizdaDrawList()												{ Initialize(); Clear(); }
	~PizdaDrawList()											{ Clear(); }
	void Clear() { commands.clear(); IndexBuffer.clear(); VertexBuffer.clear(); DrawListVTXSize = DrawListIDXSize = 0UL; }
	operator bool() { return DrawListIDXSize && DrawListVTXSize; }
	void AddDrawCommand(const PizdaDrawCmd& _cmd) { commands.push_back(_cmd); DrawListIDXSize += _cmd._IndexBufferSize; DrawListVTXSize += _cmd._VertexBufferSize; }
	void AddLine(Vector2D p1, Vector2D p2, PizdaColor color, float thickness = 1.f);
	void AddMultipointLine(vector<Vector2D>& points, int pNumber, bool cycled, PizdaColor color, float thickness = 1.f);
	void AddRectangle(Vector2D position, Vector2D size, PizdaColor color);
	void AddRectangleUV(Vector2D position, Vector2D size, PizdaColor color, LPDIRECT3DTEXTURE9 texture, UVTable uv,RECT _scissor = { -8192,-8192,8192,8192 });
	void AddRectangleFilled(Vector2D p1, Vector2D sz, PizdaColor color);
	void AddCoalBox(Vector2D start, Vector2D size, PizdaColor color);
	void AddOutlineBox(Vector2D start, Vector2D size, PizdaColor color);
	void AddOutlineCoalBox(Vector2D start, Vector2D size, PizdaColor color);
	void AddWindow(Vector2D start, Vector2D size, PizdaColor Border, PizdaColor Background);
	void AddHorBar(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float max = 100.f);
	void AddVerBar(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float max = 100.f);
	void AddHorBarWithSeps(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float seps, float max = 100.f);
	void AddVerBarWithSeps(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float seps, float max = 100.f);
	void AddGradientCircle(Vector2D start, float radius, PizdaColor c1, PizdaColor c2);
	void AddCircle(Vector2D center, float radius, PizdaColor color);
	void AddGradientSquare(Vector2D start, Vector2D size, PizdaColor c1, PizdaColor c2);
	void AddGradientSquare(Vector2D start, Vector2D size, PizdaColor c1, PizdaColor c2, PizdaColor c3, PizdaColor c4);
	void AddGradientTriangle(vector<Vector2D>& points, PizdaColor c1, PizdaColor c2);
	void AddGradientTriangle(vector<Vector2D>& points, PizdaColor c1, PizdaColor c2, PizdaColor c3);
	void AddQuarterCircle(Vector2D start, float radius, float quart, PizdaColor c);
	void AddRoundedBox(Vector2D start, Vector2D size, float smooth, PizdaColor c1, bool outline, bool filled, PizdaColor fill_col);
	float AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, const char* format, ...);
	float AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, float scale, const char* format, ...);
	float AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, const wchar_t* format, ...);
	float AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, float scale, const wchar_t* format, ...);
	float AddText_NF(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, wstring text);
	float AddText_NF(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, float scale, wstring text);
	void AddImage(Vector2D start, Vector2D size, string name, PizdaColor color);
	void AddImage(Vector2D start, Vector2D size, short name, PizdaColor color);
	void AddCursor(int type, Vector2D pos);
	void MemoryReserve(int idx, int vtx);
	void Initialize();
	//void Add3DCube(float scalarx, float scalary, QAngle angles, Vector middle_origin, PizdaColor outle, bool outline = false);
	array<Vector, 3> Add3DCube(CBasePlayer* pEntity, PizdaColor c1, bool b_outline = false, bool filled = false, PizdaColor fill_col = Color::White());
	//void AddSquare(Vector2D start,Vector2D size, PizdaColor color);
	DWORD DrawListVTXSize, DrawListIDXSize;
	vector<Vector2D> PreCalcCircle;
};

