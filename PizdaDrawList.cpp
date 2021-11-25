#include "Render.h"
#pragma region PizdaDrawList

static const Vector2D CURSOR_COORDINATES[8][3] =
{
	{ Vector2D(0,0),  Vector2D(2,2), Vector2D(0, 0) },
{ Vector2D(0,3),  Vector2D(12,19), Vector2D(0, 0) }, // ImGuiMouseCursor_Arrow
{ Vector2D(13,0), Vector2D(7,16),  Vector2D(4, 8) }, // ImGuiMouseCursor_TextInput
{ Vector2D(31,0), Vector2D(23,23), Vector2D(11,11) }, // ImGuiMouseCursor_ResizeAll
{ Vector2D(21,0), Vector2D(9,23), Vector2D(5,11) }, // ImGuiMouseCursor_ResizeNS
{ Vector2D(55,18),Vector2D(23, 9), Vector2D(11, 5) }, // ImGuiMouseCursor_ResizeEW
{ Vector2D(73,0), Vector2D(17,17), Vector2D(9, 9) }, // ImGuiMouseCursor_ResizeNESW
{ Vector2D(55,0), Vector2D(17,17), Vector2D(9, 9) }, // ImGuiMouseCursor_ResizeNWSE
};

void PizdaDrawList::Initialize() {
	for (int i = 0; i < 12; i++) {
		const float j = (float(i * 2 * M_PI) / 12.f);
		PreCalcCircle.push_back(Vector2D(cosf(j), sinf(j)));
	}
}

void PizdaDrawList::MemoryReserve(int idx_count, int vtx_count)
{
	//PizdaDrawCmd& draw_cmd = CmdBuffer.Data[CmdBuffer.Size - 1];
	//draw_cmd.ElemCount += idx_count;
	/*
	PizdaVector<PizdaDrawCmd>									commands;
	PizdaVector<DWORD>											IndexBuffer;
	PizdaVector<D3DTLVERTEX>									VertexBuffer;
	PDWORD														_IdxPtr;
	PD3DTLVERTEX												_VtxPtr;
	*/
	int vtx_buffer_old_size = VertexBuffer.Size;
	VertexBuffer.resize(vtx_buffer_old_size + vtx_count);
	_VtxPtr = VertexBuffer.Data + vtx_buffer_old_size;

	int idx_buffer_old_size = IndexBuffer.Size;
	IndexBuffer.resize(idx_buffer_old_size + idx_count);
	_IdxPtr = IndexBuffer.Data + idx_buffer_old_size;
}

void PizdaDrawList::AddCursor(int type, Vector2D pos)
{
	pos.x -= CURSOR_COORDINATES[type][2].x;
	pos.y -= CURSOR_COORDINATES[type][2].y;
	AddRectangleUV(pos, CURSOR_COORDINATES[type][1], Color::White(), CRender::Get().CursorsTexture, CRender::Get().MiscUvs[type]);
	//AddRectangleUV(Vector2D(100,100), Vector2D(900, 270), Color::White(), CRender::Get().CursorsTexture, UVTable(0,0,1,1));
}

array<Vector, 3> PizdaDrawList::Add3DCube(CBasePlayer* pEntity, PizdaColor c1, bool b_outline, bool filled, PizdaColor fill_col) {
	if (!pEntity)return array<Vector, 3>();
	ICollideable* coll = pEntity->GetCollideable();

	static auto Rotate2D = [](const float &f, Vector& vec)
	{
		float _x, _y;

		float s, c;

		s = sin(DEG2RAD(f));
		c = cos(DEG2RAD(f));

		_x = vec.x;
		_y = vec.y;

		vec.x = (_x * c) - (_y * s);
		vec.y = (_x * s) + (_y * c);
	};

	Vector min = coll->OBBMins();
	Vector max = coll->OBBMaxs();

	Vector corners[8] =
	{
		Vector(min.x,min.y,min.z),
		Vector(min.x,max.y,min.z),
		Vector(max.x,max.y,min.z),
		Vector(max.x,min.y,min.z),
		Vector(min.x,min.y,max.z + 20),
		Vector(min.x,max.y,max.z + 20),
		Vector(max.x,max.y,max.z + 20),
		Vector(max.x,min.y,max.z + 20)
	};

	float ang = pEntity->m_angEyeAngles().x;

	for (int i = 0; i <= 7; i++)
		Rotate2D(ang, corners[i]);

	Vector _corners[8];
	vector<Vector2D> pts;
	pts.resize(16, Vector2D());
	int j = 0;
	for (int i = 0; i <= 7; i++)
		if (!WorldToScreen(pEntity->GetRenderOrigin() + corners[i], _corners[i]))return array<Vector, 3>();
	for (int i = 1; i <= 4; i++)
	{
		if (!b_outline) {
			pts[j++] = Vector2D((_corners[i % 4].x), (_corners[i % 4].y));
			pts[j++] = Vector2D((_corners[i - 1].x), (_corners[i - 1].y));
			pts[j++] = Vector2D((_corners[i + 3].x), (_corners[i + 3].y));
			pts[j++] = Vector2D((_corners[i % 4 + 4].x), (_corners[i % 4 + 4].y));
		}
		if (filled) {
			vector<Vector2D> points = { Vector2D(_corners[i - 1].x, _corners[i - 1].y),Vector2D(_corners[i % 4 + 4].x, _corners[i % 4 + 4].y),Vector2D(_corners[i].x, _corners[i].y) };
			AddGradientTriangle(points, fill_col, fill_col);
			vector<Vector2D> points1 = { Vector2D(_corners[i - 1].x, _corners[i - 1].y),Vector2D(_corners[i + 3].x, _corners[i + 3].y),Vector2D(_corners[i % 4 + 4].x, _corners[i % 4 + 4].y) };
			AddGradientTriangle(points1, fill_col, fill_col);
		}
	}
	if (filled) {
		vector<Vector2D> points = { Vector2D(_corners[0].x, _corners[0].y),Vector2D(_corners[3].x, _corners[3].y), Vector2D(_corners[4].x, _corners[4].y) };
		AddGradientTriangle(points, fill_col, fill_col);
	}
	AddMultipointLine(pts, j, true, c1, 1);
	static array<Vector, 3> result;
	result[0] = Vector(_corners[5]);
	result[1] = Vector(_corners[6]);
	result[2] = Vector(_corners[1]);
	return result;
}

