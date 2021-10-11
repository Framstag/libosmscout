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

src/DrawMapDirectX ../maps/nordrhein-westfalen ../stylesheets/standard.oss 51.51241 7.46525 70000
*/

// Application based on https://msdn.microsoft.com/en-us/library/windows/desktop/dd370994

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // msvc issue with std::max/min
#include <windows.h>
#include <tchar.h>
#include <cmath>
#include <shellapi.h>

#include <d2d1.h>
#include <dwrite.h>

#include <iostream>

#include <osmscout/Database.h>

#include <osmscoutmap/MapService.h>

#include <osmscout/MapPainterDirectX.h>

#include <DrawMap.h>

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

	osmscout::MapPainterDirectX*	m_Painter;
	std::list<osmscout::TileRef>	m_Tiles;
	DrawMapDemo*                    m_pBaseData;

public:
	explicit DrawMapDirectX(DrawMapDemo* pDemoData) :
		m_hwnd(NULL),
		m_pDirect2dFactory(NULL),
		m_pWriteFactory(NULL),
		m_pRenderTarget(NULL),
		m_pLightSlateGrayBrush(NULL),
		m_pCornflowerBlueBrush(NULL),
		m_Painter(NULL),
		m_pBaseData(pDemoData)
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

		if (!SUCCEEDED(hr))
		{
			MessageBox(m_hwnd, _T("Cannot create device independent resources"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
			return E_FAIL;
		}

		std::cout << "Device independent resources created." << std::endl;

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

		Arguments args = m_pBaseData->GetArguments();

		// Create the window.
		m_hwnd = CreateWindow(
			_T("DemoDrawMapDirectX"),
			_T("DrawMapDirectX"),
			WS_OPERLAPPEDWINDOW ^ WS_THICKFRAME,
			(GetSystemMetrics(SM_CXSCREEN) - args.width) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - args.height) / 2,
			args.width,
			args.height,
			NULL,
			NULL,
			HINST_THISCOMPONENT,
			this
		);

		hr = m_hwnd ? S_OK : E_FAIL;

		if (!SUCCEEDED(hr))
		{
			MessageBox(m_hwnd, _T("Cannot create window"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
			return E_FAIL;
		}

		std::cout << "Window created." << std::endl;

		ShowWindow(m_hwnd, SW_SHOWNORMAL);
		UpdateWindow(m_hwnd);

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

			/*
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
			*/

			if (m_Painter != NULL)
			{
				m_Painter->DrawMap(m_pBaseData->projection, m_pBaseData->drawParameter, m_pBaseData->data, m_pRenderTarget);
			}
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
			m_pBaseData->projection.Set(m_pBaseData->projection.GetCenter(), m_pBaseData->projection.GetMagnification(), m_pBaseData->projection.GetDPI(), width, height);
			m_pBaseData->mapService->LookupTiles(m_pBaseData->projection, m_Tiles);
			m_pBaseData->mapService->LoadMissingTileData(m_pBaseData->searchParameter, *m_pBaseData->styleConfig, m_Tiles);
			m_pBaseData->mapService->AddTileDataToMapData(m_Tiles, m_pBaseData->data);
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
				(LONG_PTR)pDemoApp
			);

			result = 1;

			pDemoApp->m_Painter = new osmscout::MapPainterDirectX(pDemoApp->m_pBaseData->styleConfig, pDemoApp->m_pDirect2dFactory, pDemoApp->m_pWriteFactory);
		}
		else
		{
			DrawMapDirectX *pDemoApp = reinterpret_cast<DrawMapDirectX*>(static_cast<LONG_PTR>(
				::GetWindowLongPtrW(
					hWnd,
					GWLP_USERDATA
				)
			));

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

int app_main(int argc, char* argv[])
{
	DrawMapDemo drawDemo("DrawMapDirectX", argc, argv, DPI, ARG_WS_WINDOW);

	std::streambuf* oldCerrStreamBuf = std::cerr.rdbuf();
	std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
	std::ostringstream strCerr, strCout;
	std::cerr.rdbuf(strCerr.rdbuf());
	std::cout.rdbuf(strCout.rdbuf());

	if (!drawDemo.OpenDatabase()) {
		bool bHelp = drawDemo.GetArguments().help;
		MessageBoxA(NULL, bHelp ? strCout.str().c_str() : strCerr.str().c_str(), "DrawMapDirectX", MB_OK | (bHelp ? MB_ICONINFORMATION : MB_ICONERROR));
		std::cerr.rdbuf(oldCerrStreamBuf);
		std::cout.rdbuf(oldCoutStreamBuf);
		return EXIT_FAILURE;
	}
	std::cerr.rdbuf(oldCerrStreamBuf);
	std::cout.rdbuf(oldCoutStreamBuf);

	drawDemo.LoadData();

	// Use HeapSetInformation to specify that the process should
	// terminate if the heap manager detects an error in any heap used
	// by the process.
	// The return value is ignored, because we want to continue running in the
	// unlikely event that HeapSetInformation fails.
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			DrawMapDirectX app(&drawDemo);

			if (SUCCEEDED(app.Initialize())) {
				app.RunMessageLoop();
			}
		}

		CoUninitialize();
	}

	return EXIT_SUCCESS;
}

#ifdef _MSC_VER
int WINAPI WinMain(HINSTANCE /*hinstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
	int argc = 0;
	LPWSTR* w_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char** argv = NULL;
	if (w_argv)
	{
		argv = new char*[argc];
		for (int i = 0; i < argc; ++i)
		{
			int w_len = lstrlenW(w_argv[i]);
			int len = WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, NULL, 0, NULL, NULL);
			argv[i] = new char[len + 1];
			WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, argv[i], len, NULL, NULL);
			argv[i][len] = 0;
		}
		LocalFree(w_argv);
	}

	int result = app_main(argc, argv);

	if (argv != NULL)
	{
		for (int i = 0; i < argc; i++) delete argv[i];
		delete argv;
	}

	return result;
}
#else
int main(int argc, char *argv[])
{
	return app_main(argc, argv);
}
#endif
