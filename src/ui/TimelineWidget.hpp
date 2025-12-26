#pragma once

#include <QWidget>
#include <QScrollArea>
#include "domain/TimeTypes.hpp"

namespace beater {

class Project;
class Engine;

// Custom widget for rendering the timeline with tracks and regions
class TimelineCanvas : public QWidget {
    Q_OBJECT
    
public:
    explicit TimelineCanvas(QWidget* parent = nullptr);
    
    void setProject(Project* project);
    void setEngine(Engine* engine);
    void setPixelsPerBeat(double ppb) { pixelsPerBeat_ = ppb; update(); }
    double getPixelsPerBeat() const { return pixelsPerBeat_; }
    
    QSize sizeHint() const override;
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
private:
    void drawRuler(QPainter& painter, int width);
    void drawTracks(QPainter& painter, int width, int height);
    void drawPlayhead(QPainter& painter, int height);
    
    Tick pixelToTick(int x) const;
    int tickToPixel(Tick tick) const;
    
    struct RegionHitInfo {
        size_t trackIndex = 0;
        size_t regionIndex = 0;
        bool isValid = false;
        bool isLeftEdge = false;
        bool isRightEdge = false;
        bool isBody = false;
    };
    
    RegionHitInfo hitTestRegion(const QPoint& pos) const;
    void updateCursor(const QPoint& pos);
    void deleteRegion(size_t trackIndex, size_t regionIndex);
    void setRegionTimeSignature(size_t trackIndex, size_t regionIndex);
    int hitTestTrack(const QPoint& pos) const;
    
    Project* project_ = nullptr;
    Engine* engine_ = nullptr;
    double pixelsPerBeat_ = 40.0;  // Zoom level
    
    // Interaction state
    enum class InteractionMode { None, DraggingRegion, ResizingLeft, ResizingRight, DraggingPlayhead };
    InteractionMode interactionMode_ = InteractionMode::None;
    RegionHitInfo draggedRegion_;
    RegionHitInfo selectedRegion_;
    QPoint dragStartPos_;
    Tick dragStartTick_ = 0;
    Tick dragOriginalLength_ = 0;
    
    static constexpr int RULER_HEIGHT = 40;
    static constexpr int TRACK_HEIGHT = 80;
    static constexpr int TRACK_SPACING = 4;
    static constexpr int RESIZE_EDGE_WIDTH = 8;
};

// Timeline widget with ruler, tracks, and scrolling
class TimelineWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    
    void setProject(Project* project);
    void setEngine(Engine* engine);
    void updatePlayhead();
    
public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    
private:
    TimelineCanvas* canvas_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
};

} // namespace beater