void PizdaDrawList::AddRectangleUV(Vector2D position, Vector2D size, PizdaColor color, LPDIRECT3DTEXTURE9 texture, UVTable uv,RECT _scissor) {
	//PizdaDrawCommand _cmd;
	DWORD col = D3DCOLOR_RGBA((int)color.r, (int)color.g, (int)color.b, (int)color.a);
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[4];
	//_cmd._IndexBufferPtr = new DWORD[6];
	//_cmd._TexturePtr = texture;
	MemoryReserve(6, 4);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 6;
	cmd._VertexBufferSize = 4;
	cmd._TexturePtr = texture;
	/*
	0	1
	*****
	**  *
	* * *
	*  **
	*****
	3	2
	*/
	cmd._ScissorRect = _scissor;
	_VtxPtr[0].x = position.x; _VtxPtr[0].y = position.y; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = col; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = position.x + size.x; _VtxPtr[1].y = position.y; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = col; _VtxPtr[1].tu = uv.u2; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = position.x + size.x; _VtxPtr[2].y = position.y + size.y; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = col; _VtxPtr[2].tu = uv.u2; _VtxPtr[2].tv = uv.v2;
	_VtxPtr[3].x = position.x; _VtxPtr[3].y = position.y + size.y; _VtxPtr[3].z = 0; _VtxPtr[3].dxColor = col; _VtxPtr[3].tu = uv.u1; _VtxPtr[3].tv = uv.v2;
	//_cmd._VertexBufferSize = 4;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 2;
	_IdxPtr[3] = 0; _IdxPtr[4] = 2; _IdxPtr[5] = 3;
	AddDrawCommand(cmd);
	//_cmd._IndexBufferSize = 6;
	//AddDrawCommand(_cmd);
}

void PizdaDrawList::AddImage(Vector2D start, Vector2D size, string name, PizdaColor color) {
	if (CRender::Get().wtex_map.count(fnv::hash_runtime(name.c_str()))) {
		auto member = CRender::Get().wtex_map.at(fnv::hash_runtime(name.c_str()));
		D3DSURFACE_DESC desc;
		member.first->GetLevelDesc(0, &desc);
		if (size.x <= 1)size.x *= desc.Width;
		if (size.y <= 1)size.y *= desc.Height;
		if (member.first != nullptr) {
			AddRectangleUV(start, size, color, member.first, UVTable(0, 0, 1, 1));
		}
		if (member.second != nullptr) {
			AddRectangleUV(start, size, color, member.second, UVTable(0, 0, 1, 1));
		}
	}
}

void PizdaDrawList::AddImage(Vector2D start, Vector2D size, short name, PizdaColor color) {
	if (CRender::Get().wtex_map.count((uint32_t)name)) {
		auto member = CRender::Get().wtex_map.at((uint32_t)name);
		D3DSURFACE_DESC desc;
		member.first->GetLevelDesc(0, &desc);
		if (size.x <= 1)size.x *= desc.Width * 0.5f;
		if (size.y <= 1)size.y *= desc.Height * 0.5f;
		if (size.x <= 0)return;
		if (size.y <= 0)return;
		if (member.first != nullptr) {
			AddRectangleUV(start, size, color, member.first, UVTable(0, 0, 1, 1));
		}
		if (member.second != nullptr) {
			AddRectangleUV(start, size, color, member.second, UVTable(0, 0, 1, 1));
		}
	}
}

void PizdaDrawList::AddLine(Vector2D p1, Vector2D p2, PizdaColor color, float thickness) {
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//PizdaDrawCommand _cmd;
	DWORD col = D3DCOLOR_RGBA((int)color.r, (int)color.g, (int)color.b, (int)color.a);
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[4];
	//_cmd._IndexBufferPtr = new DWORD[6];
	MemoryReserve(6, 4);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 6;
	cmd._VertexBufferSize = 4;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	auto difference = p2 - p1;
	difference *= difference.IsZero() ? 1.f : (1.f / difference.Length());
	float dx = difference.x *(thickness * 0.5f);
	float dy = difference.y *(thickness * 0.5f);
	_VtxPtr[0].x = p1.x + dy; _VtxPtr[0].y = p1.y - dx; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = col; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = p2.x + dy; _VtxPtr[1].y = p2.y - dx; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = col; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = p2.x - dy; _VtxPtr[2].y = p2.y + dx; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = col; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1;
	_VtxPtr[3].x = p1.x - dy; _VtxPtr[3].y = p1.y + dx; _VtxPtr[3].z = 0; _VtxPtr[3].dxColor = col; _VtxPtr[3].tu = uv.u1; _VtxPtr[3].tv = uv.v1;
	//_cmd._VertexBufferSize = 4;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 2;
	_IdxPtr[3] = 0; _IdxPtr[4] = 2; _IdxPtr[5] = 3;
	AddDrawCommand(cmd);
	//_cmd._IndexBufferSize = 6;
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	//AddDrawCommand(_cmd);
}

