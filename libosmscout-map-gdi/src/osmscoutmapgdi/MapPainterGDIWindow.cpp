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

#include <osmscoutmapgdi/MapPainterGDIWindow.h>

namespace osmscout {

	static LRESULT CALLBACK _WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_NCCREATE)
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT(lParam))->lpCreateParams));
		}
		MapPainterGDIWindow* pWnd = (MapPainterGDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (pWnd != nullptr) {
			return pWnd->WinMsgHandler(hwnd, uMsg, wParam, lParam);
		}
        else {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
	}

	MapPainterGDIWindow::MapPainterGDIWindow()
		: m_hInstance(nullptr)
		, m_hWnd(nullptr)
		, m_hWndParent(nullptr)
		, m_pPainter(nullptr)
		, m_pProjection(nullptr)
		, m_pParameter(nullptr)
		, m_pData(nullptr)
	{
	}

	MapPainterGDIWindow::MapPainterGDIWindow(const StyleConfigRef& styleConfig, RECT position, HWND hWndParent, HINSTANCE hInstance)
		: m_hInstance(hInstance)
		, m_hWnd(nullptr)
		, m_hWndParent(nullptr)
		, m_pPainter(nullptr)
		, m_pProjection(nullptr)
		, m_pParameter(nullptr)
		, m_pData(nullptr)
	{
		CreateCanvas(styleConfig, position, hWndParent, hInstance);
	}

	MapPainterGDIWindow::~MapPainterGDIWindow()
	{
		if (m_pPainter != nullptr)
		{
			delete m_pPainter;
			m_pPainter = nullptr;
		}
	}

    void MapPainterGDIWindow::LogStatus()
    {
        if (m_pProjection != nullptr)
        {
            log.Info() << "Magnification: " << m_pProjection->GetMagnification().GetMagnification();
            log.Info() << "Dimension: " << m_pProjection->GetDimensions().GetDisplayText();
            log.Info() << "Rect: " << m_pProjection->GetWidth() << "x" << m_pProjection->GetHeight();
        }
    }

	bool MapPainterGDIWindow::CreateCanvas(const StyleConfigRef& styleConfig, RECT position, HWND hWndParent, HINSTANCE hInstance)
	{
		if (m_hWnd != nullptr) {
            return true;
        }

        m_hInstance = hInstance;

        if (m_hInstance == nullptr) {
            m_hInstance = GetModuleHandle(nullptr);
        }

		if (m_pPainter == nullptr) {
            m_pPainter = new MapPainterGDI(styleConfig);
        }

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
			(hWndParent == nullptr ? WS_OVERLAPPEDWINDOW : WS_CHILD) | WS_VISIBLE,
			position.left,
			position.top,
			position.right - position.left,
			position.bottom - position.top,
			hWndParent,
			nullptr,
			m_hInstance,
			(void*)this
		);
		return m_hWnd != nullptr;
	}

	LRESULT MapPainterGDIWindow::OnWinMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void MapPainterGDIWindow::OnTileUpdate()
	{
	}

	void MapPainterGDIWindow::Set(osmscout::Projection* pProjection,
                                  osmscout::MapParameter* pParameter,
                                  osmscout::MapData* pData)
	{
		m_pProjection = pProjection;
		m_pParameter = pParameter;
		m_pData = pData;
	}

	LRESULT MapPainterGDIWindow::WinMsgHandler(HWND hwnd,
                                               UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CREATE:
            InvalidateWindow();
    		break;

		case WM_SIZE:
		{
            log.Info() << "WM_SIZE...";

            log.Info() << "Status before:";
            LogStatus();

            RECT tmp;
			GetClientRect(hwnd, &tmp);

            LONG width=tmp.right-tmp.left+1;
            LONG height=tmp.bottom-tmp.top+1;

            osmscout::log.Info() << "New window dimension: "  << width << "x" << height;

            if (MercatorProjection* mercator = dynamic_cast<MercatorProjection*>(m_pProjection); mercator != nullptr)
			{
                log.Info() << "Update projection...";
				mercator->Set(m_pProjection->GetCenter(),
                              m_pProjection->GetMagnification(),
                              m_pProjection->GetDPI(),
                              width,
                              height);
			}

			OnTileUpdate();

            log.Info() << "Status after:";
            LogStatus();

            log.Info() << "InvalidateRect...";
			InvalidateRect(hwnd, &tmp, TRUE);
            log.Info() << "WM_SIZE done";
		}
		break;

		case WM_CLOSE:
			if (m_pPainter != nullptr)
			{
				delete m_pPainter;
				m_pPainter = nullptr;
			}
			break;

		case WM_DESTROY:
			if (m_pPainter != nullptr)
			{
				delete m_pPainter;
				m_pPainter = nullptr;
			}
			break;

        case WM_ERASEBKGND:
            log.Info() << "WM_ERASEBKGND...";
            log.Info() << "WM_ERASEBKGND done";
            return 1;

		case WM_PAINT:
		{
			std::lock_guard<std::mutex> guard(m_mutex);

            log.Info() << "WM_PAINT...";

            LogStatus();

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd,
                                 &ps);

            log.Info() << "Drawing rect: [" << ps.rcPaint.left << " - " << ps.rcPaint.right << "] x [" << ps.rcPaint.top << " - " << ps.rcPaint.bottom << "]";

			if (m_pPainter != nullptr &&
                m_pProjection != nullptr &&
                m_pParameter != nullptr &&
                m_pData != nullptr)
			{
				/*bool result = */m_pPainter->DrawMap(*m_pProjection,
                                                      *m_pParameter,
                                                      *m_pData,
                                                      hdc,
                                                      ps.rcPaint);
			}

			EndPaint(hwnd, &ps);
            log.Info() << "WM_PAINT done";
		}
		return 0;

		}

		return OnWinMsg(hwnd, uMsg, wParam, lParam);
	}

	void MapPainterGDIWindow::MoveWindow(RECT position, bool bRepaint)
	{
		::MoveWindow(m_hWnd,
                     position.left,
                     position.top,
                     position.right - position.left,
                     position.bottom - position.top,
                     bRepaint ? TRUE : FALSE);
	}

	void MapPainterGDIWindow::InvalidateWindow()
	{
		RECT wndSize;

		GetClientRect(m_hWnd, &wndSize);
		InvalidateRect(m_hWnd, &wndSize, TRUE);
	}
}
