#include <QApplication>
#include <QWidget>
#include <QMouseEvent>

#include <osmscoutclientqt/OSMScoutQt.h>

const auto MAX_ZOOM = 20;
const auto MIN_ZOOM = 0;
const auto MAP_DPI = 96;

namespace {
// used with QWheelEvent
template <typename EventType>
auto pos(EventType* event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    return event->pos();
#else
    return event->position().toPoint();
#endif
}

// used with QMouseEvent
template <typename EventType>
auto pos2(EventType* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->pos();
#else
    return event->position().toPoint();
#endif
}
}

class MapFrame : public QWidget {
public:
    explicit MapFrame(const QString& maps_dir, const QString& stylesheet_file)
    {
        m_currentProjection.Set({0, 0}, 0.0, osmscout::Magnification{osmscout::Magnification::magWorld}, MAP_DPI, width(), height());

        QFileInfo stylesheetFile(stylesheet_file);
        if (!osmscout::OSMScoutQt::NewInstance()
                .WithMapLookupDirectories({maps_dir})
                .WithStyleSheetDirectory(stylesheetFile.dir().path())
                .WithStyleSheetFile(stylesheetFile.fileName())
                .Init()) {
            throw std::runtime_error{"failed to init OSMScoutQt"};
        }

        m_renderer = osmscout::OSMScoutQt::GetInstance().MakeMapRenderer(osmscout::RenderingType::TiledRendering);
        connect(m_renderer, &osmscout::MapRenderer::Redraw, this, [this] {update();});
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        auto painter = QPainter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        m_renderer->RenderMap(painter, osmscout::MapViewStruct{
            m_currentProjection.GetCenter(),
            osmscout::Bearing{},
            m_currentProjection.GetMagnification(),
            static_cast<size_t>(width()),
            static_cast<size_t>(height()),
            MAP_DPI,
        });
    }

    void mousePressEvent(QMouseEvent* ev) override
    {
        m_lastMousePos = ::pos2(ev);
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        auto x_delta = ::pos2(ev).x() - m_lastMousePos.x();
        auto y_delta = ::pos2(ev).y() - m_lastMousePos.y();
        m_currentProjection.Move(-x_delta, y_delta);
        m_lastMousePos = ::pos2(ev);
        update();
    }

    void wheelEvent(QWheelEvent* ev) override
    {
        auto magnification = m_currentProjection.GetMagnification().GetLevel();
        if (ev->angleDelta().y() > 0) {
            if (magnification >= MAX_ZOOM) {
                return;
            }
            magnification++;
            auto x_delta = (width() / 2. - ::pos(ev).x()) * 0.75;
            auto y_delta = (height() / 2. - ::pos(ev).y()) * 0.75;
            m_currentProjection.Move(-x_delta, y_delta);
        } else {
            if (magnification <= MIN_ZOOM) {
                return;
            }
            magnification--;
            auto x_delta = (width() / 2. - ::pos(ev).x()) * 0.75;
            auto y_delta = (height() / 2. - ::pos(ev).y()) * 0.75;
            m_currentProjection.Move(x_delta, -y_delta);
        }
        m_currentProjection.Set(m_currentProjection.GetCenter(), osmscout::Magnification{osmscout::MagnificationLevel{magnification}}, width(), height());
        update();
    }

private:
    osmscout::MapRenderer* m_renderer;
    osmscout::MercatorProjection m_currentProjection;
    QPoint m_lastMousePos;
};

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << ": <map_database_directory> <stylesheet_file>\n";
        return 1;
    }
    osmscout::OSMScoutQt::RegisterQmlTypes();
    QApplication app(argc, argv);
    MapFrame map(argv[1], argv[2]);
    map.show();
    return QApplication::exec();
}