void PizdaDrawList::AddMultipointLine(vector<Vector2D>& points, int pNumber, bool cycled, PizdaColor color, float thickness) {
	if (pNumber < 2)
		return;
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;

	auto count = cycled ? pNumber : pNumber - 1;
	const auto AA = 1.0f;
	const auto col = D3DCOLOR_RGBA((int)color.r, (int)color.g, (int)color.b, (int)color.a);
	///*
	const auto trans_col = D3DCOLOR_RGBA((int)color.r, (int)color.g, (int)color.b, (int)0);
	const auto thick = thickness > 1.f;
	const auto indexies = thick ? count * 18 : count * 12;
	const auto vertecies = thick ? pNumber * 4 : pNumber * 3;
	//PizdaDrawCommand _cmd;
	MemoryReserve(indexies, vertecies);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = indexies;
	cmd._VertexBufferSize = vertecies;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[vertecies];
	//_cmd._IndexBufferPtr = new DWORD[indexies];
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	//auto temp_aas = new Vector2D[pNumber * (thick ? 5 : 3)];
	//_cmd._IndexBufferSize = 0;
	//_cmd._VertexBufferSize = 0;
	auto normals = new Vector2D[pNumber];
	auto pts = new Vector2D[pNumber * (thick ? 4 : 2)];
	for (int i = 0; i < count; i++) {
		int i_end = (i + 1) == pNumber ? 0 : i + 1;
		Vector2D difference = points[i_end] - points[i];
		difference *= difference.IsZero() ? 1.f : (1.f / difference.Length());
		normals[i].x = difference.y;
		normals[i].y = -difference.x;
	}
	if (!cycled)
		normals[pNumber - 1] = normals[pNumber - 2];

	if (!thick)
	{
		if (!cycled) {
			pts[0] = points[0] + normals[0] * AA;
			pts[1] = points[0] - normals[0] * AA;
			pts[(pNumber - 1) * 2] = points[pNumber - 1] + normals[pNumber - 1] * AA;
			pts[(pNumber - 1) * 2 + 1] = points[pNumber - 1] - normals[pNumber - 1] * AA;
		}
		auto id1 = 0;
		for (int i = 0; i < count; i++)
		{
			const auto i_end = (i + 1) == pNumber ? 0 : i + 1;
			auto id2 = (i + 1) == pNumber ? 0 : id1 + 3;
			auto dm = (normals[i] + normals[i_end]) * 0.5f;
			auto dmr = dm.x * dm.x + dm.y * dm.y;
			if (dmr > 0.000001f)
			{
				float scale = 1.0f / dmr;
				if (scale > 100.f)scale = 100.f;
				dm *= scale;
			}
			dm *= AA;
			pts[i_end * 2] = points[i_end] + dm;
			pts[i_end * 2 + 1] = points[i_end] - dm;
			_IdxPtr[0] = (id2 + 0); _IdxPtr[1] = (id1 + 0); _IdxPtr[2] = (id1 + 2);
			_IdxPtr[3] = (id1 + 2); _IdxPtr[4] = (id2 + 2); _IdxPtr[5] = (id2 + 0);
			_IdxPtr[6] = (id2 + 1); _IdxPtr[7] = (id1 + 1); _IdxPtr[8] = (id1 + 0);
			_IdxPtr[9] = (id1 + 0); _IdxPtr[10] = (id2 + 0); _IdxPtr[11] = (id2 + 1);
			_IdxPtr += 12;
			id1 = id2;
		}
		for (int i = 0; i < pNumber; i++)
		{
			_VtxPtr[0].x = points[i].x; _VtxPtr[0].y = points[i].y; _VtxPtr[0].z = 0.f; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1; _VtxPtr[0].dxColor = col;
			_VtxPtr[1].x = pts[i * 2].x; _VtxPtr[1].y = pts[i * 2].y; _VtxPtr[1].z = 0.f; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1; _VtxPtr[1].dxColor = trans_col;
			_VtxPtr[2].x = pts[i * 2 + 1].x; _VtxPtr[2].y = pts[i * 2 + 1].y; _VtxPtr[2].z = 0.f; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1; _VtxPtr[2].dxColor = trans_col;
			_VtxPtr += 3;
		}
	}
	else {
		const auto half_thickness = (thickness - AA)*0.5f;
		if (!cycled) {
			pts[0] = points[0] + normals[0] * (half_thickness + AA);
			pts[1] = points[0] + normals[0] * (half_thickness);
			pts[2] = points[0] - normals[0] * (half_thickness);
			pts[3] = points[0] - normals[0] * (half_thickness + AA);
			pts[(pNumber - 1) * 4] = points[pNumber - 1] + normals[pNumber - 1] * (half_thickness + AA);
			pts[(pNumber - 1) * 4 + 1] = points[pNumber - 1] + normals[pNumber - 1] * (half_thickness);
			pts[(pNumber - 1) * 4 + 2] = points[pNumber - 1] - normals[pNumber - 1] * (half_thickness);
			pts[(pNumber - 1) * 4 + 3] = points[pNumber - 1] - normals[pNumber - 1] * (half_thickness + AA);
		}
		auto id1 = 0;
		for (int i = 0; i < count; i++) {
			const auto i_end = (i + 1) == pNumber ? 0 : i + 1;
			auto id2 = (i + 1) == pNumber ? 0 : id1 + 4;
			auto dm = (normals[i] + normals[i_end]) * 0.5f;
			float dmr = dm.x*dm.x + dm.y*dm.y;
			if (dmr > 0.000001f) {
				auto scale = 1.0f / dmr;
				if (scale > 100.f)scale = 100.f;
				dm *= scale;
			}
			auto dm_out = dm * (half_thickness + AA);
			auto dm_in = dm * half_thickness;
			pts[i_end * 4] = points[i_end] + dm_out;
			pts[i_end * 4 + 1] = points[i_end] + dm_in;
			pts[i_end * 4 + 2] = points[i_end] - dm_in;
			pts[i_end * 4 + 3] = points[i_end] - dm_out;

			_IdxPtr[0] = (id2 + 1); _IdxPtr[1] = (id1 + 1); _IdxPtr[2] = (id1 + 2);
			_IdxPtr[3] = (id1 + 2); _IdxPtr[4] = (id2 + 2); _IdxPtr[5] = (id2 + 1);
			_IdxPtr[6] = (id2 + 1); _IdxPtr[7] = (id1 + 1); _IdxPtr[8] = (id1 + 0);
			_IdxPtr[9] = (id1 + 0); _IdxPtr[10] = (id2 + 0); _IdxPtr[11] = (id2 + 1);
			_IdxPtr[12] = (id2 + 2); _IdxPtr[13] = (id1 + 2); _IdxPtr[14] = (id1 + 3);
			_IdxPtr[15] = (id1 + 3); _IdxPtr[16] = (id2 + 3); _IdxPtr[17] = (id2 + 2);
			_IdxPtr += 18;
			id1 = id2;
		}
		for (int i = 0; i < pNumber; i++)
		{
			_VtxPtr[0].x = pts[i * 4].x; _VtxPtr[0].y = pts[i * 4].y; _VtxPtr[0].z = 0.f; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1; _VtxPtr[0].dxColor = trans_col;
			_VtxPtr[1].x = pts[i * 4 + 1].x; _VtxPtr[1].y = pts[i * 4 + 1].y; _VtxPtr[1].z = 0.f; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1; _VtxPtr[1].dxColor = col;
			_VtxPtr[2].x = pts[i * 4 + 2].x; _VtxPtr[2].y = pts[i * 4 + 2].y; _VtxPtr[2].z = 0.f; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1; _VtxPtr[2].dxColor = col;
			_VtxPtr[3].x = pts[i * 4 + 3].x; _VtxPtr[3].y = pts[i * 4 + 3].y; _VtxPtr[3].z = 0.f; _VtxPtr[3].tu = uv.u1; _VtxPtr[3].tv = uv.v1; _VtxPtr[3].dxColor = trans_col;
			_VtxPtr += 4;
		}
	}
	AddDrawCommand(cmd);
	delete[] normals;
	delete[] pts;
	//AddDrawCommand(_cmd);
}

