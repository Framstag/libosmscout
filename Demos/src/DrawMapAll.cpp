/*
  DrawMapAll - a demo program for libosmscout
  Copyright (C) 2025  Tim Teulings

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

#include <DrawMap.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(HAVE_OSMSCOUT_MAP_AGG) || defined(HAVE_OSMSCOUT_MAP_OPENGL)
#include <png.h>
#endif

// ---- Backend includes (conditional per platform) ----

#if defined(HAVE_OSMSCOUT_MAP_AGG)
  #include <osmscoutmapagg/MapPainterAgg.h>
  #include <agg2/agg_rendering_buffer.h>
  #include <agg2/agg_pixfmt_rgb.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_CAIRO)
  #include <osmscoutmapcairo/MapPainterCairo.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_OPENGL)
  #include <osmscoutmapopengl/MapPainterOpenGL.h>
  #include <osmscoutmapopengl/MapOpenGLFeatures.h>
  #include <GLFW/glfw3.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_QT)
  #include <QApplication>
  #include <QPixmap>
  #include <QPainter>
  #include <osmscoutmapqt/MapPainterQt.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_SVG)
  #include <osmscoutmapsvg/MapPainterSVG.h>
#endif

#if defined(__APPLE__) && defined(HAVE_OSMSCOUT_MAP_IOSX)
  // OSX backend implemented in DrawMapAllOSX.mm
  bool RenderWithOSX(DrawMapDemo& drawDemo,
                     const std::string& outputDir,
                     size_t width,
                     size_t height);
#endif

#if defined(_WIN32) && defined(HAVE_OSMSCOUT_MAP_DIRECTX)
#if !defined(NOMINMAX)
  #define NOMINMAX
#endif
  #include <windows.h>
  #include <d2d1.h>
  #include <dwrite.h>
  #include <wincodec.h>
  #include <osmscoutmapdirectx/MapPainterDirectX.h>
#endif

#if defined(_WIN32) && defined(HAVE_OSMSCOUT_MAP_GDI)
  #include <windows.h>
  #include <gdiplus.h>
  #include <osmscoutmapgdi/MapPainterGDI.h>
#endif

// ---- Helper: Write raw RGB buffer to PNG via libpng ----

#if defined(HAVE_OSMSCOUT_MAP_AGG) || defined(HAVE_OSMSCOUT_MAP_OPENGL)
static bool WriteRGBToPNG(const std::string& path,
                           size_t width,
                           size_t height,
                           const unsigned char* rgb,
                           bool flipVertical)
{
  FILE* fp = fopen(path.c_str(), "wb");
  if (!fp) {
    std::cerr << "ERROR: Cannot open " << path << " for writing" << std::endl;
    return false;
  }

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                             nullptr, nullptr, nullptr);
  if (!png) {
    fclose(fp);
    return false;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, nullptr);
    fclose(fp);
    return false;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return false;
  }

  png_init_io(png, fp);
  png_set_IHDR(png, info,
                (png_uint_32)width, (png_uint_32)height,
                8,
                PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);

  size_t rowStride = width * 3;
  std::vector<png_bytep> rowPointers(height);
  for (size_t y = 0; y < height; y++) {
    size_t srcY = flipVertical ? (height - 1 - y) : y;
    rowPointers[y] = const_cast<png_bytep>(rgb + srcY * rowStride);
  }

  png_write_image(png, rowPointers.data());
  png_write_end(png, info);

  png_destroy_write_struct(&png, &info);
  fclose(fp);
  return true;
}
#endif

// ---- Helper: Output file path for a backend ----

static std::string BackendPath(const std::string& dir,
                                const std::string& name,
                                const std::string& ext)
{
  return dir + "/DrawMap" + name + "." + ext;
}

// ---- Helper: Track backend success/failure ----

struct BackendResult {
  bool rendered;
  bool wrotePng;
  std::string filePath;
};

static BackendResult ReportOK(const std::string& path)
{
  std::cout << "  OK: " << path << std::endl;
  return {true, true, path};
}

static BackendResult ReportFail(const std::string& backendName)
{
  std::cerr << "  FAIL: " << backendName << std::endl;
  return {false, false, ""};
}

// ======================================================================

int main(int argc, char* argv[])
{
  DrawMapDemo drawDemo("DrawMapAll", argc, argv);

  // OpenGL shader path (must be set before OpenDatabase which triggers parsing)
  std::string shaderPath =
#if defined(HAVE_OSMSCOUT_MAP_OPENGL)
    SHADER_INSTALL_DIR;
#else
    "";
#endif

#if defined(HAVE_OSMSCOUT_MAP_OPENGL)
  drawDemo.argParser.AddOption(
      osmscout::CmdLineStringOption([&shaderPath](const std::string& value) {
        shaderPath = value;
      }),
      std::vector<std::string>{"shaders"},
      "Path to shaders (default: " + shaderPath + ")",
      false);
#endif

  if (!drawDemo.OpenDatabase()) {
    return 2;
  }

  drawDemo.LoadData();
  Arguments args = drawDemo.GetArguments();

  // Create output directory
  std::string outputDir = args.output;
  try {
    std::filesystem::create_directories(outputDir);
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Cannot create output directory '" << outputDir
              << "': " << e.what() << std::endl;
    return 1;
  }

  size_t w = args.width;
  size_t h = args.height;
  bool anySuccess = false;
  bool anyFailure = false;

  std::cout << "DrawMapAll: rendering with available backends..." << std::endl
            << "  Output directory: " << outputDir << std::endl
            << "  Size: " << w << "x" << h << std::endl;

  // ================================================================
  // AGG backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_AGG)
  {
    std::cout << "AGG backend..." << std::endl;

    auto* buffer = new unsigned char[w * h * 3];
    memset(buffer, 255, w * h * 3);

    agg::rendering_buffer rbuf(buffer, (unsigned int)w, (unsigned int)h,
                                (int)(w * 3));
    agg::pixfmt_rgb24 pf(rbuf);

    osmscout::MapPainterAgg painter;
    if (painter.DrawMap(drawDemo.projection,
                         drawDemo.drawParameter,
                         drawDemo.data,
                         &pf)) {
      std::string path = BackendPath(outputDir, "Agg", "png");
      if (WriteRGBToPNG(path, w, h, buffer, false)) {
        ReportOK(path);
        anySuccess = true;
      } else {
        anyFailure = true;
      }
    } else {
      ReportFail("AGG");
      anyFailure = true;
    }

    delete[] buffer;
  }
#endif

  // ================================================================
  // Cairo backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_CAIRO)
  {
    std::cout << "Cairo backend..." << std::endl;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                           (int)w, (int)h);
    if (surface) {
      cairo_t* cairo = cairo_create(surface);
      if (cairo) {
        osmscout::MapPainterCairo painter;
        if (painter.DrawMap(drawDemo.projection,
                             drawDemo.drawParameter,
                             drawDemo.data,
                             cairo)) {
          std::string path = BackendPath(outputDir, "Cairo", "png");
          if (cairo_surface_write_to_png(surface, path.c_str()) == CAIRO_STATUS_SUCCESS) {
            ReportOK(path);
            anySuccess = true;
          } else {
            std::cerr << "  FAIL: Cairo PNG write" << std::endl;
            anyFailure = true;
          }
        } else {
          ReportFail("Cairo");
          anyFailure = true;
        }
        cairo_destroy(cairo);
      } else {
        std::cerr << "  FAIL: Cairo context creation" << std::endl;
        anyFailure = true;
      }
      cairo_surface_destroy(surface);
    } else {
      std::cerr << "  FAIL: Cairo surface creation" << std::endl;
      anyFailure = true;
    }
  }
#endif

  // ================================================================
  // OpenGL backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_OPENGL)
  {
    std::cout << "OpenGL backend..." << std::endl;

    glfwSetErrorCallback([](int, const char* errStr) {
      std::cerr << "GLFW Error: " << errStr << std::endl;
    });

    if (!glfwInit()) {
      std::cerr << "  FAIL: GLFW init" << std::endl;
      anyFailure = true;
    } else {
      glfwWindowHint(GLFW_SAMPLES, 4);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
      glfwWindowHint(GLFW_VISIBLE, false);

      GLFWwindow* offscreenCtx = glfwCreateWindow((int)w, (int)h,
                                                    "", nullptr, nullptr);
      if (!offscreenCtx) {
        std::cerr << "  FAIL: OpenGL offscreen context creation" << std::endl;
        glfwTerminate();
        anyFailure = true;
      } else {
        glfwMakeContextCurrent(offscreenCtx);

        auto* painter = new osmscout::MapPainterOpenGL(
            (int)w, (int)h, args.dpi, args.fontName, shaderPath,
            drawDemo.drawParameter);

        if (!painter->IsInitialized()) {
          std::cerr << "  FAIL: OpenGL painter init" << std::endl;
          delete painter;
          glfwDestroyWindow(offscreenCtx);
          glfwTerminate();
          anyFailure = true;
        } else {
          painter->SetCenter(drawDemo.projection.GetCenter());
          painter->SetMagnification(drawDemo.projection.GetMagnification());

          if (!drawDemo.data.empty()) {
            painter->ProcessData(drawDemo.data.front(),
                                 drawDemo.projection,
                                 drawDemo.data.front().styleConfig);
          }
          painter->SwapData();
          painter->DrawMap();

          auto* image = new unsigned char[w * h * 3];
          glReadPixels(0, 0, (GLsizei)w, (GLsizei)h,
                        GL_RGB, GL_UNSIGNED_BYTE, image);

          std::string path = BackendPath(outputDir, "OpenGL", "png");
          if (WriteRGBToPNG(path, w, h, image, true)) {
            ReportOK(path);
            anySuccess = true;
          } else {
            anyFailure = true;
          }

          delete[] image;
          delete painter;
          glfwDestroyWindow(offscreenCtx);
          glfwTerminate();
        }
      }
    }
  }
#endif

  // ================================================================
  // Qt backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_QT)
  {
    std::cout << "Qt backend..." << std::endl;

    int qtArgc = 1;
    char qtArg0[] = "DrawMapAll";
    char* qtArgv[1] = {qtArg0};
    QApplication qtApp(qtArgc, qtArgv, true);

    auto* pixmap = new QPixmap(static_cast<int>(w),
                                static_cast<int>(h));
    auto* painter = new QPainter(pixmap);

    osmscout::MapPainterQt mapPainter;
    if (mapPainter.DrawMap(drawDemo.projection,
                            drawDemo.drawParameter,
                            drawDemo.data,
                            painter)) {
      std::string path = BackendPath(outputDir, "Qt", "png");
      if (pixmap->save(QString::fromStdString(path), "PNG", -1)) {
        ReportOK(path);
        anySuccess = true;
      } else {
        std::cerr << "  FAIL: Qt PNG write" << std::endl;
        anyFailure = true;
      }
    } else {
      ReportFail("Qt");
      anyFailure = true;
    }

    delete painter;
    delete pixmap;
  }
#endif

  // ================================================================
  // SVG backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_SVG)
  {
    std::cout << "SVG backend..." << std::endl;

    std::string path = BackendPath(outputDir, "SVG", "svg");
    std::ofstream stream(path.c_str(),
                          std::ios_base::binary |
                          std::ios_base::trunc |
                          std::ios_base::out);
    if (!stream) {
      std::cerr << "  FAIL: Cannot open " << path << " for writing" << std::endl;
      anyFailure = true;
    } else {
      osmscout::MapPainterSVG painter;
      painter.DrawMap(drawDemo.projection,
                       drawDemo.drawParameter,
                       drawDemo.data,
                       stream);
      stream.close();
      ReportOK(path);
      anySuccess = true;
    }
  }
#endif

  // ================================================================
  // OSX backend (macOS only, implemented in DrawMapAllOSX.mm)
  // ================================================================
#if defined(__APPLE__) && defined(HAVE_OSMSCOUT_MAP_IOSX)
  {
    std::cout << "OSX backend..." << std::endl;
    if (RenderWithOSX(drawDemo, outputDir, w, h)) {
      ReportOK(BackendPath(outputDir, "OSX", "png"));
      anySuccess = true;
    } else {
      ReportFail("OSX");
      anyFailure = true;
    }
  }
#endif

  // ================================================================
  // DirectX backend (Windows only)
  // ================================================================
#if defined(_WIN32) && defined(HAVE_OSMSCOUT_MAP_DIRECTX)
  {
    std::cout << "DirectX backend..." << std::endl;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (!SUCCEEDED(hr)) {
      std::cerr << "  FAIL: COM init" << std::endl;
      anyFailure = true;
    } else {
      ID2D1Factory* d2dFactory = nullptr;
      IWICImagingFactory* wicFactory = nullptr;
      IWICBitmap* wicBitmap = nullptr;
      ID2D1RenderTarget* renderTarget = nullptr;
      bool ok = false;

      hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                              &d2dFactory);
      if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_WICImagingFactory,
                               nullptr,
                               CLSCTX_INPROC_SERVER,
                               IID_IWICImagingFactory,
                               reinterpret_cast<void**>(&wicFactory));
      }
      if (SUCCEEDED(hr)) {
        hr = wicFactory->CreateBitmap(
            (UINT)w, (UINT)h,
            GUID_WICPixelFormat32bppBGRA,
            WICBitmapCacheOnLoad,
            &wicBitmap);
      }
      if (SUCCEEDED(hr)) {
        D2D1_RENDER_TARGET_PROPERTIES props =
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                   D2D1_ALPHA_MODE_IGNORE));
        hr = d2dFactory->CreateWicBitmapRenderTarget(
            wicBitmap, props, &renderTarget);
      }
      if (SUCCEEDED(hr)) {
        IDWriteFactory* writeFactory = nullptr;
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                  __uuidof(IDWriteFactory),
                                  reinterpret_cast<IUnknown**>(&writeFactory));
        if (SUCCEEDED(hr)) {
          osmscout::MapPainterDirectX dxPainter(d2dFactory, writeFactory);
          dxPainter.DrawMap(drawDemo.projection,
                             drawDemo.drawParameter,
                             drawDemo.data,
                             renderTarget);

          // Save WIC bitmap as PNG
          IWICBitmapEncoder* encoder = nullptr;
          IWICStream* stream = nullptr;
          std::string path = BackendPath(outputDir, "DirectX", "png");
          std::wstring wpath(path.begin(), path.end());

          hr = wicFactory->CreateEncoder(GUID_ContainerFormatPng,
                                          nullptr, &encoder);
          if (SUCCEEDED(hr)) {
            hr = wicFactory->CreateStream(&stream);
          }
          if (SUCCEEDED(hr)) {
            hr = stream->InitializeFromFilename(wpath.c_str(), GENERIC_WRITE);
          }
          if (SUCCEEDED(hr)) {
            hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
          }
          if (SUCCEEDED(hr)) {
            IWICBitmapFrameEncode* frame = nullptr;
            IPropertyBag2* props = nullptr;
            hr = encoder->CreateNewFrame(&frame, &props);
            if (SUCCEEDED(hr)) {
              frame->Initialize(props);
              frame->SetSize((UINT)w, (UINT)h);
              WICPixelFormatGUID fmt = GUID_WICPixelFormatDontCare;
              frame->SetPixelFormat(&fmt);
              frame->WriteSource(wicBitmap, nullptr);
              frame->Commit();
              encoder->Commit();
              frame->Release();
            }
            if (props) props->Release();
          }

          if (SUCCEEDED(hr)) {
            ReportOK(path);
            anySuccess = true;
            ok = true;
          } else {
            std::cerr << "  FAIL: DirectX PNG encode" << std::endl;
          }

          if (encoder) encoder->Release();
          if (stream) stream->Release();
          writeFactory->Release();
        } else {
          std::cerr << "  FAIL: DirectX DWrite factory" << std::endl;
        }
      } else {
        std::cerr << "  FAIL: DirectX init (hr=" << std::hex << hr << ")" << std::dec << std::endl;
      }

      if (!ok) anyFailure = true;

      if (renderTarget) renderTarget->Release();
      if (wicBitmap) wicBitmap->Release();
      if (wicFactory) wicFactory->Release();
      if (d2dFactory) d2dFactory->Release();
      CoUninitialize();
    }
  }
#endif

  // ================================================================
  // GDI backend (Windows only)
  // ================================================================
#if defined(_WIN32) && defined(HAVE_OSMSCOUT_MAP_GDI)
  {
    std::cout << "GDI backend..." << std::endl;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::Status gdiStatus = Gdiplus::GdiplusStartup(
        &gdiplusToken, &gdiplusStartupInput, nullptr);

    if (gdiStatus != Gdiplus::Ok) {
      std::cerr << "  FAIL: GDI+ startup" << std::endl;
      anyFailure = true;
    } else {
      HDC hdcScreen = GetDC(nullptr);
      HDC hdc = CreateCompatibleDC(hdcScreen);

      BITMAPINFO bmi = {};
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = (LONG)w;
      bmi.bmiHeader.biHeight = -(LONG)h; // top-down
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      void* bits = nullptr;
      HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
                                          &bits, nullptr, 0);
      if (hBitmap) {
        SelectObject(hdc, hBitmap);

        osmscout::MapPainterGDI gdiPainter;
        gdiPainter.DrawMap(drawDemo.projection,
                            drawDemo.drawParameter,
                            drawDemo.data,
                            hdc);

        // Save via GDI+
        Gdiplus::Bitmap gdiBitmap(hBitmap, nullptr);
        std::string path = BackendPath(outputDir, "GDI", "png");
        std::wstring wpath(path.begin(), path.end());
        CLSID pngClsid;
        CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);

        if (gdiBitmap.Save(wpath.c_str(), &pngClsid, nullptr) == Gdiplus::Ok) {
          ReportOK(path);
          anySuccess = true;
        } else {
          std::cerr << "  FAIL: GDI PNG write" << std::endl;
          anyFailure = true;
        }

        DeleteObject(hBitmap);
      } else {
        std::cerr << "  FAIL: GDI DIBSection creation" << std::endl;
        anyFailure = true;
      }

      DeleteDC(hdc);
      ReleaseDC(nullptr, hdcScreen);
      Gdiplus::GdiplusShutdown(gdiplusToken);
    }
  }
#endif

  // ================================================================
  // Summary
  // ================================================================

  if (anySuccess) {
    std::cout << "DrawMapAll: done." << std::endl;
    return anyFailure ? 1 : 0;
  } else {
    std::cerr << "DrawMapAll: all backends failed." << std::endl;
    return 1;
  }
}