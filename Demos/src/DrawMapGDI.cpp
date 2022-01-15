/*
DrawMapGDI - a demo program for libosmscout
Copyright (C) 2020 Transporter

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

src/DrawMapGDI ../maps/nordrhein-westfalen ../stylesheets/standard.oss 51.51241 7.46525 70000
*/

#include <iostream>

#include <osmscout/Database.h>

#include <osmscoutmap/MapService.h>

#include <osmscoutmapgdi/MapPainterGDI.h>
#include <osmscoutmapgdi/MapPainterGDIWindow.h>

#include <DrawMap.h>

#include <Windowsx.h>
#include <tchar.h>

#ifndef DPI
#define DPI 96.f
#endif

class DrawMapGDI : public osmscout::MapPainterGDIWindow
{
private:
	osmscout::MapPainterGDI*	    m_Painter;
	std::list<osmscout::TileRef>	m_Tiles;
	DrawMapDemo*                    m_pBaseData;

public:
	explicit DrawMapGDI(DrawMapDemo* pBaseData)
		: m_Painter(nullptr)
		, m_pBaseData(pBaseData)
	{
	}

	bool initialize(HINSTANCE hInstance, int  /*nShowCmd*/)
	{
		Arguments args = m_pBaseData->GetArguments();
		RECT size = {
			(GetSystemMetrics(SM_CXSCREEN) - args.width) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - args.height) / 2,
			(GetSystemMetrics(SM_CXSCREEN) + args.width) / 2,
			(GetSystemMetrics(SM_CYSCREEN) + args.height) / 2
		};

        if (!CreateCanvas(m_pBaseData->styleConfig, size, nullptr, hInstance))
        {
            return false;
        }

		Set(&m_pBaseData->projection, &m_pBaseData->drawParameter, &m_pBaseData->data);
		return true;
	}

	int run()
	{
		MSG msg = { };
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return EXIT_SUCCESS;
	}

	LRESULT OnWinMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		default:
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

        case WM_KEYDOWN:
            if (wParam == VK_OEM_MINUS)
            {
                osmscout::log.Info() << "Zoom out...";

                double newMagnification=m_pBaseData->projection.GetMagnification().GetMagnification() / 2;

                if (newMagnification<1.0) {
                    newMagnification=1.0;
                }

                osmscout::log.Info() << " Change magnification from " << m_pBaseData->projection.GetMagnification().GetMagnification() << " to " << newMagnification;

                m_pBaseData->projection.Set(m_pBaseData->projection.GetCenter(),
                                            osmscout::Magnification(newMagnification),
                                            m_pBaseData->projection.GetDPI(),
                                            m_pBaseData->projection.GetWidth(),
                                            m_pBaseData->projection.GetHeight());
                OnTileUpdate();
                InvalidateWindow();

            }
            else if (wParam == VK_OEM_PLUS)
            {
                osmscout::log.Info() << "Zoom in...";

                double newMagnification=m_pBaseData->projection.GetMagnification().GetMagnification()*2;

                osmscout::log.Info() << " Change magnification from " << m_pBaseData->projection.GetMagnification().GetMagnification() << " to " << newMagnification;

                m_pBaseData->projection.Set(m_pBaseData->projection.GetCenter(),
                                            osmscout::Magnification(newMagnification),
                                            m_pBaseData->projection.GetDPI(),
                                            m_pBaseData->projection.GetWidth(),
                                            m_pBaseData->projection.GetHeight());
                OnTileUpdate();
                InvalidateWindow();
            }
            break;

		case WM_MOUSEWHEEL:
            osmscout::log.Info() << "WM_MOUSEWHEEL...";

            double newMagnification=m_pBaseData->projection.GetMagnification().GetMagnification() + 100.0 * GET_WHEEL_DELTA_WPARAM(wParam);

            if (newMagnification<1.0) {
                newMagnification=1.0;
            }

            osmscout::log.Info() << " Change magnification from " << m_pBaseData->projection.GetMagnification().GetMagnification() << " to " << newMagnification;

            m_pBaseData->projection.Set(m_pBaseData->projection.GetCenter(),
                                        osmscout::Magnification(newMagnification),
                                        m_pBaseData->projection.GetDPI(),
                                        m_pBaseData->projection.GetWidth(),
                                        m_pBaseData->projection.GetHeight());
			OnTileUpdate();
			InvalidateWindow();
            osmscout::log.Info() << "WM_MOUSEWHEEL done";
			break;
		}
		return MapPainterGDIWindow::OnWinMsg(hwnd, uMsg, wParam, lParam);
	}

	void OnTileUpdate() override
	{
        osmscout::log.Info() << "OnTileUpdate()...";
        m_pBaseData->data.ClearDBData();
		m_pBaseData->mapService->LookupTiles(m_pBaseData->projection, m_Tiles);
		m_pBaseData->mapService->LoadMissingTileData(m_pBaseData->searchParameter, *m_pBaseData->styleConfig, m_Tiles);
		m_pBaseData->mapService->AddTileDataToMapData(m_Tiles, m_pBaseData->data);
        osmscout::log.Info() << "Nodes: "  << m_pBaseData->data.nodes.size();
        osmscout::log.Info() << "Ways: "  << m_pBaseData->data.ways.size();
        osmscout::log.Info() << "Areas: "  << m_pBaseData->data.areas.size();
        osmscout::log.Info() << "OnTileUpdate() done";
	}
};