void PizdaDrawList::AddRectangle(Vector2D position, Vector2D size, PizdaColor color) {
	vector<Vector2D> temp_vec = { position,Vector2D(position.x + size.x, position.y),Vector2D(position.x + size.x, position.y + size.y), Vector2D(position.x, position.y + size.y) };
	AddMultipointLine(temp_vec, 4, true, color);
}

void PizdaDrawList::AddRectangleFilled(Vector2D p1, Vector2D sz, PizdaColor color) {
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//PizdaDrawCommand _cmd;
	DWORD col = D3DCOLOR_RGBA((int)color.r, (int)color.g, (int)color.b, (int)color.a);
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[4];
	//_cmd._IndexBufferPtr = new DWORD[6];
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	MemoryReserve(6, 4);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 6;
	cmd._VertexBufferSize = 4;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	/*

	0	1
	*****
	**  *
	* * *
	*  **
	*****
	3	2

	*/
	_VtxPtr[0].x = p1.x; _VtxPtr[0].y = p1.y; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = col; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = p1.x + sz.x; _VtxPtr[1].y = p1.y; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = col; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = p1.x + sz.x; _VtxPtr[2].y = p1.y + sz.y; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = col; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1;
	_VtxPtr[3].x = p1.x; _VtxPtr[3].y = p1.y + sz.y; _VtxPtr[3].z = 0; _VtxPtr[3].dxColor = col; _VtxPtr[3].tu = uv.u1; _VtxPtr[3].tv = uv.v1;
	//_cmd._VertexBufferSize = 4;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 2;
	_IdxPtr[3] = 0; _IdxPtr[4] = 2; _IdxPtr[5] = 3;
	AddDrawCommand(cmd);
	//_cmd._IndexBufferSize = 6;
	//AddDrawCommand(_cmd);
}

void PizdaDrawList::AddCoalBox(Vector2D start, Vector2D size, PizdaColor color) {
	float iw = size.x / 4;
	float ih = size.y / 4;
	vector<Vector2D> top_left = { Vector2D(start.x,start.y + ih),Vector2D(start.x,start.y),Vector2D(start.x + iw,start.y) };
	vector<Vector2D> top_right = { Vector2D(start.x + size.x - iw,start.y), Vector2D(start.x + size.x,start.y) ,Vector2D(start.x + size.x,start.y + ih) };
	vector<Vector2D> bottom_left = { Vector2D(start.x,start.y + size.y - ih),Vector2D(start.x,start.y + size.y),Vector2D(start.x + iw,start.y + size.y) };
	vector<Vector2D> bottom_right = { Vector2D(start.x + size.x - iw,start.y + size.y), Vector2D(start.x + size.x,start.y + size.y) ,Vector2D(start.x + size.x,start.y + size.y - ih) };
	AddMultipointLine(top_left, 3, false, color, 1);
	AddMultipointLine(top_right, 3, false, color, 1);
	AddMultipointLine(bottom_left, 3, false, color, 1);
	AddMultipointLine(bottom_right, 3, false, color, 1);
}

void PizdaDrawList::AddOutlineBox(Vector2D start, Vector2D size, PizdaColor color) {
	AddRectangle(start, size, PizdaColor(Color::Black()));
	start += 1;
	size -= 2;
	AddRectangle(start, size, color);
	start += 1;
	size -= 2;
	AddRectangle(start, size, PizdaColor(Color::Black()));
}

