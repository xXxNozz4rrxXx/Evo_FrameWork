#include "Render.h"

#pragma region PizdaFont
LPDIRECT3DDEVICE9 PizdaFont::pd3dDevice = nullptr;
PizdaFont::PizdaFont()
{
	CharactersTexture = NULL;
	ZeroMemory(FontName, sizeof(FontName));
	GDIFont = NULL;
}

PizdaFont::PizdaFont(string_view FontName, float FontSize, COLORREF TextColor, int flags, LPDIRECT3DDEVICE9 pd3dDevice)
{
	PizdaFont();
	Initialize(FontName, FontSize, TextColor, flags, pd3dDevice);
}

PizdaFont::~PizdaFont()
{
	if (NULL != CharactersTexture)
	{
		CharactersTexture->Release();
	}

	DeleteObject(GDIFont);
}


void PizdaFont::Initialize(string_view FontName, float FontSize, COLORREF TextColor, int flags, LPDIRECT3DDEVICE9 dev)
{
	font_size = FontSize;
	HRESULT Result = 0;
	if (dev)
		this->pd3dDevice = dev;

	HDC hdc = CreateCompatibleDC(NULL);

	TextStats.CharHeight = -MulDiv(FontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);

	GDIFont = CreateFontA(TextStats.CharHeight, 0, 0, 0, flags & FLAG_BOLD == 1 ? FW_BLACK : FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, FontName.data());
	HFONT PrevFont = (HFONT)SelectObject(hdc, GDIFont);
	SetTextColor(hdc, TextColor);
	SetMapMode(hdc, MM_TEXT);
	HBITMAP IntermediateDIB = NULL;
	BITMAPINFO DIBInfo;

	TEXTMETRIC TextMetrics;
	GetTextMetrics(hdc, &TextMetrics);
	TextStats.CharHeight = TextMetrics.tmHeight + TextMetrics.tmExternalLeading + 1; //Add one to prevent an off-by-one bug.
	TextureWidth = FindTextureSize(hdc);
	TextureHeight = TextureWidth;
	ZeroMemory(&DIBInfo.bmiHeader, sizeof(BITMAPINFOHEADER));
	DIBInfo.bmiHeader.biBitCount = 32;
	DIBInfo.bmiHeader.biCompression = BI_RGB;
	DIBInfo.bmiHeader.biPlanes = 1;
	DIBInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	DIBInfo.bmiHeader.biWidth = TextureWidth;
	DIBInfo.bmiHeader.biHeight = TextureHeight;
	DWORD* BitmapBits;
	IntermediateDIB = CreateDIBSection(hdc, &DIBInfo, DIB_RGB_COLORS, (void**)&BitmapBits, NULL, 0);
	while (!IntermediateDIB)
	{
		IntermediateDIB = CreateDIBSection(hdc, &DIBInfo, DIB_RGB_COLORS, (void**)&BitmapBits, NULL, 0);
	}
	SelectObject(hdc, IntermediateDIB);
	COLORREF BackgroundColor = RGB(~GetRValue(TextColor), ~GetGValue(TextColor), ~GetBValue(TextColor));
	LOGBRUSH BKLogBrush;
	BKLogBrush.lbColor = BackgroundColor;
	BKLogBrush.lbStyle = BS_SOLID;
	HBRUSH BKBrush = CreateBrushIndirect(&BKLogBrush);
	RECT BitmapArea;
	SetRect(&BitmapArea, 0, 0, TextureWidth, TextureHeight);
	FillRect(hdc, &BitmapArea, BKBrush);
	SetBkColor(hdc, BackgroundColor);
	int TextOutX = 0, TextOutY = 0;
	SIZE CharSize;
	TCHAR TempString[] = TEXT("x");
	TCHAR Character = 1025;
	for (int j = 0; j < 161; j++, Character++)
	{
		Character = Character == 1026 ? 1040 : Character;
		Character = Character == 1104 ? 1105 : Character;
		Character = Character == 1106 ? 32 : Character;
		_glyphset[Character] = j;
		TempString[0] = Character;//410 44F
		GetTextExtentPoint32(hdc, TempString, 1, &CharSize);
		CharSize.cx += 1;   //Add one to prevent off-by-one bug.
		if (CharSize.cx + TextOutX > TextureWidth)
		{
			if (TextOutY + TextStats.CharHeight > TextureHeight)
			{
				return;
			}
			TextOutX = 0;
			TextOutY += TextStats.CharHeight;
		}
		ExtTextOut(hdc, TextOutX, TextOutY, ETO_OPAQUE, nullptr, TempString, 1, nullptr);
		TextStats.CharWidth[j] = CharSize.cx;
		LookUpTable[j].u1 = 1.f - float(float(TextureWidth - TextOutX + 1) / TextureWidth);
		LookUpTable[j].v1 = 1.f - float(float(TextureHeight - TextOutY - 1) / TextureHeight);
		LookUpTable[j].u2 = 1.f - float(float(TextureWidth - (float(TextOutX + CharSize.cx - 1) + 1)) / TextureWidth);
		LookUpTable[j].v2 = 1.f - float(float(TextureHeight - (float(TextOutY + TextStats.CharHeight - 1))) / TextureHeight);
		TextOutX += CharSize.cx;
	}

	this->GetTextStats();
	while (D3D_OK != (Result = pd3dDevice->CreateTexture(TextureWidth, TextureHeight, 1, 0, D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED, &CharactersTexture, NULL)))
	{
		return;
	}

	///*
	D3DLOCKED_RECT LockedRect;
	if (D3D_OK != (Result = CharactersTexture->LockRect(0, &LockedRect, NULL, 0)))
	{
		return;
	}
	DWORD* DestData = (DWORD*)LockedRect.pBits;
	DWORD BMTextColor = ((TextColor >> 16) & 0x000000ff) |
		((TextColor << 16) & 0x00ff0000) |
		(TextColor & 0x0000ff00);
	BitmapBits += TextureWidth * (TextureHeight - 1);
	int y;
	for (y = 0; y < TextureHeight; y++)
	{
		DWORD* Row = BitmapBits;
		for (int k = 0; k < TextureWidth; k++, Row++)
		{
			if (!(*Row == BMTextColor))
			{
				*Row = 0x00000000;
			}
			else
			{
				*Row |= 0xff000000;
				WhitePixel.u1 = 1.f - float(float(TextureWidth - k) / TextureWidth);
				WhitePixel.v1 = 1.f - float(float(TextureHeight - y) / TextureHeight);
				WhitePixel.u2 = 1.f - float(float(TextureWidth - (k + 1)) / TextureWidth);
				WhitePixel.v2 = 1.f - float(float(TextureHeight - (y + 1)) / TextureHeight);
			}
		}
		memcpy(DestData, BitmapBits, TextureWidth * sizeof(DWORD));
		DestData += (LockedRect.Pitch / 4);
		BitmapBits -= TextureWidth;
	}
	CharactersTexture->UnlockRect(0);
	//*/
	DeleteDC(hdc);
}

