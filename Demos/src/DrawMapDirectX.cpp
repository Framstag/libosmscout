/*
DrawMapDirectX - a demo program for libosmscout
Copyright (C) 2016  Tim Teulings

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
Example for the nordrhein-westfalen.osm (to be executed in the Demos top
level directory):

src/DrawMapDirectX ../maps/nordrhein-westfalen ../stylesheets/standard.oss 7.46525 51.51241 70000
*/

// Application based on https://msdn.microsoft.com/en-us/library/windows/desktop/dd370994

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <cmath>
#include <shellapi.h>

#include <d2d1.h>
#include <dwrite.h>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>
#include <osmscout/MapPainterDirectX.h>

#define WINDOW_CLASS_NAME _T("DemoDrawMapDirectX")
#ifndef WS_OPERLAPPEDWINDOW
#define WS_OPERLAPPEDWINDOW WS_TILEDWINDOW
#endif

#ifndef DPI
#define DPI 96.f
#endif

template<class Interface>
inline void SafeRelease(
	Interface **ppInterfaceToRelease
	)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class DrawMapDirectX
{
private:
	HWND							m_hwnd;
	ID2D1Factory*					m_pDirect2dFactory;
	IDWriteFactory*					m_pWriteFactory;
	ID2D1HwndRenderTarget*			m_pRenderTarget;
	ID2D1SolidColorBrush*			m_pLightSlateGrayBrush;
	ID2D1SolidColorBrush*			m_pCornflowerBlueBrush;

	osmscout::DatabaseParameter		m_databaseParameter;
	osmscout::DatabaseRef			m_database;
	osmscout::MapServiceRef			m_mapService;
	osmscout::MercatorProjection	m_Projection;
	osmscout::MapParameter			m_DrawParameter;
	osmscout::MapData				m_Data;
	osmscout::MapPainterDirectX*	m_Painter;
	osmscout::AreaSearchParameter	m_SearchParameter;
	osmscout::StyleConfigRef		m_StyleConfig;
	std::list<osmscout::TileRef>	m_Tiles;

	std::string						m_szMap;
	std::string						m_szStyle;
	double							m_fLongitude;
	double							m_fLatitude;
	double							m_fZoom;

public:
	DrawMapDirectX(std::string map = "", std::string style = "", double lon = 0.0, double lat = 0.0, double zoom = 0.0) :
		m_hwnd(NULL),
		m_pDirect2dFactory(NULL),
		m_pWriteFactory(NULL),
		m_pRenderTarget(NULL),
		m_pLightSlateGrayBrush(NULL),
		m_pCornflowerBlueBrush(NULL),
		m_Painter(NULL),
		m_szMap(map),
		m_szStyle(style),
		m_fLongitude(lon),
		m_fLatitude(lat),
		m_fZoom(zoom)
	{
	}

	~DrawMapDirectX()
	{
		if (m_Painter != NULL)
		{
			delete m_Painter;
			m_Painter = NULL;
		}
		SafeRelease(&m_pWriteFactory);
		SafeRelease(&m_pDirect2dFactory);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pLightSlateGrayBrush);
		SafeRelease(&m_pCornflowerBlueBrush);
	}

	// Register the window class and call methods for instantiating drawing resources
	HRESULT Initialize()
	{
		HRESULT hr;

		// Initialize device-indpendent resources, such
		// as the Direct2D factory.
		hr = CreateDeviceIndependentResources();

		if (SUCCEEDED(hr))
		{
			// Register the window class.
			WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = DrawMapDirectX::WndProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = sizeof(LONG_PTR);
			wcex.hInstance = HINST_THISCOMPONENT;
			wcex.hbrBackground = NULL;
			wcex.lpszMenuName = NULL;
			wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
			wcex.lpszClassName = _T("DemoDrawMapDirectX");

			RegisterClassEx(&wcex);


			// Because the CreateWindow function takes its size in pixels,
			// obtain the system DPI and use it to scale the window size.
			FLOAT dpiX, dpiY;

			// The factory returns the current system DPI. This is also the value it will use
			// to create its own windows.
			m_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);

			// Init osmscout
			m_database = osmscout::DatabaseRef(new osmscout::Database(m_databaseParameter));
			if (!m_database->Open(m_szMap.c_str()))
			{
				MessageBox(m_hwnd, _T("Cannot open database"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
				return E_FAIL;
			}
			m_mapService = osmscout::MapServiceRef(new osmscout::MapService(m_database));
			m_StyleConfig = osmscout::StyleConfigRef(new osmscout::StyleConfig(m_database->GetTypeConfig()));
			if (!m_StyleConfig->Load(m_szStyle))
			{
				MessageBox(m_hwnd, _T("Cannot open style"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
				return E_FAIL;
			}

			// Create the window.
			m_hwnd = CreateWindow(
				_T("DemoDrawMapDirectX"),
				_T("DrawMapDirectX"),
				WS_OPERLAPPEDWINDOW ^ WS_THICKFRAME,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				static_cast<UINT>(std::ceil(800.f * dpiX / DPI)),
				static_cast<UINT>(std::ceil(600.f * dpiY / DPI)),
				NULL,
				NULL,
				HINST_THISCOMPONENT,
				this
				);
			hr = m_hwnd ? S_OK : E_FAIL;
			if (SUCCEEDED(hr))
			{
				m_DrawParameter.SetFontName("sans-serif");
				m_DrawParameter.SetFontSize(3.0);
				m_DrawParameter.SetDebugPerformance(true);

				m_Projection.Set(osmscout::GeoCoord(m_fLatitude, m_fLongitude), osmscout::Magnification(m_fZoom), DPI, 800, 600);
				m_mapService->LookupTiles(m_Projection, m_Tiles);
				m_mapService->LoadMissingTileData(m_SearchParameter, *m_StyleConfig, m_Tiles);
				m_mapService->AddTileDataToMapData(m_Tiles, m_Data);

				ShowWindow(m_hwnd, SW_SHOWNORMAL);
				UpdateWindow(m_hwnd);
			}
		}

		return hr;
	}

	// Process and dispatch messages
	void RunMessageLoop()
	{
		MSG msg;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

private:
	// Initialize device-independent resources.
	HRESULT CreateDeviceIndependentResources()
	{
		HRESULT hr = S_OK;

		// Create a Direct2D factory.
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);

		if (SUCCEEDED(hr))
		{
			// Create a DirectWrite factory.
			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pWriteFactory), reinterpret_cast<IUnknown **>(&m_pWriteFactory));
		}

		return hr;
	}

	// Initialize device-dependent resources.
	HRESULT CreateDeviceResources()
	{
		HRESULT hr = S_OK;

		if (!m_pRenderTarget)
		{
			RECT rc;
			GetClientRect(m_hwnd, &rc);

			D2D1_SIZE_U size = D2D1::SizeU(
				rc.right - rc.left,
				rc.bottom - rc.top
				);

			// Create a Direct2D render target.
			hr = m_pDirect2dFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(m_hwnd, size),
				&m_pRenderTarget
				);


			if (SUCCEEDED(hr))
			{
				// Create a gray brush.
				hr = m_pRenderTarget->CreateSolidColorBrush(
					D2D1::ColorF(D2D1::ColorF::LightSlateGray),
					&m_pLightSlateGrayBrush
					);
			}
			if (SUCCEEDED(hr))
			{
				// Create a blue brush.
				hr = m_pRenderTarget->CreateSolidColorBrush(
					D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
					&m_pCornflowerBlueBrush
					);
			}
		}

		return hr;
	}

	// Release device-dependent resource.
	void DiscardDeviceResources()
	{
		m_Painter->DiscardDeviceResources();
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pLightSlateGrayBrush);
		SafeRelease(&m_pCornflowerBlueBrush);
	}

	// Draw content.
	HRESULT OnRender()
	{
		HRESULT hr = S_OK;
		hr = CreateDeviceResources();
		if (SUCCEEDED(hr))
		{
			m_pRenderTarget->BeginDraw();

			m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

			m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
			D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();
			// Draw a grid background.
			int width = static_cast<int>(rtSize.width);
			int height = static_cast<int>(rtSize.height);

			for (int x = 0; x < width; x += 10)
			{
				m_pRenderTarget->DrawLine(
					D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
					D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height),
					m_pLightSlateGrayBrush,
					0.5f
					);
			}

			for (int y = 0; y < height; y += 10)
			{
				m_pRenderTarget->DrawLine(
					D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
					D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)),
					m_pLightSlateGrayBrush,
					0.5f
					);
			}
			// Draw two rectangles.
			D2D1_RECT_F rectangle1 = D2D1::RectF(
				rtSize.width / 2 - 50.0f,
				rtSize.height / 2 - 50.0f,
				rtSize.width / 2 + 50.0f,
				rtSize.height / 2 + 50.0f
				);

			D2D1_RECT_F rectangle2 = D2D1::RectF(
				rtSize.width / 2 - 100.0f,
				rtSize.height / 2 - 100.0f,
				rtSize.width / 2 + 100.0f,
				rtSize.height / 2 + 100.0f
				);
			// Draw a filled rectangle.
			m_pRenderTarget->FillRectangle(&rectangle1, m_pLightSlateGrayBrush);
			// Draw the outline of a rectangle.
			m_pRenderTarget->DrawRectangle(&rectangle2, m_pCornflowerBlueBrush);

			if (m_Painter != NULL)
				m_Painter->DrawMap(m_Projection, m_DrawParameter, m_Data, m_pRenderTarget);

			hr = m_pRenderTarget->EndDraw();
		}
		if (hr == D2DERR_RECREATE_TARGET)
		{
			hr = S_OK;
			DiscardDeviceResources();
		}
		return hr;
	}

	// Resize the render target.
	void OnResize(UINT width, UINT height)
	{
		if (m_pRenderTarget)
		{
			// Note: This method can fail, but it's okay to ignore the
			// error here, because the error will be returned again
			// the next time EndDraw is called.
			m_pRenderTarget->Resize(D2D1::SizeU(width, height));
		}
		if (width > 0 && height > 0)
		{
			m_Projection.Set(osmscout::GeoCoord(m_fLatitude, m_fLongitude), osmscout::Magnification(m_fZoom), DPI, width, height);
			m_mapService->LookupTiles(m_Projection, m_Tiles);
			m_mapService->LoadMissingTileData(m_SearchParameter, *m_StyleConfig, m_Tiles);
			m_mapService->AddTileDataToMapData(m_Tiles, m_Data);
		}
	}

	// The windows procedure.
	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
		)
	{
		LRESULT result = 0;

		if (message == WM_CREATE)
		{
			LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
			DrawMapDirectX *pDemoApp = (DrawMapDirectX*)pcs->lpCreateParams;

			::SetWindowLongPtrW(
				hWnd,
				GWLP_USERDATA,
				PtrToUlong(pDemoApp)
				);

			result = 1;

			pDemoApp->m_Painter = new osmscout::MapPainterDirectX(pDemoApp->m_StyleConfig, pDemoApp->m_pDirect2dFactory, pDemoApp->m_pWriteFactory);
		}
		else
		{
			DrawMapDirectX *pDemoApp = reinterpret_cast<DrawMapDirectX*>(static_cast<LONG_PTR>(
				::GetWindowLongPtrW(
				hWnd,
				GWLP_USERDATA
				)));

			bool wasHandled = false;

			if (pDemoApp)
			{
				switch (message)
				{
				case WM_SIZE:
				{
					UINT width = LOWORD(lParam);
					UINT height = HIWORD(lParam);
					pDemoApp->OnResize(width, height);
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_DISPLAYCHANGE:
				{
					InvalidateRect(hWnd, NULL, FALSE);
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_PAINT:
				{
					pDemoApp->OnRender();
					ValidateRect(hWnd, NULL);
				}
				result = 0;
				wasHandled = true;
				break;

				case WM_DESTROY:
				{
					PostQuitMessage(0);
				}
				result = 1;
				wasHandled = true;
				break;
				}
			}

			if (!wasHandled)
			{
				result = DefWindowProc(hWnd, message, wParam, lParam);
			}
		}

		return result;
	}
};

int WINAPI WinMain(
	HINSTANCE /* hInstance */,
	HINSTANCE /* hPrevInstance */,
	LPSTR /* lpCmdLine */,
	int /* nCmdShow */
	)
{
	int argc = 0;
	LPWSTR* w_argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc != 6)
	{
		if (w_argv) LocalFree(w_argv);
		MessageBox(NULL, _T("Arguments requied!\n\nDrawMapDirectX <map directory> <style-file> <lon> <lat> <zoom>"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	std::vector<std::string> argv;
	if (w_argv)
	{
		for (int i = 0; i < argc; ++i)
		{
			int w_len = lstrlenW(w_argv[i]);
			int len = WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, NULL, 0, NULL, NULL);
			std::string s(len, 0);
			WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, &s[0], len, NULL, NULL);
			argv.push_back(s);
		}
		LocalFree(w_argv);
	}

	std::string map = argv[1];
	std::string style = argv[2];
	double lon, lat, zoom;
	if (sscanf(argv[3].c_str(), "%lf", &lon) != 1) {
		MessageBox(NULL, _T("lon is not numeric!"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (sscanf(argv[4].c_str(), "%lf", &lat) != 1) {
		MessageBox(NULL, _T("lat is not numeric!"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (sscanf(argv[5].c_str(), "%lf", &zoom) != 1) {
		MessageBox(NULL, _T("zoom is not numeric!"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	// Use HeapSetInformation to specify that the process should
	// terminate if the heap manager detects an error in any heap used
	// by the process.
	// The return value is ignored, because we want to continue running in the
	// unlikely event that HeapSetInformation fails.
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			DrawMapDirectX app(map, style, lon, lat, zoom);

			if (SUCCEEDED(app.Initialize()))
			{
				app.RunMessageLoop();
			}
		}
		CoUninitialize();
	}

	return EXIT_SUCCESS;
}