int app_main(int argc, char *argv[], HINSTANCE hinstance, int nShowCmd)
{
	DrawMapDemo drawDemo("DrawMapGDI", argc, argv, DPI, ARG_WS_WINDOW);

	std::streambuf* oldCerrStreamBuf = std::cerr.rdbuf();
	std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
	std::ostringstream strCerr, strCout;
	std::cerr.rdbuf(strCerr.rdbuf());
	std::cout.rdbuf(strCout.rdbuf());

	if (!drawDemo.OpenDatabase()) {
		bool bHelp = drawDemo.GetArguments().help;
		MessageBoxA(nullptr, bHelp ? strCout.str().c_str() : strCerr.str().c_str(), "DrawMapGDI", MB_OK | (bHelp ? MB_ICONINFORMATION : MB_ICONERROR));
		std::cerr.rdbuf(oldCerrStreamBuf);
		std::cout.rdbuf(oldCoutStreamBuf);
		return EXIT_FAILURE;
	}
	std::cerr.rdbuf(oldCerrStreamBuf);
	std::cout.rdbuf(oldCoutStreamBuf);

	drawDemo.LoadData();

	DrawMapGDI app(&drawDemo);

    if (!app.initialize(hinstance, nShowCmd))
    {
        return EXIT_FAILURE;
    }

	return app.run();
}

#ifdef _MSC_VER
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nShowCmd)
{
	AllocConsole();

	FILE* fDummy;

	freopen_s(&fDummy,"CONOUT$","w",stdout);
	freopen_s(&fDummy,"CONOUT$","w",stderr);
	freopen_s(&fDummy,"CONIN$","r",stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	int argc = 0;
	LPWSTR* w_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char** argv = nullptr;
	if (w_argv != nullptr)
	{
		argv = new char*[argc];
		for (int i = 0; i < argc; ++i)
		{
			int w_len = lstrlenW(w_argv[i]);
			int len = WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, nullptr, 0, nullptr, nullptr);
			argv[i] = new char[len + 1];
			WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, argv[i], len, nullptr, nullptr);
			argv[i][len] = 0;
		}
		LocalFree(w_argv);
}

	int result = app_main(argc, argv, hinstance, nShowCmd);

	if (argv != nullptr)
	{
		for (int i = 0; i < argc; i++)
        {
            delete argv[i];
        }

		delete argv;
	}

	return result;
}
#else
int main(int argc, char *argv[])
{
	return app_main(argc, argv, GetModuleHandle(NULL), SW_SHOW);
}
#endif
