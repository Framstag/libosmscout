/*
  This source is part of the libosmscout-map-gdi library
  Copyright (C) 2020 Transporter

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

#include <osmscout/MapPainterGDIWindow.h>

namespace osmscout {

	static LRESULT CALLBACK _WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_NCCREATE)
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT(lParam))->lpCreateParams));
		}
		MapPainterGDIWindow* pWnd = (MapPainterGDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (pWnd)
			return pWnd->WinMsgHandler(hwnd, uMsg, wParam, lParam);
		else
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	MapPainterGDIWindow::MapPainterGDIWindow()
		: m_hInstance(NULL)
		, m_hWnd(NULL)
		, m_hWndParent(NULL)
		, m_pPainter(NULL)
		, m_pProjection(NULL)
		, m_pParameter(NULL)
		, m_pData(NULL)
	{
	}

	MapPainterGDIWindow::MapPainterGDIWindow(const StyleConfigRef& styleConfig, RECT position, HWND hWndParent, HINSTANCE hInstance)
		: m_hInstance(hInstance)
		, m_hWnd(NULL)
		, m_hWndParent(NULL)
		, m_pPainter(NULL)
		, m_pProjection(NULL)
		, m_pParameter(NULL)
		, m_pData(NULL)
	{
		CreateCanvas(styleConfig, position, hWndParent, hInstance);
	}

	MapPainterGDIWindow::~MapPainterGDIWindow()
	{
		if (m_pPainter != NULL)
		{
			delete m_pPainter;
			m_pPainter = NULL;
		}
	}

	bool MapPainterGDIWindow::CreateCanvas(const StyleConfigRef& styleConfig, RECT position, HWND hWndParent, HINSTANCE hInstance)
	{
		if (m_hWnd != NULL) return true;
		if (m_hInstance == NULL) m_hInstance = GetModuleHandle(NULL);
		if (m_pPainter == NULL) m_pPainter = new MapPainterGDI(styleConfig);
		const wchar_t CLASS_NAME[] = L"MapPainterGDIWindow";
		WNDCLASSEX wcx;
		memset(&wcx, 0, sizeof(WNDCLASSEX));
		wcx.cbSize = sizeof(WNDCLASSEX);
		wcx.style = CS_HREDRAW | CS_VREDRAW;
		wcx.lpfnWndProc = _WinMsgHandler;
		wcx.hInstance = hInstance;
		wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wcx.lpszClassName = CLASS_NAME;
		RegisterClassEx(&wcx);
		m_hWndParent = hWndParent;
		m_hWnd = CreateWindowEx(0,
			CLASS_NAME,
			CLASS_NAME,
			(hWndParent == NULL ? WS_OVERLAPPEDWINDOW : WS_CHILD) | WS_VISIBLE,
			position.left,
			position.top,
			position.right - position.left,
			position.bottom - position.top,
			hWndParent,
			NULL,
			m_hInstance,
			(void*)this
		);
		return m_hWnd != NULL;
	}

	LRESULT MapPainterGDIWindow::OnWinMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void MapPainterGDIWindow::OnTileUpdate()
	{
	}

	void MapPainterGDIWindow::Set(osmscout::Projection* pProjection, osmscout::MapParameter* pParameter, osmscout::MapData* pData)
	{
		m_pProjection = pProjection;
		m_pParameter = pParameter;
		m_pData = pData;
	}

	LRESULT MapPainterGDIWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CREATE:
		{
			RECT tmp;
			GetClientRect(hwnd, &tmp);
			InvalidateRect(hwnd, &tmp, TRUE);
		}
		break;

		case WM_SIZE:
		{
			RECT tmp;
			GetClientRect(hwnd, &tmp);
			if (MercatorProjection* mercator = dynamic_cast<MercatorProjection*>(m_pProjection); mercator != nullptr)
			{
				mercator->Set(m_pProjection->GetCenter(), m_pProjection->GetMagnification(), m_pProjection->GetDPI(), m_pProjection->GetWidth(), m_pProjection->GetHeight());
			}
			else
			{
			}
			OnTileUpdate();
			InvalidateRect(hwnd, &tmp, TRUE);
		}
		break;

		case WM_CLOSE:
			if (m_pPainter != NULL)
			{
				delete m_pPainter;
				m_pPainter = NULL;
			}
			break;

		case WM_DESTROY:
			if (m_pPainter != NULL)
			{
				delete m_pPainter;
				m_pPainter = NULL;
			}
			break;

		case WM_PAINT:
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			if (m_pPainter != NULL && m_pProjection != NULL && m_pParameter != NULL && m_pData != NULL)
			{
				/*bool result = */m_pPainter->DrawMap(*m_pProjection, *m_pParameter, *m_pData, hdc, ps.rcPaint);
			}
			EndPaint(hwnd, &ps);
		}
		return 0;

		}
		return OnWinMsg(hwnd, uMsg, wParam, lParam);
	}

	void MapPainterGDIWindow::MoveWindow(RECT position, bool bRepaint)
	{
		::MoveWindow(m_hWnd, position.left, position.top, position.right - position.left, position.bottom - position.top, bRepaint ? TRUE : FALSE);
	}

	void MapPainterGDIWindow::InvalidateWindow()
	{
		RECT wndSize;
		GetClientRect(m_hWnd, &wndSize);
		InvalidateRect(m_hWnd, &wndSize, TRUE);
	}
}
