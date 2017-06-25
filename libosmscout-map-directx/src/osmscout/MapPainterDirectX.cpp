/*
  This source is part of the libosmscout-map library
  Copyright (C) 2011  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/MapPainterDirectX.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <list>

#include <osmscout/util/String.h>
#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

//add_definitions(-DUNICODE -D_UNICODE)

#if defined(UNICODE) || defined(_UNICODE) || defined(_MBCS) || defined(MBCS)
  #define MBUC
  #define __T(x)      L ## x
#else
  #define __T(x)           x
#endif

#define _T(x)       __T(x)

namespace osmscout
{
	static const uint64_t s_crc64_tab[256] = { UINT64_C(0x0000000000000000), UINT64_C(0x7ad870c830358979), UINT64_C(0xf5b0e190606b12f2), UINT64_C(0x8f689158505e9b8b), UINT64_C(0xc038e5739841b68f), UINT64_C(0xbae095bba8743ff6), UINT64_C(0x358804e3f82aa47d), UINT64_C(0x4f50742bc81f2d04), UINT64_C(0xab28ecb46814fe75), UINT64_C(0xd1f09c7c5821770c), UINT64_C(0x5e980d24087fec87), UINT64_C(0x24407dec384a65fe), UINT64_C(0x6b1009c7f05548fa), UINT64_C(0x11c8790fc060c183), UINT64_C(0x9ea0e857903e5a08), UINT64_C(0xe478989fa00bd371), UINT64_C(0x7d08ff3b88be6f81), UINT64_C(0x07d08ff3b88be6f8), UINT64_C(0x88b81eabe8d57d73), UINT64_C(0xf2606e63d8e0f40a), UINT64_C(0xbd301a4810ffd90e), UINT64_C(0xc7e86a8020ca5077), UINT64_C(0x4880fbd87094cbfc), UINT64_C(0x32588b1040a14285), UINT64_C(0xd620138fe0aa91f4), UINT64_C(0xacf86347d09f188d), UINT64_C(0x2390f21f80c18306), UINT64_C(0x594882d7b0f40a7f), UINT64_C(0x1618f6fc78eb277b), UINT64_C(0x6cc0863448deae02), UINT64_C(0xe3a8176c18803589), UINT64_C(0x997067a428b5bcf0), UINT64_C(0xfa11fe77117cdf02), UINT64_C(0x80c98ebf2149567b), UINT64_C(0x0fa11fe77117cdf0), UINT64_C(0x75796f2f41224489), UINT64_C(0x3a291b04893d698d), UINT64_C(0x40f16bccb908e0f4), UINT64_C(0xcf99fa94e9567b7f), UINT64_C(0xb5418a5cd963f206), UINT64_C(0x513912c379682177), UINT64_C(0x2be1620b495da80e), UINT64_C(0xa489f35319033385), UINT64_C(0xde51839b2936bafc), UINT64_C(0x9101f7b0e12997f8), UINT64_C(0xebd98778d11c1e81), UINT64_C(0x64b116208142850a), UINT64_C(0x1e6966e8b1770c73), UINT64_C(0x8719014c99c2b083), UINT64_C(0xfdc17184a9f739fa), UINT64_C(0x72a9e0dcf9a9a271), UINT64_C(0x08719014c99c2b08), UINT64_C(0x4721e43f0183060c), UINT64_C(0x3df994f731b68f75), UINT64_C(0xb29105af61e814fe), UINT64_C(0xc849756751dd9d87), UINT64_C(0x2c31edf8f1d64ef6), UINT64_C(0x56e99d30c1e3c78f), UINT64_C(0xd9810c6891bd5c04), UINT64_C(0xa3597ca0a188d57d), UINT64_C(0xec09088b6997f879), UINT64_C(0x96d1784359a27100), UINT64_C(0x19b9e91b09fcea8b), UINT64_C(0x636199d339c963f2), UINT64_C(0xdf7adabd7a6e2d6f), UINT64_C(0xa5a2aa754a5ba416), UINT64_C(0x2aca3b2d1a053f9d), UINT64_C(0x50124be52a30b6e4), UINT64_C(0x1f423fcee22f9be0), UINT64_C(0x659a4f06d21a1299), UINT64_C(0xeaf2de5e82448912), UINT64_C(0x902aae96b271006b), UINT64_C(0x74523609127ad31a), UINT64_C(0x0e8a46c1224f5a63), UINT64_C(0x81e2d7997211c1e8), UINT64_C(0xfb3aa75142244891), UINT64_C(0xb46ad37a8a3b6595), UINT64_C(0xceb2a3b2ba0eecec), UINT64_C(0x41da32eaea507767), UINT64_C(0x3b024222da65fe1e), UINT64_C(0xa2722586f2d042ee), UINT64_C(0xd8aa554ec2e5cb97), UINT64_C(0x57c2c41692bb501c), UINT64_C(0x2d1ab4dea28ed965), UINT64_C(0x624ac0f56a91f461), UINT64_C(0x1892b03d5aa47d18), UINT64_C(0x97fa21650afae693), UINT64_C(0xed2251ad3acf6fea), UINT64_C(0x095ac9329ac4bc9b), UINT64_C(0x7382b9faaaf135e2), UINT64_C(0xfcea28a2faafae69), UINT64_C(0x8632586aca9a2710), UINT64_C(0xc9622c4102850a14), UINT64_C(0xb3ba5c8932b0836d), UINT64_C(0x3cd2cdd162ee18e6), UINT64_C(0x460abd1952db919f), UINT64_C(0x256b24ca6b12f26d), UINT64_C(0x5fb354025b277b14), UINT64_C(0xd0dbc55a0b79e09f), UINT64_C(0xaa03b5923b4c69e6), UINT64_C(0xe553c1b9f35344e2), UINT64_C(0x9f8bb171c366cd9b), UINT64_C(0x10e3202993385610), UINT64_C(0x6a3b50e1a30ddf69), UINT64_C(0x8e43c87e03060c18), UINT64_C(0xf49bb8b633338561), UINT64_C(0x7bf329ee636d1eea), UINT64_C(0x012b592653589793), UINT64_C(0x4e7b2d0d9b47ba97), UINT64_C(0x34a35dc5ab7233ee), UINT64_C(0xbbcbcc9dfb2ca865), UINT64_C(0xc113bc55cb19211c), UINT64_C(0x5863dbf1e3ac9dec), UINT64_C(0x22bbab39d3991495), UINT64_C(0xadd33a6183c78f1e), UINT64_C(0xd70b4aa9b3f20667), UINT64_C(0x985b3e827bed2b63), UINT64_C(0xe2834e4a4bd8a21a), UINT64_C(0x6debdf121b863991), UINT64_C(0x1733afda2bb3b0e8), UINT64_C(0xf34b37458bb86399), UINT64_C(0x8993478dbb8deae0), UINT64_C(0x06fbd6d5ebd3716b), UINT64_C(0x7c23a61ddbe6f812), UINT64_C(0x3373d23613f9d516), UINT64_C(0x49aba2fe23cc5c6f), UINT64_C(0xc6c333a67392c7e4), UINT64_C(0xbc1b436e43a74e9d), UINT64_C(0x95ac9329ac4bc9b5), UINT64_C(0xef74e3e19c7e40cc), UINT64_C(0x601c72b9cc20db47), UINT64_C(0x1ac40271fc15523e), UINT64_C(0x5594765a340a7f3a), UINT64_C(0x2f4c0692043ff643), UINT64_C(0xa02497ca54616dc8), UINT64_C(0xdafce7026454e4b1), UINT64_C(0x3e847f9dc45f37c0), UINT64_C(0x445c0f55f46abeb9), UINT64_C(0xcb349e0da4342532), UINT64_C(0xb1eceec59401ac4b), UINT64_C(0xfebc9aee5c1e814f), UINT64_C(0x8464ea266c2b0836), UINT64_C(0x0b0c7b7e3c7593bd), UINT64_C(0x71d40bb60c401ac4), UINT64_C(0xe8a46c1224f5a634), UINT64_C(0x927c1cda14c02f4d), UINT64_C(0x1d148d82449eb4c6), UINT64_C(0x67ccfd4a74ab3dbf), UINT64_C(0x289c8961bcb410bb), UINT64_C(0x5244f9a98c8199c2), UINT64_C(0xdd2c68f1dcdf0249), UINT64_C(0xa7f41839ecea8b30), UINT64_C(0x438c80a64ce15841), UINT64_C(0x3954f06e7cd4d138), UINT64_C(0xb63c61362c8a4ab3), UINT64_C(0xcce411fe1cbfc3ca), UINT64_C(0x83b465d5d4a0eece), UINT64_C(0xf96c151de49567b7), UINT64_C(0x76048445b4cbfc3c), UINT64_C(0x0cdcf48d84fe7545), UINT64_C(0x6fbd6d5ebd3716b7), UINT64_C(0x15651d968d029fce), UINT64_C(0x9a0d8ccedd5c0445), UINT64_C(0xe0d5fc06ed698d3c), UINT64_C(0xaf85882d2576a038), UINT64_C(0xd55df8e515432941), UINT64_C(0x5a3569bd451db2ca), UINT64_C(0x20ed197575283bb3), UINT64_C(0xc49581ead523e8c2), UINT64_C(0xbe4df122e51661bb), UINT64_C(0x3125607ab548fa30), UINT64_C(0x4bfd10b2857d7349), UINT64_C(0x04ad64994d625e4d), UINT64_C(0x7e7514517d57d734), UINT64_C(0xf11d85092d094cbf), UINT64_C(0x8bc5f5c11d3cc5c6), UINT64_C(0x12b5926535897936), UINT64_C(0x686de2ad05bcf04f), UINT64_C(0xe70573f555e26bc4), UINT64_C(0x9ddd033d65d7e2bd), UINT64_C(0xd28d7716adc8cfb9), UINT64_C(0xa85507de9dfd46c0), UINT64_C(0x273d9686cda3dd4b), UINT64_C(0x5de5e64efd965432), UINT64_C(0xb99d7ed15d9d8743), UINT64_C(0xc3450e196da80e3a), UINT64_C(0x4c2d9f413df695b1), UINT64_C(0x36f5ef890dc31cc8), UINT64_C(0x79a59ba2c5dc31cc), UINT64_C(0x037deb6af5e9b8b5), UINT64_C(0x8c157a32a5b7233e), UINT64_C(0xf6cd0afa9582aa47), UINT64_C(0x4ad64994d625e4da), UINT64_C(0x300e395ce6106da3), UINT64_C(0xbf66a804b64ef628), UINT64_C(0xc5bed8cc867b7f51), UINT64_C(0x8aeeace74e645255), UINT64_C(0xf036dc2f7e51db2c), UINT64_C(0x7f5e4d772e0f40a7), UINT64_C(0x05863dbf1e3ac9de), UINT64_C(0xe1fea520be311aaf), UINT64_C(0x9b26d5e88e0493d6), UINT64_C(0x144e44b0de5a085d), UINT64_C(0x6e963478ee6f8124), UINT64_C(0x21c640532670ac20), UINT64_C(0x5b1e309b16452559), UINT64_C(0xd476a1c3461bbed2), UINT64_C(0xaeaed10b762e37ab), UINT64_C(0x37deb6af5e9b8b5b), UINT64_C(0x4d06c6676eae0222), UINT64_C(0xc26e573f3ef099a9), UINT64_C(0xb8b627f70ec510d0), UINT64_C(0xf7e653dcc6da3dd4), UINT64_C(0x8d3e2314f6efb4ad), UINT64_C(0x0256b24ca6b12f26), UINT64_C(0x788ec2849684a65f), UINT64_C(0x9cf65a1b368f752e), UINT64_C(0xe62e2ad306bafc57), UINT64_C(0x6946bb8b56e467dc), UINT64_C(0x139ecb4366d1eea5), UINT64_C(0x5ccebf68aecec3a1), UINT64_C(0x2616cfa09efb4ad8), UINT64_C(0xa97e5ef8cea5d153), UINT64_C(0xd3a62e30fe90582a), UINT64_C(0xb0c7b7e3c7593bd8), UINT64_C(0xca1fc72bf76cb2a1), UINT64_C(0x45775673a732292a), UINT64_C(0x3faf26bb9707a053), UINT64_C(0x70ff52905f188d57), UINT64_C(0x0a2722586f2d042e), UINT64_C(0x854fb3003f739fa5), UINT64_C(0xff97c3c80f4616dc), UINT64_C(0x1bef5b57af4dc5ad), UINT64_C(0x61372b9f9f784cd4), UINT64_C(0xee5fbac7cf26d75f), UINT64_C(0x9487ca0fff135e26), UINT64_C(0xdbd7be24370c7322), UINT64_C(0xa10fceec0739fa5b), UINT64_C(0x2e675fb4576761d0), UINT64_C(0x54bf2f7c6752e8a9), UINT64_C(0xcdcf48d84fe75459), UINT64_C(0xb71738107fd2dd20), UINT64_C(0x387fa9482f8c46ab), UINT64_C(0x42a7d9801fb9cfd2), UINT64_C(0x0df7adabd7a6e2d6), UINT64_C(0x772fdd63e7936baf), UINT64_C(0xf8474c3bb7cdf024), UINT64_C(0x829f3cf387f8795d), UINT64_C(0x66e7a46c27f3aa2c), UINT64_C(0x1c3fd4a417c62355), UINT64_C(0x935745fc4798b8de), UINT64_C(0xe98f353477ad31a7), UINT64_C(0xa6df411fbfb21ca3), UINT64_C(0xdc0731d78f8795da), UINT64_C(0x536fa08fdfd90e51), UINT64_C(0x29b7d047efec8728) };

	uint64_t crc64(const unsigned char *data, size_t size)
	{
		uint64_t retVal = 0;
		for (size_t uj = 0; uj < size; uj++)
		{
			uint8_t byte = data[uj];
			retVal = s_crc64_tab[(uint8_t)retVal ^ byte] ^ (retVal >> 8);
		}
		return retVal;
	}

	uint32_t GetFontHash(const char* fontFamily, double fontSize)
	{
		uint8_t x;
		uint16_t crc = 0xFFFF;
		int length = (int)strlen(fontFamily);
		while (length--)
		{
			x = crc >> 8 ^ *fontFamily++;
			x ^= x >> 4;
			crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
		}
		uint16_t size = (uint16_t)((int)fontSize);
		return (crc << 16) | size;
	}

	std::wstring s2w(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	D2D1_COLOR_F MapPainterDirectX::GetColorValue(const Color& color)
	{
		return D2D1::ColorF((FLOAT)color.GetR(), (FLOAT)color.GetG(), (FLOAT)color.GetB(), (FLOAT)color.GetA());
	}

	ID2D1SolidColorBrush* MapPainterDirectX::GetColorBrush(const Color& color)
	{
		uint8_t r = (uint8_t)(color.GetR() * 255.0);
		uint8_t g = (uint8_t)(color.GetG() * 255.0);
		uint8_t b = (uint8_t)(color.GetB() * 255.0);
		uint8_t a = (uint8_t)(color.GetA() * 255.0);
		uint32_t clr = (r << 24) | (g << 16) | (b << 8) | a;
		BrushMap::const_iterator bi = m_Brushs.find(clr);
		if (bi != m_Brushs.end()) {
			return bi->second;
		}
		ID2D1SolidColorBrush* solidColorBrush;
		HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(GetColorValue(color), &solidColorBrush);
		if (SUCCEEDED(hr))
			return m_Brushs.insert(std::make_pair(clr, solidColorBrush)).first->second;
		else
			return NULL;
	}

	ID2D1SolidColorBrush* MapPainterDirectX::GetColorBrush(D2D1_COLOR_F& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);
		uint32_t clr = (r << 24) | (g << 16) | (b << 8) | a;
		BrushMap::const_iterator bi = m_Brushs.find(clr);
		if (bi != m_Brushs.end()) {
			return bi->second;
		}
		ID2D1SolidColorBrush* solidColorBrush;
		HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(color, &solidColorBrush);
		if (SUCCEEDED(hr))
			return m_Brushs.insert(std::make_pair(clr, solidColorBrush)).first->second;
		else
			return NULL;
	}

	ID2D1StrokeStyle* MapPainterDirectX::GetStrokeStyle(const std::vector<double>& dash)
	{
		if (dash.size() > 0)
		{
			float* dashes = new float[dash.size()];
			for (size_t uj = 0; uj < dash.size(); uj++) dashes[uj] = (float)dash[uj];
			uint64_t id = crc64((const unsigned char*)dashes, dash.size() * sizeof(float));
			if (m_StrokeStyles.find(id) != m_StrokeStyles.end())
				return m_StrokeStyles[id];
			else
			{
				ID2D1StrokeStyle* pStrokeStyle = NULL;
				HRESULT hr = m_pDirect2dFactory->CreateStrokeStyle(
					D2D1::StrokeStyleProperties(
						D2D1_CAP_STYLE_FLAT,
						D2D1_CAP_STYLE_FLAT,
						D2D1_CAP_STYLE_ROUND,
						D2D1_LINE_JOIN_MITER,
						10.0f,
						D2D1_DASH_STYLE_CUSTOM,
						0.0f),
					dashes,
					dash.size(),
					&pStrokeStyle);
				if (SUCCEEDED(hr))
				{
					m_StrokeStyles[id] = pStrokeStyle;
					return pStrokeStyle;
				}
			}
		}
		return NULL;
	}

	void MapPainterDirectX::_DrawText(const Projection& projection, const MapParameter& parameter, double x, double y, double fontSize, const Color& color, std::string text)
	{
#ifdef MBUC
		std::wstring enc = s2w(text);
#else
		std::string enc = text;
#endif
		FLOAT size = fontSize * 20.0f * text.length();
		IDWriteTextFormat* tf = GetFont(projection, parameter, fontSize);
		IDWriteTextLayout* pDWriteTextLayout = NULL;
		HRESULT hr = m_pWriteFactory->CreateTextLayout(
			enc.c_str(),
			enc.length(),
			tf,
			size * 2.0f,
			size,
			&pDWriteTextLayout);
		if (FAILED(hr))
			return;
		DWRITE_TEXT_METRICS textMetrics;
		hr = pDWriteTextLayout->GetMetrics(&textMetrics);
		pDWriteTextLayout->Release();
		if (SUCCEEDED(hr)) {
			m_pRenderTarget->DrawText(enc.c_str(), enc.length(), tf, D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)x + textMetrics.width * 1.1f, (FLOAT)y + textMetrics.height * 1.1f), GetColorBrush(color));
		}
	}

	bool MapPainterDirectX::LoadBitmapFromFile(PCWSTR uri, ID2D1Bitmap **ppBitmap)
	{
		IWICBitmapDecoder *pDecoder = NULL;
		IWICBitmapFrameDecode *pSource = NULL;
		IWICStream*pStream = NULL;
		IWICFormatConverter *pConverter = NULL;
		IWICBitmapScaler *pScaler = NULL;

		HRESULT hr = m_pImagingFactory->CreateDecoderFromFilename(uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);

		if (SUCCEEDED(hr))
		{
			hr = pDecoder->GetFrame(0, &pSource);
		}

		if (SUCCEEDED(hr))
		{
			// Convert Image format to 32bppPBGRA
			// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
			hr = m_pImagingFactory->CreateFormatConverter(&pConverter);
		}

		if (SUCCEEDED(hr))
		{
			hr = pConverter->Initialize(
				pSource,
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.f,
				WICBitmapPaletteTypeMedianCut
				);
		}

		if (SUCCEEDED(hr))
		{
			// Create a Direct2D bitmap from the WIC bitmap
			hr = m_pRenderTarget->CreateBitmapFromWicBitmap(
				pConverter,
				NULL,
				ppBitmap
				);
		}

		if (pDecoder)
		{
			pDecoder->Release();
			pDecoder = NULL;
		}
		if (pSource)
		{
			pSource->Release();
			pSource = NULL;
		}
		if (pStream)
		{
			pStream->Release();
			pStream = NULL;
		}
		if (pConverter)
		{
			pConverter->Release();
			pConverter = NULL;
		}
		if (pScaler)
		{
			pScaler->Release();
			pScaler = NULL;
		}

		return (SUCCEEDED(hr));
	}

	IDWriteTextFormat* MapPainterDirectX::GetFont(const Projection& projection, const MapParameter& parameter, double fontSize)
	{
		FontMap::const_iterator f;
		fontSize = fontSize * projection.ConvertWidthToPixel(parameter.GetFontSize());

		uint32_t hash = GetFontHash(parameter.GetFontName().c_str(), fontSize);
		f = m_Fonts.find(hash);

		if (f != m_Fonts.end()) {
			return f->second;
		}
#ifdef MBUC
		std::wstring font = s2w(parameter.GetFontName());
#else
		std::string font = parameter.GetFontName();
#endif
		IDWriteTextFormat* pTextFormat;
		HRESULT hr = m_pWriteFactory->CreateTextFormat(
			font.c_str(),
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			fontSize,
			_T(""),
			&pTextFormat
			);

		if (SUCCEEDED(hr))
			return m_Fonts.insert(std::make_pair(hash, pTextFormat)).first->second;
		else
			return NULL;
	}

	void MapPainterDirectX::AfterPreprocessing(const StyleConfig& styleConfig, const Projection& projection, const MapParameter& parameter, const MapData& data)
	{
		size_t nextAreaId = 0;
		for (const auto& area : GetAreaData()) {
			if (area.fillStyle) {
				std::map<FillStyle, std::string>::const_iterator entry = fillStyleNameMap.find(*area.fillStyle);

				if (entry == fillStyleNameMap.end()) {
					std::string name = "area_" + NumberToString(nextAreaId);
					fillStyleNameMap.insert(std::make_pair(*area.fillStyle, name));
					nextAreaId++;
				}
			}

			if (area.borderStyle) {
				std::map<BorderStyle, std::string>::const_iterator entry = borderStyleNameMap.find(*area.borderStyle);

				if (entry == borderStyleNameMap.end()) {
					std::string name = "area_" + NumberToString(nextAreaId);
					borderStyleNameMap.insert(std::make_pair(*area.borderStyle, name));
					nextAreaId++;
				}
			}
		}
		size_t nextWayId = 0;
		for (const auto& way : GetWayData()) {
			std::map<LineStyle, std::string>::const_iterator entry = lineStyleNameMap.find(*way.lineStyle);

			if (entry == lineStyleNameMap.end()) {
				std::string name = "way_" + NumberToString(nextWayId);
				lineStyleNameMap.insert(std::make_pair(*way.lineStyle, name));
				nextWayId++;
			}
		}
	}

	void MapPainterDirectX::BeforeDrawing(const StyleConfig& styleConfig, const Projection& projection, const MapParameter& parameter, const MapData& data)
	{
	}

	void MapPainterDirectX::AfterDrawing(const StyleConfig& styleConfig, const Projection& projection, const MapParameter& parameter, const MapData& data)
	{
		for (GeometryMap::const_iterator entry = m_Polygons.begin(); entry != m_Polygons.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_Polygons[entry->first] = NULL;
			}
		}
		m_Polygons.clear();
	}

	bool MapPainterDirectX::HasIcon(const StyleConfig& styleConfig, const MapParameter& parameter, IconStyle& style)
	{
		// Already loaded with error
		if (style.GetIconId() == 0)
		{
			return false;
		}

		size_t idx = style.GetIconId() - 1;

		// Already cached?
		if (idx < m_Bitmaps.size() && m_Bitmaps[idx] != NULL)
		{
			return true;
		}

		if (m_pImagingFactory == NULL) return false;

		for (std::list<std::string>::const_iterator path = parameter.GetIconPaths().begin(); path != parameter.GetIconPaths().end(); ++path)
		{
			std::wstring filename = s2w(*path + style.GetIconName() + ".png");
			ID2D1Bitmap* pBitmap = NULL;
			if (LoadBitmapFromFile(filename.c_str(), &pBitmap))
			{
				std::wcout << L"Loaded image '" << filename << L"'" << std::endl;
				m_Bitmaps[idx] = pBitmap;
				return true;
			}
		}

		std::cerr << "ERROR while loading image '" << style.GetIconName() << "'" << std::endl;
		style.SetIconId(0);

		return false;
	}

	void MapPainterDirectX::GetFontHeight(const Projection& projection, const MapParameter& parameter, double fontSize, double& height)
	{
		if (fontHeightMap.find(fontSize) != fontHeightMap.end())
			height = fontHeightMap[fontSize];
		else
		{
			double x, y, w, h;
			GetTextDimension(projection, parameter, /*objectWidth*/ -1, fontSize, "App", x, y, w, h);
			fontHeightMap[fontSize] = h;
			height = h;
		}
	}

	void MapPainterDirectX::GetTextDimension(const Projection& projection, const MapParameter& parameter, double objectWidth, double fontSize, const std::string& text, double& xOff, double& yOff, double& width, double& height)
	{
#ifdef MBUC
		std::wstring sample = s2w(text);
#else
		std::string sample = text;
#endif
		FLOAT size = fontSize * 20.0f * text.length();
		IDWriteTextFormat* font = GetFont(projection, parameter, fontSize);
		IDWriteTextLayout* pDWriteTextLayout = NULL;
		HRESULT hr = m_pWriteFactory->CreateTextLayout(
			sample.c_str(),
			sample.length(),
			font,
			size * 2.0f,
			size,
			&pDWriteTextLayout);
		xOff = 0.0;
		yOff = 0.0;
		if (FAILED(hr))
		{
			width = 0.0;
			height = 0.0;
			return;
		}
		DWRITE_TEXT_METRICS textMetrics;
		hr = pDWriteTextLayout->GetMetrics(&textMetrics);
		pDWriteTextLayout->Release();
		if (FAILED(hr)) {
			width = 0.0;
			height = 0.0;
		}
		else
		{
			width = textMetrics.width;
			height = textMetrics.height;
		}
	}

	/*void MapPainterDirectX::GetLabelFrame(const LabelStyle& style, double& horizontal, double& vertical)
	{
		// Not implemented
	}*/

	void MapPainterDirectX::DrawGround(const Projection& projection, const MapParameter& parameter, const FillStyle& style)
	{
		m_pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, projection.GetWidth(), projection.GetHeight()), GetColorBrush(style.GetFillColor()));
	}

	void MapPainterDirectX::DrawLabel(const Projection& projection, const MapParameter& parameter, const LabelData& label)
	{
		if (dynamic_cast<const TextStyle*>(label.style.get()) != NULL) {
			const TextStyle* style = dynamic_cast<const TextStyle*>(label.style.get());
			double r = style->GetTextColor().GetR();
			double g = style->GetTextColor().GetG();
			double b = style->GetTextColor().GetB();
			_DrawText(projection, parameter, label.x, label.y, label.fontSize, Color(r, g, b, label.alpha), label.text);
		}
		else if (dynamic_cast<const ShieldStyle*>(label.style.get()) != NULL) {
			const ShieldStyle *style = dynamic_cast<const ShieldStyle*>(label.style.get());
			double r = style->GetTextColor().GetR();
			double g = style->GetTextColor().GetG();
			double b = style->GetTextColor().GetB();
			// Shield background
			D2D1_RECT_F shieldRectangle = D2D1::RectF(label.bx1, label.by1, label.bx2 + 1, label.by2 + 1);
			m_pRenderTarget->FillRectangle(shieldRectangle, GetColorBrush(style->GetBgColor()));
			// Shield border
			shieldRectangle = D2D1::RectF(label.bx1 + 2, label.by1 + 2, label.bx2 + 1 - 4, label.by2 + 1 - 4);
			m_pRenderTarget->DrawRectangle(shieldRectangle, GetColorBrush(style->GetBorderColor()));
			_DrawText(projection, parameter, label.x, label.y, label.fontSize, Color(r, g, b, label.alpha), label.text);
		}
		else
			_DrawText(projection, parameter, label.x, label.y, label.fontSize, Color::BLACK, label.text);
	}

	void MapPainterDirectX::DrawIcon(const IconStyle* style, double x, double y)
	{
		size_t idx = style->GetIconId() - 1;
		assert(idx < m_Bitmaps.size());
		assert(m_Bitmaps[idx] != NULL);
		D2D1_SIZE_U size = m_Bitmaps[idx]->GetPixelSize();
		FLOAT dx = (FLOAT)size.width / 2.0f;
		FLOAT dy = (FLOAT)size.height / 2.0f;
		m_pRenderTarget->DrawBitmap(m_Bitmaps[idx], D2D1::RectF(x - dx, y - dy, x + dx, y + dy));
	}

	void MapPainterDirectX::DrawSymbol(const Projection& projection, const MapParameter& parameter, const Symbol& symbol, double x, double y)
	{
		double minX;
		double minY;
		double maxX;
		double maxY;

		symbol.GetBoundingBox(minX, minY, maxX, maxY);
		double centerX = (minX + maxX) / 2.0;
		double centerY = (minY + maxY) / 2.0;

		for (std::list<DrawPrimitiveRef>::const_iterator p = symbol.GetPrimitives().begin(); p != symbol.GetPrimitives().end(); ++p)
		{
			DrawPrimitive* primitive = p->get();
			FillStyleRef   fillStyle = primitive->GetFillStyle();
			BorderStyleRef borderStyle = primitive->GetBorderStyle();
			bool hasBorder = borderStyle && borderStyle->GetWidth() > 0.0 && borderStyle->GetColor().IsVisible();
			double borderWidth = hasBorder ? projection.ConvertWidthToPixel(borderStyle->GetWidth()) : 0.0;
			if (dynamic_cast<PolygonPrimitive*>(primitive) != NULL)
			{
				PolygonPrimitive* polygon = dynamic_cast<PolygonPrimitive*>(primitive);
				const std::list<osmscout::Vertex2D> data = polygon->GetCoords();
				if (data.size() > 2)
				{
					double* coords = new double[data.size() * 2];
					std::list<osmscout::Vertex2D>::const_iterator iter = data.begin();
					for (size_t uj = 0; iter != data.end(); uj++)
					{
						coords[uj * 2 + 0] = iter->GetX();
						coords[uj * 2 + 1] = iter->GetY();
						iter++;
					}
					uint64_t hash = crc64((const unsigned char *)coords, data.size() * sizeof(double));
					GeometryMap::const_iterator g = m_Polygons.find(hash);
					if (g != m_Polygons.end())
					{
						m_pRenderTarget->FillGeometry(g->second, GetColorBrush(polygon->GetFillStyle()->GetFillColor()));
						if (hasBorder) m_pRenderTarget->DrawGeometry(g->second, GetColorBrush(polygon->GetBorderStyle()->GetColor()), borderWidth, GetStrokeStyle(polygon->GetBorderStyle()->GetDash()));
					}
					else
					{
						ID2D1PathGeometry* pPathGeometry;
						HRESULT hr = m_pDirect2dFactory->CreatePathGeometry(&pPathGeometry);
						if (SUCCEEDED(hr))
						{
							ID2D1GeometrySink *pSink = NULL;
							hr = pPathGeometry->Open(&pSink);
							if (SUCCEEDED(hr))
							{
								pSink->BeginFigure(D2D1::Point2F(coords[0], coords[1]), D2D1_FIGURE_BEGIN_HOLLOW);

								for (size_t uj = 1; uj < data.size(); uj++)
								{
									pSink->AddLine(D2D1::Point2F(coords[uj * 2 + 0], coords[uj * 2 + 1]));
								}
								pSink->EndFigure(D2D1_FIGURE_END_OPEN);
								hr = pSink->Close();
								pSink->Release();
								pSink = NULL;
							}
						}
						m_Polygons.insert(std::make_pair(hash, pPathGeometry));
						m_pRenderTarget->FillGeometry(pPathGeometry, GetColorBrush(polygon->GetFillStyle()->GetFillColor()));
						if (hasBorder) m_pRenderTarget->DrawGeometry(pPathGeometry, GetColorBrush(polygon->GetBorderStyle()->GetColor()), borderWidth, GetStrokeStyle(polygon->GetBorderStyle()->GetDash()));
					}
					delete coords;
				}
			}
			else if (dynamic_cast<RectanglePrimitive*>(primitive) != NULL)
			{
				RectanglePrimitive* rectangle = dynamic_cast<RectanglePrimitive*>(primitive);
				D2D1_RECT_F rect = D2D1::RectF(x + projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX() - centerX),
					y + projection.ConvertWidthToPixel(maxY - rectangle->GetTopLeft().GetY() - centerY),
					x + projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX() - centerX) + projection.ConvertWidthToPixel(rectangle->GetWidth()),
					y + projection.ConvertWidthToPixel(maxY - rectangle->GetTopLeft().GetY() - centerY) + projection.ConvertWidthToPixel(rectangle->GetHeight()));
				m_pRenderTarget->FillRectangle(rect, GetColorBrush(fillStyle->GetFillColor()));
				if (hasBorder) m_pRenderTarget->DrawRectangle(rect, GetColorBrush(borderStyle->GetColor()), borderWidth, GetStrokeStyle(borderStyle->GetDash()));
			}
			else if (dynamic_cast<CirclePrimitive*>(primitive) != NULL)
			{
				CirclePrimitive* circle = dynamic_cast<CirclePrimitive*>(primitive);
				D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(centerX, centerY), projection.ConvertWidthToPixel(circle->GetRadius()), projection.ConvertWidthToPixel(circle->GetRadius()));
				m_pRenderTarget->FillEllipse(ellipse, GetColorBrush(fillStyle->GetFillColor()));
				if (hasBorder) m_pRenderTarget->DrawEllipse(ellipse, GetColorBrush(borderStyle->GetColor()), borderWidth, GetStrokeStyle(borderStyle->GetDash()));
			}
			else
			{
			}
		}
	}

	void MapPainterDirectX::DrawPath(const Projection& projection, const MapParameter& parameter, const Color& color, double width, const std::vector<double>& dash, LineStyle::CapStyle startCap, LineStyle::CapStyle endCap, size_t transStart, size_t transEnd)
	{
		uint64_t hash = (((uint32_t)transStart) << 32) | ((uint32_t)transEnd);
		GeometryMap::const_iterator g = m_Geometries.find(hash);
		if (g != m_Geometries.end()) {
			m_pRenderTarget->DrawGeometry(g->second, GetColorBrush(color), width, GetStrokeStyle(dash));
		}
		else
		{
			ID2D1PathGeometry* pPathGeometry;
			HRESULT hr = m_pDirect2dFactory->CreatePathGeometry(&pPathGeometry);
			if (SUCCEEDED(hr))
			{
				ID2D1GeometrySink *pSink = NULL;
				hr = pPathGeometry->Open(&pSink);
				if (SUCCEEDED(hr))
				{
					pSink->BeginFigure(D2D1::Point2F(coordBuffer->buffer[transStart].GetX(), coordBuffer->buffer[transStart].GetY()), D2D1_FIGURE_BEGIN_HOLLOW);

					for (size_t i = transStart + 1; i <= transEnd; i++) {
						pSink->AddLine(D2D1::Point2F(coordBuffer->buffer[i].GetX(), coordBuffer->buffer[i].GetY()));
					}
					pSink->EndFigure(D2D1_FIGURE_END_OPEN);
					hr = pSink->Close();
					pSink->Release();
					pSink = NULL;
				}
			}
			m_pRenderTarget->DrawGeometry(m_Geometries.insert(std::make_pair(hash, pPathGeometry)).first->second, GetColorBrush(color), width, GetStrokeStyle(dash));
		}
	}

	void MapPainterDirectX::DrawContourLabel(const Projection& projection, const MapParameter& parameter, const PathTextStyle& style, const std::string& text, size_t transStart, size_t transEnd)
	{
		// Not implemented yet
	}

	void MapPainterDirectX::DrawContourSymbol(const Projection& projection, const MapParameter& parameter, const Symbol& symbol, double space, size_t transStart, size_t transEnd)
	{
		// Not implemented yet
	}

	void MapPainterDirectX::DrawArea(const Projection& projection, const MapParameter& parameter, const MapPainter::AreaData& area)
	{
		std::map<FillStyle, std::string>::const_iterator fillStyleNameEntry = fillStyleNameMap.find(*area.fillStyle);
		std::map<BorderStyle, std::string>::const_iterator borderStyleNameEntry = borderStyleNameMap.find(*area.borderStyle);

		assert(fillStyleNameEntry != fillStyleNameMap.end());
		assert(borderStyleNameEntry != borderStyleNameMap.end());
		bool hasBorder = borderStyleNameEntry->first.GetWidth() > 0.0 && borderStyleNameEntry->first.GetColor().IsVisible();
		double borderWidth = hasBorder ? projection.ConvertWidthToPixel(borderStyleNameEntry->first.GetWidth()) : 0.0;

		uint64_t hash = (((uint32_t)area.transStart) << 32) | ((uint32_t)area.transEnd);
		GeometryMap::const_iterator g = m_Geometries.find(hash);
		if (g == m_Geometries.end()) {
			ID2D1PathGeometry* pPathGeometry;
			HRESULT hr = m_pDirect2dFactory->CreatePathGeometry(&pPathGeometry);
			if (SUCCEEDED(hr))
			{
				ID2D1GeometrySink *pSink = NULL;
				hr = pPathGeometry->Open(&pSink);
				if (SUCCEEDED(hr))
				{
					pSink->BeginFigure(D2D1::Point2F(coordBuffer->buffer[area.transStart].GetX(), coordBuffer->buffer[area.transStart].GetY()), D2D1_FIGURE_BEGIN_FILLED);
					for (size_t i = area.transStart + 1; i <= area.transEnd; i++) {
						pSink->AddLine(D2D1::Point2F(coordBuffer->buffer[i].GetX(), coordBuffer->buffer[i].GetY()));
					}
					pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
					hr = pSink->Close();
					pSink->Release();
					pSink = NULL;
				}
			}
			g = m_Geometries.insert(std::make_pair(hash, pPathGeometry)).first;
		}
		m_pRenderTarget->FillGeometry(g->second, GetColorBrush(fillStyleNameEntry->first.GetFillColor()));
		if (hasBorder) m_pRenderTarget->DrawGeometry(g->second, GetColorBrush(borderStyleNameEntry->first.GetColor()), borderWidth, GetStrokeStyle(borderStyleNameEntry->first.GetDash()));
		for (std::list<PolyData>::const_iterator c = area.clippings.begin();
			c != area.clippings.end();
			c++) {
			const PolyData    &data = *c;
			hash = (((uint32_t)data.transStart) << 32) | ((uint32_t)data.transEnd);
			g = m_Geometries.find(hash);
			if (g == m_Geometries.end()) {
				ID2D1PathGeometry* pPathGeometry;
				HRESULT hr = m_pDirect2dFactory->CreatePathGeometry(&pPathGeometry);
				if (SUCCEEDED(hr))
				{
					ID2D1GeometrySink *pSink = NULL;
					hr = pPathGeometry->Open(&pSink);
					if (SUCCEEDED(hr))
					{
						pSink->BeginFigure(D2D1::Point2F(coordBuffer->buffer[data.transStart].GetX(), coordBuffer->buffer[data.transStart].GetY()), D2D1_FIGURE_BEGIN_FILLED);

						for (size_t i = data.transStart + 1; i <= data.transEnd; i++) {
							pSink->AddLine(D2D1::Point2F(coordBuffer->buffer[i].GetX(), coordBuffer->buffer[i].GetY()));
						}
						pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
						hr = pSink->Close();
						pSink->Release();
						pSink = NULL;
					}
				}
				g = m_Geometries.insert(std::make_pair(hash, pPathGeometry)).first;
			}
			m_pRenderTarget->FillGeometry(g->second, GetColorBrush(fillStyleNameEntry->first.GetFillColor()));
			if (hasBorder) m_pRenderTarget->DrawGeometry(g->second, GetColorBrush(borderStyleNameEntry->first.GetColor()), borderWidth, GetStrokeStyle(borderStyleNameEntry->first.GetDash()));
		}
	}

	MapPainterDirectX::MapPainterDirectX(const StyleConfigRef& styleConfig, ID2D1Factory* pDirect2dFactory, IDWriteFactory* pWriteFactory)
		: MapPainter(styleConfig, new CoordBufferImpl<Vertex2D>()),
		m_pDirect2dFactory(pDirect2dFactory),
		m_pWriteFactory(pWriteFactory),
		m_pRenderTarget(NULL),
		m_pImagingFactory(NULL),
		dpiX(0.0f),
		dpiY(0.0f),
		coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer),
		typeConfig(NULL)
	{
		pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&m_pImagingFactory);
		if (SUCCEEDED(hr))
		{
		}
		else
			m_pImagingFactory = NULL;
	}

	MapPainterDirectX::~MapPainterDirectX()
	{
		DiscardDeviceResources();
		for (FontMap::const_iterator entry = m_Fonts.begin(); entry != m_Fonts.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_Fonts[entry->first] = NULL;
			}
		}
		m_Fonts.clear();
		for (BitmapMap::const_iterator entry = m_Bitmaps.begin(); entry != m_Bitmaps.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_Bitmaps[entry->first] = NULL;
			}
		}
		m_Bitmaps.clear();
		for (StrokeStyleMap::const_iterator entry = m_StrokeStyles.begin(); entry != m_StrokeStyles.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_StrokeStyles[entry->first] = NULL;
			}
		}
		m_StrokeStyles.clear();
		if (m_pImagingFactory)
		{
			m_pImagingFactory->Release();
			m_pImagingFactory = NULL;
		}
	}

	void MapPainterDirectX::DiscardDeviceResources()
	{
		for (BrushMap::const_iterator entry = m_Brushs.begin(); entry != m_Brushs.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_Brushs[entry->first] = NULL;
			}
		}
		m_Brushs.clear();
		for (GeometryMap::const_iterator entry = m_Geometries.begin(); entry != m_Geometries.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_Geometries[entry->first] = NULL;
			}
		}
		m_Geometries.clear();
		for (GeometryMap::const_iterator entry = m_Polygons.begin(); entry != m_Polygons.end(); ++entry) {
			if (entry->second != NULL) {
				entry->second->Release();
				m_Polygons[entry->first] = NULL;
			}
		}
		m_Polygons.clear();
	}

	bool MapPainterDirectX::DrawMap(const Projection& projection, const MapParameter& parameter, const MapData& data, ID2D1RenderTarget* renderTarget)
	{
		if (m_pDirect2dFactory == NULL) return false;
		typeConfig = styleConfig->GetTypeConfig();
		m_pRenderTarget = renderTarget;

		Draw(projection, parameter, data);

		fillStyleNameMap.clear();
		lineStyleNameMap.clear();

		return true;
	}
}

