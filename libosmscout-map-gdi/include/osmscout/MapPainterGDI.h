#ifndef OSMSCOUT_MAP_MAPPAINTERGDI_H
#define OSMSCOUT_MAP_MAPPAINTERGDI_H

/*
  This source is part of the libosmscout-map-gdi library
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

#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <mutex>

#include <osmscout/MapGDIImportExport.h>
#include <osmscout/MapPainter.h>

namespace osmscout {

	class OSMSCOUT_MAP_GDI_API MapPainterGDI : public MapPainter
	{
	public:
		struct NativeLabel {
			std::wstring wstr;
			void* font;
			void* render;
		};
		struct NativeGlyph {
			std::string character;
			double width;
			double height;
		};
		using GdiLabel = Label<NativeGlyph, NativeLabel>;

	private:
		using GdiGlyph = Glyph<NativeGlyph>;
		using GdiLabelInstance = LabelInstance<NativeGlyph, NativeLabel>;
		using GdiLabelLayouter = LabelLayouter<NativeGlyph, NativeLabel, MapPainterGDI>;
		friend GdiLabelLayouter;

		GdiLabelLayouter                   m_labelLayouter;
		std::mutex                         m_mutex;
		HWND                               m_hWnd;
		HINSTANCE                          m_hInstance;
		RECT                               m_Size;
		void*                              m_pBuffer;
		static ULONG_PTR                   m_gdiplusToken;
		static DWORD                       m_gdiplusInstCount;
		osmscout::MapData*                 m_pData;
		osmscout::MapParameter*            m_pParameter;
		osmscout::MercatorProjection*      m_pProjection;

	private:
		osmscout::DoubleRectangle GlyphBoundingBox(const NativeGlyph &glyph) const;

		std::shared_ptr<GdiLabel> Layout(const Projection& projection,
			const MapParameter& parameter,
			const std::string& text,
			double fontSize,
			double objectWidth,
			bool enableWrapping = false,
			bool contourLabel = false);

		void DrawLabel(const Projection& projection,
			const MapParameter& parameter,
			const DoubleRectangle& labelRectangle,
			const LabelData& label,
			const NativeLabel& layout);

		void DrawGlyphs(const Projection &projection,
			const MapParameter &parameter,
			const osmscout::PathTextStyleRef style,
			const std::vector<GdiGlyph> &glyphs);

	protected:
		void AfterPreprocessing(const StyleConfig& styleConfig,
			const Projection& projection,
			const MapParameter& parameter,
			const MapData& data) override;

		void BeforeDrawing(const StyleConfig& styleConfig,
			const Projection& projection,
			const MapParameter& parameter,
			const MapData& data) override;

		void AfterDrawing(const StyleConfig& styleConfig,
			const Projection& projection,
			const MapParameter& parameter,
			const MapData& data) override;

		bool HasIcon(const StyleConfig& styleConfig,
			const Projection& projection,
			const MapParameter& parameter,
			IconStyle& style) override;

		double GetFontHeight(const Projection& projection,
			const MapParameter& parameter,
			double fontSize) override;

		void DrawGround(const Projection& projection,
			const MapParameter& parameter,
			const FillStyle& style) override;

		/**
		  Register regular label with given text at the given pixel coordinate
		  in a style defined by the given LabelStyle.
		 */
		virtual void RegisterRegularLabel(const Projection &projection,
			const MapParameter &parameter,
			const std::vector<LabelData> &labels,
			const Vertex2D &position,
			double objectWidth) override;

		/**
		 * Register contour label
		 */
		virtual void RegisterContourLabel(const Projection &projection,
			const MapParameter &parameter,
			const PathLabelData &label,
			const LabelPath &labelPath) override;

		virtual void DrawLabels(const Projection& projection,
			const MapParameter& parameter,
			const MapData& data) override;

		void DrawSymbol(const Projection& projection,
			const MapParameter& parameter,
			const Symbol& style,
			double x, double y) override;

		void DrawIcon(const IconStyle* style,
			double centerX, double centerY,
			double width, double height) override;

		void DrawPath(const Projection& projection,
			const MapParameter& parameter,
			const Color& color,
			double width,
			const std::vector<double>& dash,
			LineStyle::CapStyle startCap,
			LineStyle::CapStyle endCap,
			size_t transStart, size_t transEnd) override;

		void DrawWayOutline(const StyleConfig& styleConfig,
			const Projection& projection,
			const MapParameter& parameter,
			const WayData& data);

		void DrawWay(const StyleConfig& styleConfig,
			const Projection& projection,
			const MapParameter& parameter,
			const WayData& data) override;

		void DrawContourSymbol(const Projection& projection,
			const MapParameter& parameter,
			const Symbol& symbol,
			double space,
			size_t transStart, size_t transEnd) override;

		void DrawArea(const Projection& projection,
			const MapParameter& parameter,
			const AreaData& area) override;

	public:
		/**
		@brief Default constructor
		@details Standard constructor with parameters for map display.
		@param[in] styleConfig Configuration of the drawing styles
		@param[in] hInstance HINSTANCE of the program or NULL if it should be determined by GetModuleHandle
		@param[in] position Position and size of the drawing area on the parent window
		@param[in] hWndParent Handle of the parent window
		@param[in] data Map data
		@param[in] parameter Parameter for map drawing
		@param[in] projection Map projection
		*/
		explicit MapPainterGDI(const StyleConfigRef& styleConfig, HINSTANCE hInstance, RECT position, HWND hWndParent, osmscout::MapData* pData, osmscout::MapParameter* pParameter, osmscout::MercatorProjection* pProjection);
		~MapPainterGDI() override;

		/**
		@brief Windows message handler function
		@details Static callback function to receive Windows messages of the used window. The data is passed to the non-static function of the class.
		@warning This function is for internal use only! Do not use it in your application!
		*/
		static LRESULT CALLBACK _WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/**
		@brief Windows message handler function
		@details Non-static callback function to receive Windows messages of the used window in current class instance.
		@warning This function is for internal use only! Do not use it in your application!
		*/
		LRESULT CALLBACK WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/**
		@brief Move and/or resize the drawing Window
		@details Moves and/or changes the size of the drawing window on the parent window. The function can be used to update the size of the parent window's WM_SIZE message, for example.
		@param[in] position Rectangle on the parent window in which to draw.
		@param[in] bRepaint True, if the drawing window should be redrawn immediately after changing size/position.
		*/
		void MoveWindow(RECT position, bool bRepaint = true);

		/**
		@brief Repaint window
		@details Declares the drawing area invalid and triggers a new drawing.
		*/
		void UpdateWindow();

	private:
		void Resize();
	};
}

#endif
