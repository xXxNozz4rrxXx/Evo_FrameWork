#pragma once
#include <string_view>
#include <Windows.h>
#include <map>
#include <d3d9.h>

using namespace std;

struct UVTable
{
	float u1, u2, v1, v2;
	UVTable(float u1, float v1, float u2, float v2) {
		this->u1 = u1;
		this->u2 = u2;
		this->v1 = v1;
		this->v2 = v2;
	}
	UVTable() {
		memset(this, 0, sizeof(UVTable));
	}
};

class PizdaFont
{
public:
	enum Flags {
		FLAG_NONE = 0,
		FLAG_BOLD = 0x1,
	};
private:
	TCHAR FontName[128];
	HFONT GDIFont;
	int TextureWidth, TextureHeight;
	static LPDIRECT3DDEVICE9 pd3dDevice;
	float font_size;
	LPDIRECT3DTEXTURE9 CharactersTexture;
	UVTable LookUpTable[161];
	int FindTextureSize(HDC hdc);
	void GetTextStats();
	map<wchar_t, int> _glyphset;
public:
	PizdaFont();
	PizdaFont(string_view FontName, float FontSize, COLORREF TextColor,int flags = FLAG_NONE, LPDIRECT3DDEVICE9 dev = nullptr);
	~PizdaFont();
	void Initialize(string_view FontName, float FontSize, COLORREF TextColor, int flags, LPDIRECT3DDEVICE9 dev);
	UVTable Get(wchar_t c);
	int GetLength(wstring s, float font_size);
	int GetLength(string s, float font_size);
	float CharLen(wchar_t s, float font_size);
	struct TextMetrics
	{
		int CharWidth[161];
		int CharHeight;
		unsigned int AvgCharWidth;
		unsigned int MaxCharWidth;
		int MaxCharHeight;
	} TextStats;
	UVTable WhitePixel;
	inline TextMetrics GeFontStats() { return TextStats; }
	inline LPDIRECT3DTEXTURE9 GetTexture() { return CharactersTexture; }
	
};