void PizdaDrawList::AddOutlineCoalBox(Vector2D start, Vector2D size, PizdaColor color) {
	float iw = size.x / 4;
	float ih = size.y / 4;
	AddCoalBox(start, size, PizdaColor(Color::Black()));
	AddCoalBox(Vector2D(start.x + 1, start.y + 1), Vector2D(size.x - 2, size.y - 2), color);
	AddCoalBox(Vector2D(start.x + 2, start.y + 2), Vector2D(size.x - 4, size.y - 4), PizdaColor(Color::Black()));
	AddLine(Vector2D(start.x, start.y + ih), Vector2D(start.x + 2, start.y + ih), Color::Black());
	AddLine(Vector2D(start.x + size.x, start.y + ih), Vector2D(start.x - 2 + size.x, start.y + ih), Color::Black());
	AddLine(Vector2D(start.x, start.y + size.y - ih), Vector2D(start.x + 2, start.y + size.y - ih), Color::Black());
	AddLine(Vector2D(start.x + size.x, start.y + size.y - ih), Vector2D(start.x - 2 + size.x, start.y + size.y - ih), Color::Black());

	AddLine(Vector2D(start.x + iw, start.y), Vector2D(start.x + iw, start.y + 2), Color::Black());
	AddLine(Vector2D(start.x + size.x - iw, start.y + 2), Vector2D(start.x + size.x - iw, start.y), Color::Black());
	AddLine(Vector2D(start.x + iw, start.y + size.y - 2), Vector2D(start.x + iw, start.y + size.y), Color::Black());
	AddLine(Vector2D(start.x + size.x - iw, start.y + size.y - 2), Vector2D(start.x + size.x - iw, start.y + size.y), Color::Black());
}

void PizdaDrawList::AddWindow(Vector2D start, Vector2D size, PizdaColor Border, PizdaColor Background) {
	AddRectangle(start, size, Border);
	start += 1;
	size -= 2;
	AddRectangleFilled(start, size, Background);
}

void PizdaDrawList::AddHorBar(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float max) {
	if (val > max) val = max;
	float sz = (size.x * val) / max;

	AddRectangle(start, size, Color::Black());
	AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(size.x - 2, size.y - 2), color2);

	if (sz == size.x)
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(sz - 2, size.y - 2), color1);
	else
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(sz, size.y - 2), color1);
}

void PizdaDrawList::AddVerBar(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float max) {
	if (val > max) val = max;
	float sz = (size.y * val) / max;

	AddRectangle(start, size, Color(0, 0, 0));
	AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(size.x - 2, size.y - 2), color2);

	if (sz == size.y)
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1 + size.y - sz), Vector2D(size.x - 2, sz - 2), color1);
	else
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1 + size.y - sz), Vector2D(size.x - 2, sz), color1);
}

void PizdaDrawList::AddHorBarWithSeps(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float seps, float max) {
	if (max <= 0)return;
	if (val > max) val = max;
	float sz = (size.x * val) / max;
	if (seps == 0) seps = 1;
	if (size.x <= seps)seps = size.x / 2;
	float sepStep = size.x / seps;

	AddRectangle(start, size, Color::Black());
	AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(size.x - 2, size.y - 2), color2);

	if (sz == size.x)
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(sz - 2, size.y - 2), color1);
	else
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(sz, size.y - 2), color1);
	for (float i = start.x; i < start.x + size.x; i += sepStep) {
		if (i >= start.x + sz)
			AddLine(Vector2D(i, start.y), Vector2D(i, start.y + size.y), Color::Black());
	}
}

void PizdaDrawList::AddVerBarWithSeps(Vector2D start, Vector2D size, float val, PizdaColor color1, PizdaColor color2, float seps, float max) {
	if (max <= 0)return;
	if (val > max) val = max;
	float sz = (size.y * val) / max;
	if (seps == 0) seps = 1;
	if (size.y <= seps)seps = size.y / 2;
	float sepStep = size.y / seps;

	AddRectangle(start, size, Color(0, 0, 0));
	AddRectangleFilled(Vector2D(start.x + 1, start.y + 1), Vector2D(size.x - 2, size.y - 2), color2);

	if (sz == size.y)
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1 + size.y - sz), Vector2D(size.x - 2, sz - 2), color1);
	else
		AddRectangleFilled(Vector2D(start.x + 1, start.y + 1 + size.y - sz), Vector2D(size.x - 2, sz), color1);
	for (float i = start.y; i < start.y + size.y; i += sepStep) {
		if (i >= start.y + 1 + size.y - sz)
			AddLine(Vector2D(start.x, i), Vector2D(start.x + size.x, i), Color::Black());
		AddLine(Vector2D(start.x, i), Vector2D(start.x + size.x, i + 1), Color::Black());
	}
}

void PizdaDrawList::AddCircle(Vector2D start, float radius, PizdaColor c1)
{
	static vector<Vector2D> points;
	points.clear();
	points.resize(60);
	int i = 0;
	float fDegree = 0;
	for (int h = 0; h < 4; h++)
	{
		for (float k = fDegree; k < fDegree + M_PI / 2; k += D3DXToRadian(9), i++)
		{
			float sin1 = sin(k);
			float cos1 = cos(k);
			points[i] = Vector2D(start.x + radius * cos1, start.y + radius * sin1);
		}
		fDegree += M_PI / 2;
	}
	AddMultipointLine(points, i, true, c1, 3);
}

