from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy
import os


class LibosmscoutConan(ConanFile):
    name = "libosmscout"
    settings = "os", "compiler", "build_type", "arch"

    # -------------------------------------------------------------------------
    # Options: Feature-Gates für optionale Backends
    # -------------------------------------------------------------------------
    options = {
        "shared":          [True, False],
        "with_protobuf":   [True, False],   # OSM PBF import
        "with_xml2":       [True, False],   # OSM XML import
        "with_cairo":      [True, False],   # Cairo renderer
        "with_opengl":     [True, False],   # OpenGL / GLFW renderer
        "with_qt":         [True, False],   # Qt5/Qt6 renderer + map widget
        "with_marisa":     [True, False],   # Komprimierte Trie-Datenstruktur
        "with_libagg":     [True, False],   # AGG renderer (2D anti-grain)
        "with_tests":      [True, False],   # Unit-Tests mit Catch2
    }
    default_options = {
        "shared":          True,
        "with_protobuf":   True,
        "with_xml2":       True,
        "with_cairo":      False,
        "with_opengl":     True,
        "with_qt":         True,
        "with_marisa":     True,
        "with_libagg":     False,
        "with_tests":      True,
        # Qt-spezifische Optionen
        "qt/*:opengl":     "desktop",
        "qt/*:qtshadertools": True,
        "qt/*:qt5compat":  True,
        "qt/*:qtsvg":       True,
        "qt/*:qtdeclarative": True,
        "qt/*:qtpositioning": True,
        "qt/*:qtlocation": True,
        "qt/*:qtmultimedia": False,
        "qt/*:qttools":    True,
        "qt/*:with_pq":   False,
        "qt/*:with_sqlite3":  False,   # kein externes SQLite
        "qt/*:with_odbc":     False,   # kein ODBC-Treiber (ebenfalls oft unnötig)
        "qt/*:with_mysql":    False,
        "libxml2/*:with_postgresql": False,
        "libxml2/*:with_lzma":       False,
   }

    # -------------------------------------------------------------------------
    # Abhängigkeiten
    # -------------------------------------------------------------------------
    def requirements(self):
        # --- Core (immer benötigt) -------------------------------------------
        self.requires("zlib/1.3.2")
        self.requires("libiconv/1.17")

        # --- OSM PBF Import --------------------------------------------------
        if self.options.with_protobuf:
            self.requires("protobuf/6.33.5")

        # --- OSM XML Import --------------------------------------------------
        if self.options.with_xml2:
            self.requires("libxml2/2.15.3")

        # --- Komprimierte Trie (Marisa) --------------------------------------
        if self.options.with_marisa:
            self.requires("marisa/0.2.6")

        # --- Cairo Renderer --------------------------------------------------
        if self.options.with_cairo:
            self.requires("cairo/1.18.4")
            self.requires("freetype/2.14.3")
            self.requires("libpng/1.6.58")

        # --- OpenGL Renderer -------------------------------------------------
        if self.options.with_opengl:
            self.requires("glfw/3.4")
            self.requires("glm/1.0.1")
            self.requires("glew/2.2.0")
            self.requires("mesa-glu/9.0.3")
            self.requires("opengl/system")   # System-OpenGL (kein eigenes Paket nötig)

        # --- Qt Renderer + Map Widget ----------------------------------------
        # if self.options.with_qt:
        #     self.requires("qt/6.11.1")

    # --- AGG Renderer ----------------------------------------------------
        if self.options.with_libagg:
            self.requires("agg/2.5")

        # --- Unit Tests ------------------------------------------------------
        if self.options.with_tests:
            self.requires("catch2/3.15.0")

    # -------------------------------------------------------------------------
    # Build-Abhängigkeiten (werden nicht in den Paketen weitergegeben)
    # -------------------------------------------------------------------------
    def build_requirements(self):
        if self.options.with_protobuf:
            # Protoc-Compiler wird nur beim Build benötigt
            self.tool_requires("protobuf/<host_version>")

    # -------------------------------------------------------------------------
    # Layout & Toolchain
    # -------------------------------------------------------------------------
    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)

        # Feature-Flags an CMake durchreichen
        tc.variables["OSMSCOUT_BUILD_WITH_OPENSSL"]  = False
        tc.variables["OSMSCOUT_BUILD_IMPORT"]          = self.options.with_protobuf or self.options.with_xml2
        tc.variables["OSMSCOUT_BUILD_MAP_CAIRO"]       = self.options.with_cairo
        tc.variables["OSMSCOUT_BUILD_MAP_OPENGL"]      = self.options.with_opengl
        tc.variables["OSMSCOUT_BUILD_MAP_QT"]          = self.options.with_qt
        tc.variables["OSMSCOUT_BUILD_MAP_AGG"]         = self.options.with_libagg
        tc.variables["OSMSCOUT_BUILD_TESTS"]           = self.options.with_tests
        tc.variables["BUILD_SHARED_LIBS"]              = self.options.shared
        tc.variables["OSMSCOUT_BUILD_MAP_QT"]          = self.options.with_qt
        tc.variables["CMAKE_DISABLE_FIND_PACKAGE_Qt5"] = True
        tc.variables["QT_DEFAULT_MAJOR_VERSION"]       = 6

        # Qt6-Pfad aus Conan-Dependencies extrahieren und setzen
        #qt_info = self.dependencies["qt"].cpp_info
        #qt_lib_path = qt_info.libdirs[0]
        #qt_cmake_path = os.path.join(os.path.dirname(qt_lib_path), "lib", "cmake")
        #tc.variables["CMAKE_PREFIX_PATH"] = qt_cmake_path

        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    # -------------------------------------------------------------------------
    # Build & Package (optional – für den Fall, dass libosmscout selbst
    # als Conan-Paket verteilt werden soll)
    # -------------------------------------------------------------------------
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE*", self.source_folder,
             os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["osmscout"]
        if self.options.with_cairo:
            self.cpp_info.libs.append("osmscout_map_cairo")
        if self.options.with_opengl:
            self.cpp_info.libs.append("osmscout_map_opengl")
        if self.options.with_qt:
            self.cpp_info.libs.append("osmscout_map_qt")
            self.cpp_info.libs.append("osmscout_client_qt")