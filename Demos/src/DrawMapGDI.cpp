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
#include <osmscout/MapService.h>
#include <osmscout/MapPainterGDI.h>
#include <osmscout/MapPainterGDIWindow.h>

#include <Windowsx.h>
#include <tchar.h>

#ifndef DPI
#define DPI 96.f
#endif

class DrawMapGDI : public osmscout::MapPainterGDIWindow
{
private:
	osmscout::DatabaseParameter		m_databaseParameter;
	osmscout::DatabaseRef			m_database;
	osmscout::MapServiceRef			m_mapService;
	osmscout::MercatorProjection	m_Projection;
	osmscout::MapParameter			m_DrawParameter;
	osmscout::MapData				m_Data;
	osmscout::MapPainterGDI*	    m_Painter;
	osmscout::AreaSearchParameter	m_SearchParameter;
	osmscout::StyleConfigRef		m_StyleConfig;
	std::list<osmscout::TileRef>	m_Tiles;

	std::string						m_szMap;
	std::string						m_szStyle;
	double							m_fLongitude;
	double							m_fLatitude;
	double							m_fZoom;

public:
	DrawMapGDI(std::string map, std::string style, double lon, double lat, double zoom)
		: m_szMap(map)
		, m_szStyle(style)
		, m_fLongitude(lon)
		, m_fLatitude(lat)
		, m_fZoom(zoom)
		, osmscout::MapPainterGDIWindow()
	{
	}

	~DrawMapGDI()
	{
	}

	bool DrawMapGDI::initialize(HINSTANCE hInstance, int nShowCmd)
	{
		// Init osmscout
		m_database = std::make_shared<osmscout::Database>(m_databaseParameter);
		if (!m_database->Open(m_szMap.c_str()))
		{
			MessageBox(NULL, _T("Cannot open database"), _T("DrawMapGDI"), MB_OK | MB_ICONERROR);
			return false;
		}
		m_mapService = std::make_shared<osmscout::MapService>(m_database);
		m_StyleConfig = std::make_shared<osmscout::StyleConfig>(m_database->GetTypeConfig());
		if (!m_StyleConfig->Load(m_szStyle))
		{
			MessageBox(NULL, _T("Cannot open style"), _T("DrawMapGDI"), MB_OK | MB_ICONERROR);
			return false;
		}

		m_DrawParameter.SetFontName("sans-serif");
		m_DrawParameter.SetFontSize(3.0);
#ifdef NDEBUG
		m_DrawParameter.SetDebugPerformance(false);
#else
		m_DrawParameter.SetDebugPerformance(true);
#endif

		m_Projection.Set(osmscout::GeoCoord(m_fLatitude, m_fLongitude), osmscout::Magnification(m_fZoom), DPI, 800, 600);
		m_mapService->LookupTiles(m_Projection, m_Tiles);
		m_mapService->LoadMissingTileData(m_SearchParameter, *m_StyleConfig, m_Tiles);
		m_mapService->AddTileDataToMapData(m_Tiles, m_Data);

		RECT size = { 0, 0, 800, 600 };
		if (!CreateCanvas(m_StyleConfig, size, NULL, hInstance)) return false;
		Set(&m_Projection, &m_DrawParameter, &m_Data);
		return true;
	}

	int DrawMapGDI::run()
	{
		MSG msg = { };
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return EXIT_SUCCESS;
	}

	virtual LRESULT OnWinMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		default:
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_MOUSEWHEEL:
			m_Projection.Set(m_Projection.GetCenter(), osmscout::Magnification(m_Projection.GetMagnification().GetMagnification() + 100.0 * GET_WHEEL_DELTA_WPARAM(wParam)), m_Projection.GetDPI(), m_Projection.GetWidth(), m_Projection.GetHeight());
			OnTileUpdate();
			InvalidateWindow();
			break;
		}
		return MapPainterGDIWindow::OnWinMsg(hwnd, uMsg, wParam, lParam);
	}

	virtual void OnTileUpdate()
	{
		m_mapService->LookupTiles(m_Projection, m_Tiles);
		m_mapService->LoadMissingTileData(m_SearchParameter, *m_StyleConfig, m_Tiles);
		m_mapService->AddTileDataToMapData(m_Tiles, m_Data);
	}
};

int app_main(HINSTANCE hinstance, int nShowCmd)
{
	int argc = 0;
	LPWSTR* w_argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc != 6)
	{
		if (w_argv) LocalFree(w_argv);
		MessageBox(NULL, _T("Arguments requied!\n\nDrawMapGDI <map directory> <style-file> <lat> <lon> <zoom>"), _T("DrawMapGDI"), MB_OK | MB_ICONERROR);
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
	double lat, lon, zoom;
	if (sscanf(argv[3].c_str(), "%lf", &lat) != 1) {
		MessageBox(NULL, _T("lat is not numeric!"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (sscanf(argv[4].c_str(), "%lf", &lon) != 1) {
		MessageBox(NULL, _T("lon is not numeric!"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (sscanf(argv[5].c_str(), "%lf", &zoom) != 1) {
		MessageBox(NULL, _T("zoom is not numeric!"), _T("DrawMapDirectX"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	DrawMapGDI app(map, style, lon, lat, zoom);
	if (!app.initialize(hinstance, nShowCmd)) return EXIT_FAILURE;
	return app.run();
}

#ifdef _MSC_VER
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nShowCmd)
{
	return app_main(hinstance, nShowCmd);
}
#else
int main(int argc, char *argv[])
{
	return app_main(GetModuleHandle(NULL), SW_SHOW);
}
#endif