void PizdaDrawList::AddGradientCircle(Vector2D start, float radius, PizdaColor c1, PizdaColor c2) {
	if (radius <= 0)
		return;
	DWORD col1 = D3DCOLOR_RGBA((int)c1.r, (int)c1.g, (int)c1.b, (int)c1.a);
	DWORD col2 = D3DCOLOR_RGBA((int)c2.r, (int)c2.g, (int)c2.b, (int)c2.a);
	//PizdaDrawCommand cmd;
	//cmd.Clear();
	MemoryReserve(123, 42);
	PizdaDrawCmd cmd;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	cmd._VertexBufferSize = 0;
	cmd._IndexBufferSize = 0;
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	int i = 1;
	_VtxPtr[0].x = start.x; _VtxPtr[0].y = start.y; _VtxPtr[0].z = 0.f; _VtxPtr[0].dxColor = col1; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	cmd._VertexBufferSize += 1;
	int id1 = i;
	for (float lat = 0; lat < M_PI * 2.0f + D3DXToRadian(4.5f); lat += D3DXToRadian(9), i++)
	{
		int id2 = i == 41 ? 1 : id1 + 1;
		float sin1 = sin(lat);
		float cos1 = cos(lat);
		_VtxPtr[i].x = start.x + radius * cos1;
		_VtxPtr[i].y = start.y + radius * sin1;
		_VtxPtr[i].z = 0.f;
		_VtxPtr[i].dxColor = col2;
		_VtxPtr[i].tu = uv.u1;
		_VtxPtr[i].tv = uv.v1;
		cmd._VertexBufferSize += 1;
		_IdxPtr[0] = 0;
		_IdxPtr[1] = id1;
		_IdxPtr[2] = id2;
		_IdxPtr += 3;
		cmd._IndexBufferSize += 3;
		id1 = id2;
	}
	AddDrawCommand(cmd);
}

void PizdaDrawList::AddGradientSquare(Vector2D start, Vector2D size, PizdaColor c1, PizdaColor c2) {
	DWORD dxBoxColor1 = D3DCOLOR_RGBA((int)c1.r, (int)c1.g, (int)c1.b, (int)c1.a);
	DWORD dxBoxColor2 = D3DCOLOR_RGBA((int)c2.r, (int)c2.g, (int)c2.b, (int)c2.a);
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//PizdaDrawCommand _cmd;
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[4];
	//_cmd._IndexBufferPtr = new DWORD[6];
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	MemoryReserve(6, 4);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 6;
	cmd._VertexBufferSize = 4;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	/*

	0	1
	*****
	**  *
	* * *
	*  **
	*****
	3	2

	*/
	_VtxPtr[0].x = start.x; _VtxPtr[0].y = start.y; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = dxBoxColor1; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = start.x + size.x; _VtxPtr[1].y = start.y; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = dxBoxColor1; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = start.x + size.x; _VtxPtr[2].y = start.y + size.y; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = dxBoxColor2; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1;
	_VtxPtr[3].x = start.x; _VtxPtr[3].y = start.y + size.y; _VtxPtr[3].z = 0; _VtxPtr[3].dxColor = dxBoxColor2; _VtxPtr[3].tu = uv.u1; _VtxPtr[3].tv = uv.v1;
	//_cmd._VertexBufferSize = 4;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 2;
	_IdxPtr[3] = 0; _IdxPtr[4] = 2; _IdxPtr[5] = 3;
	//_cmd._IndexBufferSize = 6;
	//AddDrawCommand(_cmd);
	AddDrawCommand(cmd);
}

void PizdaDrawList::AddGradientSquare(Vector2D start, Vector2D size, PizdaColor c1, PizdaColor c2, PizdaColor c3, PizdaColor c4) {
	DWORD dxBoxColor1 = D3DCOLOR_RGBA((int)c1.r, (int)c1.g, (int)c1.b, (int)c1.a);
	DWORD dxBoxColor2 = D3DCOLOR_RGBA((int)c2.r, (int)c2.g, (int)c2.b, (int)c2.a);
	DWORD dxBoxColor3 = D3DCOLOR_RGBA((int)c3.r, (int)c3.g, (int)c3.b, (int)c3.a);
	DWORD dxBoxColor4 = D3DCOLOR_RGBA((int)c4.r, (int)c4.g, (int)c4.b, (int)c4.a);
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//PizdaDrawCommand _cmd;
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[4];
	//_cmd._IndexBufferPtr = new DWORD[6];
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	MemoryReserve(6, 4);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 6;
	cmd._VertexBufferSize = 4;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	/*

	0	1
	*****
	**  *
	* * *
	*  **
	*****
	3	2

	*/
	_VtxPtr[0].x = start.x; _VtxPtr[0].y = start.y; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = dxBoxColor1; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = start.x + size.x; _VtxPtr[1].y = start.y; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = dxBoxColor2; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = start.x + size.x; _VtxPtr[2].y = start.y + size.y; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = dxBoxColor3; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1;
	_VtxPtr[3].x = start.x; _VtxPtr[3].y = start.y + size.y; _VtxPtr[3].z = 0; _VtxPtr[3].dxColor = dxBoxColor4; _VtxPtr[3].tu = uv.u1; _VtxPtr[3].tv = uv.v1;
	//_cmd._VertexBufferSize = 4;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 3;
	_IdxPtr[3] = 1; _IdxPtr[4] = 2; _IdxPtr[5] = 3;
	//_cmd._IndexBufferSize = 6;
	//AddDrawCommand(_cmd);
	AddDrawCommand(cmd);
}

