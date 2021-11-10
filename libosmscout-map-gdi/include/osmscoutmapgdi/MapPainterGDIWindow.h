#ifndef OSMSCOUT_MAP_MAPPAINTERGDIWINDOW_H
#define OSMSCOUT_MAP_MAPPAINTERGDIWINDOW_H

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

#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <osmscoutmapgdi/MapGDIImportExport.h>
#include <osmscoutmapgdi/MapPainterGDI.h>
#include <mutex>

namespace osmscout {

	class OSMSCOUT_MAP_GDI_API MapPainterGDIWindow
	{
	private:
		std::mutex                m_mutex;

	protected:
		HINSTANCE                 m_hInstance;
		HWND                      m_hWnd;
		HWND                      m_hWndParent;
		osmscout::MapPainterGDI*  m_pPainter;

		osmscout::Projection*     m_pProjection;
		osmscout::MapParameter*   m_pParameter;
		osmscout::MapData*        m_pData;

	public:
		/**
		@brief Default constructor
		@details Standard constructor.
		*/
		MapPainterGDIWindow();

		/**
		@brief Default constructor
		@details Standard constructor with parameters for map display.
		@param[in] styleConfig Configuration of the drawing styles
		@param[in] position Position and size of the drawing window.
		@param[in] hWndParent Handle of parent Windows or NULL for non-child window.
		@param[in] hInstance hInstance of the program or NULL for default.
		*/
		MapPainterGDIWindow(const StyleConfigRef& styleConfig, RECT position, HWND hWndParent, HINSTANCE hInstance = NULL);

		/**
		@brief Default destructor
		@details Standard destructor to release reserved memory.
		*/
		~MapPainterGDIWindow();

		/**
		@brief Creates the drawing window
		@details Creates the drawing window if it does not yet exist.
		@param[in] styleConfig Configuration of the drawing styles
		@param[in] position Position and size of the drawing window.
		@param[in] hWndParent Handle of parent Windows or NULL for non-child window.
		@param[in] hInstance hInstance of the program or NULL for default.
		*/
		bool CreateCanvas(const StyleConfigRef& styleConfig, RECT position, HWND hWndParent = NULL, HINSTANCE hInstance = NULL);

		/**
		@brief Function for Windows message handling
		@details An application-defined function that processes messages sent to the window.
		The function can be overwritten in own classes to process the window messages.
		Some messages such as WM_PAINT are already processed by the class for the basic function.
		@param[in] hwnd Window handle.
		@param[in] uMsg The message ID.
		@param[in] wParam Additional message information. The contents of this parameter depend on the value of the uMsg parameter.
		@param[in] lParam Additional message information. The contents of this parameter depend on the value of the uMsg parameter.
		@remark At the end of the nested overwritten function DefWindowProc(m_hWnd, uMsg, wParam, lParam) or the original function should be returned.
		*/
		virtual LRESULT OnWinMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/**
		@brief Function to update tile data
		@details The function is called as a sub-function if the data must be updated before rendering.
		For example, when zoom, position or window size has been changed.
		*/
		virtual void OnTileUpdate();

		/**
		@brief Sets data and settings for rendering
		@details Sets references to the projection, map parameters and map data used for rendering.
		@param[in] pProjection Pointer on map projection
		@param[in] pParameter Pointer on map parameter
		@param[in] pData Pointer on map data
		*/
		void Set(osmscout::Projection* pProjection, osmscout::MapParameter* pParameter, osmscout::MapData* pData);

		/**
		@brief Move current window
		@details Sets a new position and/or size of the window.
		@param[in] position Rectangle (position and size) into which the window is fitted.
		@param[in] bRepaint With True the window is redrawn after the change.
		*/
		void MoveWindow(RECT position, bool bRepaint = true);

		/**
		@brief Declares the window invalid and initiates a redraw
		@details The entire area of the window is declared invalid and thus a redraw is forced.
		*/
		void InvalidateWindow();

		/**
		@brief Windows message handler function
		@details Non-static callback function to receive Windows messages of the used window in current class instance.
		@warning This function is for internal use only! Do not use it in your application!
		*/
		LRESULT CALLBACK WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}

#endif