int PizdaFont::FindTextureSize(HDC hdc)
{
	int PotentialSize = 128;

	TEXTMETRIC tm;
	GetTextMetrics(hdc, &tm);
	TextStats.CharHeight = tm.tmHeight + tm.tmExternalLeading;
	TCHAR TempString[] = TEXT("x");
	TCHAR Character = 1025;
	SIZE CharSize;
	int TextOutX = 0, TextOutY = 0;

	int j = 0;

	while (j != 161)
	{
		for (j = 0; j < 161; j++, Character++)
		{
			Character = Character == 1026 ? 1040 : Character;
			Character = Character == 1104 ? 1105 : Character;
			Character = Character == 1106 ? 32 : Character;
			TempString[0] = Character;
			GetTextExtentPoint32(hdc, TempString, 1, &CharSize);
			CharSize.cx += 1;

			if (CharSize.cx + TextOutX > PotentialSize)
			{
				if (TextOutY + TextStats.CharHeight > PotentialSize)
				{
					break;
				}
				TextOutX = 0;
				TextOutY += TextStats.CharHeight;
			}

			TextOutX += CharSize.cx;
		}

		PotentialSize *= 2;
	}

	return PotentialSize;


}

void PizdaFont::GetTextStats()
{
	//--------------------------------------------

	//Find average char width

	int i;
	TextStats.AvgCharWidth = 0;
	for (i = 0; i < 161; i++)
	{
		TextStats.AvgCharWidth += TextStats.CharWidth[i];
	}
	TextStats.AvgCharWidth /= 161;

	//--------------------------------------------

	//Find max char width. 

	TextStats.MaxCharWidth = 0;
	for (i = 0; i < 161; i++)
	{
		TextStats.MaxCharWidth = max(TextStats.MaxCharWidth, TextStats.CharWidth[i]);
	}
}

UVTable PizdaFont::Get(wchar_t c)
{
	if (_glyphset.count(c)) {
		return LookUpTable[_glyphset[c]];
	}
	return LookUpTable[_glyphset[L'?']];
	//const unsigned int Offset = 32;
	//
	//if (text[i] >= 32 && text[i] <= 94 + 32)
	//Offset = -34;
	//else if (text[i] >= -64 && text[i] <= -1)
	//Offset = -65;
	//else if (text[i] == 168)
	//Offset = 168;
	//else if (text[i] == 184)
	//Offset = 119;
	//else
	//text[i] = 63;
	//
	//if (c >= 32 && c <= 94 + 32)
	//	return LookUpTable[c - Offset + 66];
	//else if (c >= -64 && c <= -1)
	//	return LookUpTable[c + 65];
	//else if (c == '¨')
	//	return LookUpTable[0];
	//else if (c == '¸')
	//	return LookUpTable[65];
	//return LookUpTable[97];
}

float PizdaFont::CharLen(wchar_t s, float font_size) {
	float scale = font_size / this->font_size;
	if (_glyphset.count(s)) {
		return TextStats.CharWidth[_glyphset[s]] * scale;
	}
	return TextStats.CharWidth[_glyphset[L'?']] * scale;
}

int PizdaFont::GetLength(wstring s, float font_size) {
	int sz = 0;
	for (const auto& ch : s) {
		sz += CharLen(ch, font_size);
	}
	return sz;
}

int PizdaFont::GetLength(string s, float font_size) {
	return GetLength(InputSystem::Get().utf8toWStr(s), font_size);
}
#pragma endregion