void PizdaDrawList::AddGradientTriangle(vector<Vector2D>& points, PizdaColor c1, PizdaColor c2) {
	DWORD dxBoxColor1 = D3DCOLOR_RGBA((int)c1.r, (int)c1.g, (int)c1.b, (int)c1.a);
	DWORD dxBoxColor2 = D3DCOLOR_RGBA((int)c2.r, (int)c2.g, (int)c2.b, (int)c2.a);
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//PizdaDrawCommand _cmd;
	//_cmd._VertexBufferPtr = new D3DTLVERTEX[3];
	//_cmd._IndexBufferPtr = new DWORD[3];
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	MemoryReserve(3, 3);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 3;
	cmd._VertexBufferSize = 3;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	/*

	0	1
	*****
	**  *
	* * *
	*  **
	*****
	3	2

	*/
	_VtxPtr[0].x = points[1].x; _VtxPtr[0].y = points[1].y; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = dxBoxColor2; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = points[0].x; _VtxPtr[1].y = points[0].y; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = dxBoxColor1; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = points[2].x; _VtxPtr[2].y = points[2].y; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = dxBoxColor1; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1;
	//_cmd._VertexBufferSize = 3;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 2;
	//_cmd._IndexBufferSize = 3;
	//AddDrawCommand(_cmd);
	AddDrawCommand(cmd);
}

void PizdaDrawList::AddGradientTriangle(vector<Vector2D>& points, PizdaColor c1, PizdaColor c2, PizdaColor c3) {
	DWORD dxBoxColor1 = D3DCOLOR_RGBA((int)c1.r, (int)c1.g, (int)c1.b, (int)c1.a);
	DWORD dxBoxColor2 = D3DCOLOR_RGBA((int)c2.r, (int)c2.g, (int)c2.b, (int)c2.a);
	DWORD dxBoxColor3 = D3DCOLOR_RGBA((int)c3.r, (int)c3.g, (int)c3.b, (int)c3.a);
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//PizdaDrawCommand _cmd;
	//_VtxPtr = new D3DTLVERTEX[3];
	//_cmd._IndexBufferPtr = new DWORD[3];
	//_cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	MemoryReserve(3, 3);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 3;
	cmd._VertexBufferSize = 3;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	/*

	0	1
	*****
	**  *
	* * *
	*  **	PizdaDrawCommand cmd;
	*****
	3	2

	*/
	_VtxPtr[0].x = points[1].x; _VtxPtr[0].y = points[1].y; _VtxPtr[0].z = 0; _VtxPtr[0].dxColor = dxBoxColor2; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	_VtxPtr[1].x = points[0].x; _VtxPtr[1].y = points[0].y; _VtxPtr[1].z = 0; _VtxPtr[1].dxColor = dxBoxColor1; _VtxPtr[1].tu = uv.u1; _VtxPtr[1].tv = uv.v1;
	_VtxPtr[2].x = points[2].x; _VtxPtr[2].y = points[2].y; _VtxPtr[2].z = 0; _VtxPtr[2].dxColor = dxBoxColor3; _VtxPtr[2].tu = uv.u1; _VtxPtr[2].tv = uv.v1;
	//_cmd._VertexBufferSize = 3;
	_IdxPtr[0] = 0; _IdxPtr[1] = 1; _IdxPtr[2] = 2;
	//_cmd._IndexBufferSize = 3;
	//AddDrawCommand(_cmd);
	AddDrawCommand(cmd);
}

void PizdaDrawList::AddQuarterCircle(Vector2D start, float radius, float quart, PizdaColor c1) {
	DWORD col1 = D3DCOLOR_RGBA((int)c1.r, (int)c1.g, (int)c1.b, (int)c1.a);
	//PizdaDrawCommand cmd;
	//cmd.Clear();
	//cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	MemoryReserve(33, 12);
	PizdaDrawCmd cmd;
	cmd._IndexBufferSize = 0;
	cmd._VertexBufferSize = 0;
	cmd._TexturePtr = CRender::Get().font_map["BigNoodle"]->GetTexture();
	const auto uv = CRender::Get().font_map["BigNoodle"]->WhitePixel;
	//cmd._VertexBufferPtr = new D3DTLVERTEX[15];
	//cmd._IndexBufferPtr = new DWORD[45];
	int i = 1;
	_VtxPtr[0].x = start.x; _VtxPtr[0].y = start.y; _VtxPtr[0].z = 0.f; _VtxPtr[0].dxColor = col1; _VtxPtr[0].tu = uv.u1; _VtxPtr[0].tv = uv.v1;
	cmd._VertexBufferSize++;
	int id1 = i;
	for (float lat = quart; lat < quart + M_PI / 2 + D3DXToRadian(4.5f); lat += D3DXToRadian(9), i++)
	{
		int id2 = i == 11 ? 1 : id1 + 1;
		float sin1 = sin(lat);
		float cos1 = cos(lat);
		_VtxPtr[i].x = start.x + radius * cos1;
		_VtxPtr[i].y = start.y + radius * sin1;
		_VtxPtr[i].z = 0.f;
		_VtxPtr[i].dxColor = col1;
		_VtxPtr[i].tu = uv.u1;
		_VtxPtr[i].tv = uv.v1;
		cmd._VertexBufferSize++;
		_IdxPtr[0] = 0;
		_IdxPtr[1] = id1;
		_IdxPtr[2] = id2;
		cmd._IndexBufferSize += 3;
		_IdxPtr += 3;
		id1 = id2;
	}
	AddDrawCommand(cmd);
	//AddDrawCommand(cmd);
}

void PizdaDrawList::AddRoundedBox(Vector2D start, Vector2D size, float smooth, PizdaColor c1, bool outline, bool filled, PizdaColor fill_col) {
	POINT pt[4];

	// Get all corners
	pt[0].x = start.x + (size.x - smooth);
	pt[0].y = start.y + (size.y - smooth);

	pt[1].x = start.x + smooth;
	pt[1].y = start.y + (size.y - smooth);

	pt[2].x = start.x + smooth;
	pt[2].y = start.y + smooth;

	pt[3].x = start.x + size.x - smooth;
	pt[3].y = start.y + smooth;

	float fDegree = 0;
	vector<Vector2D> points;
	points.resize(120);
	int j = 0;
	for (int i = 0; i < 4; i++)
	{
		switch (i) {
		case 0:
			points[j++] = Vector2D(start.x + size.x, start.y + size.y - smooth);
			points[j++] = Vector2D(start.x + size.x, start.y + smooth);
			break;
		case 2:
			points[j++] = Vector2D(start.x, start.y + smooth);
			points[j++] = Vector2D(start.x, start.y + size.x - smooth);
			break;
		}
		for (float k = fDegree; k < fDegree + M_PI / 2; k += D3DXToRadian(9))
		{
			points[j++] = Vector2D(pt[i].x + (cosf(k) * smooth), pt[i].y + (sinf(k) * smooth));
			if (filled)
				AddQuarterCircle(Vector2D(pt[i].x, pt[i].y), smooth, fDegree, PizdaColor(fill_col.r, fill_col.g, fill_col.b, fill_col.a / 9, false));
		}
		fDegree += M_PI / 2;
	}
	AddMultipointLine(points, j, true, c1, 1);
	if (filled)
		AddRectangleFilled(Vector2D(start.x, start.y + smooth), Vector2D(size.x, size.y - smooth * 2), fill_col);
}

float PizdaDrawList::AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, const char* format, ...) {
	char Buffer[128] = { '\0' };
	va_list va_alist;
	va_start(va_alist, format);
	vsprintf_s(Buffer, format, va_alist);
	//vsprintf_s(Buffer, format, va_alist);
	va_end(va_alist);
	return AddText_NF(start, center, shadow, color, fontname, InputSystem::Get().utf8toWStr(Buffer).c_str());
}

float PizdaDrawList::AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, float scale, const char* format, ...) {
	char Buffer[128] = { '\0' };
	va_list va_alist;
	va_start(va_alist, format);
	vsprintf_s(Buffer, format, va_alist);
	//vsprintf_s(Buffer, format, va_alist);
	va_end(va_alist);
	return AddText_NF(start, center, shadow, color, fontname, scale, InputSystem::Get().utf8toWStr(Buffer).c_str());
}

float PizdaDrawList::AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, const wchar_t* format, ...) {
	if (!CRender::Get().font_map.count(fontname))return 0.f;
	wchar_t Buffer[128] = { '\0' };
	va_list va_alist;
	va_start(va_alist, format);
	wvsprintfW(Buffer, format, va_alist);
	va_end(va_alist);
	return AddText_NF(start, center, shadow, color, fontname, Buffer);
}

float PizdaDrawList::AddText(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, float scale, const wchar_t* format, ...) {
	if (!CRender::Get().font_map.count(fontname))return 0.f;
	wchar_t Buffer[128] = { '\0' };
	va_list va_alist;
	va_start(va_alist, format);
	wvsprintfW(Buffer, format, va_alist);
	//vsprintf_s(Buffer, format, va_alist);
	va_end(va_alist);
	return AddText_NF(start, center, shadow, color, fontname, scale, Buffer);
}

float PizdaDrawList::AddText_NF(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, wstring text) {
	const auto font = CRender::Get().font_map[fontname];
	const auto TextStats = font->GeFontStats();
	const auto font_size = 46.f;
	if (center) {
		start.x -= float(font->GetLength(text, TextStats.CharHeight) / 2);
	}
	float OutputX = start.x;
	float OutputY = start.y;
	for (const auto& ch : text)
	{
		if (ch == '\n')
		{
			OutputX = start.x;
			OutputY += float((TextStats.CharHeight + 1) / 2.f);
			continue;
		}
		auto cha = font->Get(ch);
		if (shadow) {
			AddRectangleUV(Vector2D(OutputX - 1, OutputY - 1), Vector2D(font->CharLen(ch, font_size), TextStats.CharHeight), Color::Black(), font->GetTexture(), cha);
			AddRectangleUV(Vector2D(OutputX + 1, OutputY + 1), Vector2D(font->CharLen(ch, font_size), TextStats.CharHeight), Color::Black(), font->GetTexture(), cha);
		}
		AddRectangleUV(Vector2D(OutputX, OutputY), Vector2D(font->CharLen(ch, font_size), TextStats.CharHeight), color, font->GetTexture(), cha);
		OutputX += font->CharLen(ch, font_size);
	}
	return font->GetLength(text, font_size);
}
float PizdaDrawList::AddText_NF(Vector2D start, bool center, bool shadow, PizdaColor color, string fontname, float scale, wstring text) {
	const auto font = CRender::Get().font_map[fontname];
	auto TextStats = font->GeFontStats();
	const auto font_size = scale;
	scale = scale / 46.f;
	if (center) {
		start.x -= float(font->GetLength(text, font_size) / 2);
	}
	float OutputX = start.x;
	float OutputY = start.y;
	for (const auto& ch : text) {
		if (ch == '\n')
		{
			OutputX = start.x;
			OutputY += float((TextStats.CharHeight + 1) * scale / 2);
			continue;
		}
		auto cha = font->Get(ch);
		if (shadow) {
			AddRectangleUV(Vector2D(OutputX - 1, OutputY - 1), Vector2D(font->CharLen(ch, font_size), TextStats.CharHeight * scale), Color::Black(), font->GetTexture(), cha);
			AddRectangleUV(Vector2D(OutputX + 1, OutputY + 1), Vector2D(font->CharLen(ch, font_size), TextStats.CharHeight * scale), Color::Black(), font->GetTexture(), cha);
		}
		AddRectangleUV(Vector2D(OutputX, OutputY), Vector2D(font->CharLen(ch, font_size), TextStats.CharHeight * scale), color, font->GetTexture(), cha);
		OutputX += font->CharLen(ch, font_size);
	}
	return font->GetLength(text, font_size);
}
#pragma endregion
