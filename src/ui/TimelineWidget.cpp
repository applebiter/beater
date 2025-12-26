#include "ui/TimelineWidget.hpp"
#include "domain/Project.hpp"
#include "domain/Track.hpp"
#include "domain/Region.hpp"
#include "engine/Engine.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QKeyEvent>
#include <QWheelEvent>
#include <cmath>

namespace beater {

// ============================================================================
// TimelineCanvas Implementation
// ============================================================================

TimelineCanvas::TimelineCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(300);
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

void TimelineCanvas::setProject(Project* project) {
    project_ = project;
    update();
}

void TimelineCanvas::setEngine(Engine* engine) {
    engine_ = engine;
}

QSize TimelineCanvas::sizeHint() const {
    if (!project_) {
        return QSize(2000, 300);
    }
    
    // Calculate width based on project length - check actual region extent
    Tick maxTick = 32 * PPQ * 4; // Default: 32 bars
    
    // Find the furthest region
    for (size_t i = 0; i < project_->getTrackCount(); ++i) {
        const Track* track = project_->getTrack(i);
        if (track) {
            for (const auto& region : track->getRegions()) {
                Tick endTick = region.getEndTick();
                if (endTick > maxTick) {
                    maxTick = endTick;
                }
            }
        }
    }
    
    // Add some extra space at the end
    maxTick += PPQ * 8; // 2 extra bars
    int width = tickToPixel(maxTick);
    
    // Calculate height based on track count
    int trackCount = static_cast<int>(project_->getTrackCount());
    int height = RULER_HEIGHT + (trackCount * (TRACK_HEIGHT + TRACK_SPACING)) + 50;
    
    return QSize(std::max(2000, width), std::max(300, height));
}

Tick TimelineCanvas::pixelToTick(int x) const {
    return static_cast<Tick>((x * PPQ) / pixelsPerBeat_);
}

int TimelineCanvas::tickToPixel(Tick tick) const {
    return static_cast<int>((tick * pixelsPerBeat_) / PPQ);
}

void TimelineCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int width = this->width();
    int height = this->height();
    
    // Background
    painter.fillRect(0, 0, width, height, QColor(0x1a, 0x1a, 0x1a));
    
    // Draw ruler
    drawRuler(painter, width);
    
    // Draw tracks
    if (project_) {
        drawTracks(painter, width, height);
    }
    
    // Draw playhead - always visible for positioning
    if (engine_) {
        drawPlayhead(painter, height);
    }
}

void TimelineCanvas::drawRuler(QPainter& painter, int width) {
    // Ruler background
    painter.fillRect(0, 0, width, RULER_HEIGHT, QColor(0x25, 0x25, 0x25));
    
    if (!project_) return;
    
    // Draw bar numbers and beat marks using MeterMap
    const auto& meterMap = project_->getMeterMap();
    painter.setPen(QColor(0x60, 0x60, 0x60));
    
    // Calculate visible range
    Tick endTick = pixelToTick(width);
    
    // Draw bars from start to end
    Tick currentTick = 0;
    int barNumber = 1;
    
    while (currentTick <= endTick) {
        TimeSignature sig = meterMap.getSignatureAt(currentTick);
        Tick barLength = TimeUtils::ticksPerBar(sig);
        
        int x = tickToPixel(currentTick);
        
        if (x >= 0 && x <= width) {
            // Bar line
            painter.setPen(QColor(0x60, 0x60, 0x60));
            painter.drawLine(x, 0, x, RULER_HEIGHT);
            
            // Bar number
            painter.setPen(QColor(0xb0, 0xb0, 0xb0));
            painter.drawText(x + 4, 12, 50, 20, Qt::AlignLeft | Qt::AlignTop, QString::number(barNumber));
            
            // Beat marks (lighter lines)
            painter.setPen(QColor(0x40, 0x40, 0x40));
            for (int beat = 1; beat < sig.numerator; ++beat) {
                int beatX = tickToPixel(currentTick + beat * PPQ);
                if (beatX >= 0 && beatX <= width) {
                    painter.drawLine(beatX, RULER_HEIGHT - 10, beatX, RULER_HEIGHT);
                }
            }
        }
        
        currentTick += barLength;
        barNumber++;
        
        // Safety check to prevent infinite loop
        if (barNumber > 1000) break;
    }
    
    // Ruler bottom border
    painter.setPen(QColor(0x3a, 0x3a, 0x3a));
    painter.drawLine(0, RULER_HEIGHT, width, RULER_HEIGHT);
}

void TimelineCanvas::drawTracks(QPainter& painter, int width, int) {
    int y = RULER_HEIGHT + TRACK_SPACING;
    
    for (size_t i = 0; i < project_->getTrackCount(); ++i) {
        const Track* track = project_->getTrack(i);
        if (!track) continue;
        
        // Track background
        QColor trackBg = (i % 2 == 0) ? QColor(0x20, 0x20, 0x20) : QColor(0x22, 0x22, 0x22);
        painter.fillRect(0, y, width, TRACK_HEIGHT, trackBg);
        
        // Track name
        painter.setPen(QColor(0xa0, 0xa0, 0xa0));
        painter.drawText(8, y + 5, 150, 20, Qt::AlignLeft | Qt::AlignTop, 
                        QString::fromStdString(track->getName()));
        
        // Draw regions
        for (const auto& region : track->getRegions()) {
            int regionX = tickToPixel(region.getStartTick());
            int regionWidth = tickToPixel(region.getLengthTicks());
            int regionY = y + 25;
            int regionHeight = TRACK_HEIGHT - 30;
            
            // Region color based on pattern name heuristics
            QColor regionColor;
            QString regionLabel;
            std::string patternId = region.getPatternId();
            QString patternName = QString::fromStdString(patternId).toLower();
            
            if (patternName.contains("groove") || patternName.contains("beat") || patternName.contains("basic")) {
                regionColor = QColor(0x00, 0xaa, 0x66);  // Green
                regionLabel = "Groove";
            } else if (patternName.contains("fill")) {
                regionColor = QColor(0xff, 0x88, 0x00);  // Orange
                regionLabel = "Fill";
            } else if (patternName.contains("half") || patternName.contains("time")) {
                regionColor = QColor(0x66, 0x66, 0xff);  // Blue
                regionLabel = "Variation";
            } else {
                // Fallback to region type
                switch (region.getType()) {
                    case RegionType::Groove:
                        regionColor = QColor(0x00, 0xaa, 0x66);
                        regionLabel = "Groove";
                        break;
                    case RegionType::Fill:
                        regionColor = QColor(0xff, 0x88, 0x00);
                        regionLabel = "Fill";
                        break;
                    default:
                        regionColor = QColor(0x60, 0x60, 0x60);
                        regionLabel = "Region";
                        break;
                }
            }
            
            // Draw region rectangle
            painter.fillRect(regionX, regionY, regionWidth, regionHeight, regionColor);
            
            // Region border
            painter.setPen(regionColor.lighter(150));
            painter.drawRect(regionX, regionY, regionWidth, regionHeight);
            
            // Region label (if wide enough)
            if (regionWidth > 60) {
                painter.setPen(Qt::white);
                QFont font = painter.font();
                font.setPointSize(9);
                font.setBold(true);
                painter.setFont(font);
                
                QString text = regionLabel;
                if (!region.getPatternId().empty()) {
                    text = QString::fromStdString(region.getPatternId());
                }
                
                painter.drawText(regionX + 5, regionY + 5, regionWidth - 10, regionHeight - 10,
                               Qt::AlignLeft | Qt::AlignVCenter, text);
            }
        }
        
        // Track separator
        painter.setPen(QColor(0x2a, 0x2a, 0x2a));
        painter.drawLine(0, y + TRACK_HEIGHT, width, y + TRACK_HEIGHT);
        
        y += TRACK_HEIGHT + TRACK_SPACING;
    }
}

void TimelineCanvas::drawPlayhead(QPainter& painter, int height) {
    if (!engine_) return;
    
    const auto& state = engine_->getTransport().getState();
    int x = tickToPixel(state.tick);
    
    // Playhead line - always visible for positioning
    painter.setPen(QPen(QColor(0x00, 0xaa, 0xff), 2));
    painter.drawLine(x, 0, x, height);
    
    // Playhead triangle at top
    QPolygon triangle;
    triangle << QPoint(x, 0) << QPoint(x - 6, 10) << QPoint(x + 6, 10);
    painter.setBrush(QColor(0x00, 0xaa, 0xff));
    painter.drawPolygon(triangle);
}

TimelineCanvas::RegionHitInfo TimelineCanvas::hitTestRegion(const QPoint& pos) const {
    RegionHitInfo info;
    info.isValid = false;
    
    if (!project_ || pos.y() < RULER_HEIGHT) {
        return info;
    }
    
    int y = RULER_HEIGHT + TRACK_SPACING;
    
    for (size_t trackIdx = 0; trackIdx < project_->getTrackCount(); ++trackIdx) {
        const Track* track = project_->getTrack(trackIdx);
        if (!track) continue;
        
        int regionY = y + 25;
        int regionHeight = TRACK_HEIGHT - 30;
        
        if (pos.y() >= regionY && pos.y() <= regionY + regionHeight) {
            // Check regions in this track
            const auto& regions = track->getRegions();
            for (size_t regionIdx = 0; regionIdx < regions.size(); ++regionIdx) {
                const auto& region = regions[regionIdx];
                int regionX = tickToPixel(region.getStartTick());
                int regionWidth = tickToPixel(region.getLengthTicks());
                
                if (pos.x() >= regionX && pos.x() <= regionX + regionWidth) {
                    info.trackIndex = trackIdx;
                    info.regionIndex = regionIdx;
                    info.isValid = true;
                    
                    // Check edges
                    if (pos.x() < regionX + RESIZE_EDGE_WIDTH) {
                        info.isLeftEdge = true;
                    } else if (pos.x() > regionX + regionWidth - RESIZE_EDGE_WIDTH) {
                        info.isRightEdge = true;
                    } else {
                        info.isBody = true;
                    }
                    return info;
                }
            }
        }
        
        y += TRACK_HEIGHT + TRACK_SPACING;
    }
    
    return info;
}

void TimelineCanvas::updateCursor(const QPoint& pos) {
    // Check if hovering over playhead
    if (engine_) {
        const auto& state = engine_->getTransport().getState();
        int playheadX = tickToPixel(state.tick);
        if (std::abs(pos.x() - playheadX) <= 6) {
            setCursor(Qt::SizeHorCursor);
            return;
        }
    }
    
    auto hitInfo = hitTestRegion(pos);
    
    if (hitInfo.isValid) {
        if (hitInfo.isLeftEdge || hitInfo.isRightEdge) {
            setCursor(Qt::SizeHorCursor);
        } else if (hitInfo.isBody) {
            setCursor(Qt::OpenHandCursor);
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void TimelineCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !project_) {
        QWidget::mousePressEvent(event);
        return;
    }
    
    // Check if clicking on playhead first
    if (engine_) {
        const auto& state = engine_->getTransport().getState();
        int playheadX = tickToPixel(state.tick);
        if (std::abs(event->pos().x() - playheadX) <= 6) {
            // Clicked on playhead - start dragging it
            interactionMode_ = InteractionMode::DraggingPlayhead;
            dragStartPos_ = event->pos();
            setCursor(Qt::SizeHorCursor);
            return;
        }
    }
    
    draggedRegion_ = hitTestRegion(event->pos());
    
    if (draggedRegion_.isValid) {
        // Track this as the selected region for keyboard operations
        selectedRegion_ = draggedRegion_;
        
        Track* track = const_cast<Project*>(project_)->getTrack(draggedRegion_.trackIndex);
        if (!track || draggedRegion_.regionIndex >= track->getRegions().size()) {
            return;
        }
        
        const auto& regions = track->getRegions();
        const Region& region = regions[draggedRegion_.regionIndex];
        
        dragStartPos_ = event->pos();
        dragStartTick_ = region.getStartTick();
        dragOriginalLength_ = region.getLengthTicks();
        
        if (draggedRegion_.isLeftEdge) {
            interactionMode_ = InteractionMode::ResizingLeft;
            setCursor(Qt::SizeHorCursor);
        } else if (draggedRegion_.isRightEdge) {
            interactionMode_ = InteractionMode::ResizingRight;
            setCursor(Qt::SizeHorCursor);
        } else if (draggedRegion_.isBody) {
            interactionMode_ = InteractionMode::DraggingRegion;
            setCursor(Qt::ClosedHandCursor);
        }
    } else {
        // Clicked on empty space - move playhead to clicked position
        if (engine_) {
            Tick clickedTick = pixelToTick(event->pos().x());
            // Snap to beat grid
            Tick snapSize = PPQ;
            clickedTick = (clickedTick / snapSize) * snapSize;
            engine_->getTransport().setPosition(clickedTick);
            update();
        }
    }
}

void TimelineCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (interactionMode_ == InteractionMode::None) {
        updateCursor(event->pos());
        QWidget::mouseMoveEvent(event);
        return;
    }
    
    // Handle playhead dragging
    if (interactionMode_ == InteractionMode::DraggingPlayhead) {
        if (engine_) {
            Tick newTick = pixelToTick(event->pos().x());
            // Snap to beat grid
            Tick snapSize = PPQ;
            newTick = std::max(Tick(0), (newTick / snapSize) * snapSize);
            engine_->getTransport().setPosition(newTick);
            update();
        }
        return;
    }
    
    if (!project_) return;
    
    Track* track = project_->getTrack(draggedRegion_.trackIndex);
    if (!track || draggedRegion_.regionIndex >= track->getRegions().size()) {
        return;
    }
    
    // Get mutable region
    auto& regions = const_cast<std::vector<Region>&>(track->getRegions());
    Region& region = regions[draggedRegion_.regionIndex];
    
    int deltaX = event->pos().x() - dragStartPos_.x();
    Tick deltaTicks = pixelToTick(deltaX);
    
    // Snap to beats (PPQ)
    Tick snapSize = PPQ;
    
    switch (interactionMode_) {
        case InteractionMode::DraggingRegion: {
            Tick newStartTick = dragStartTick_ + deltaTicks;
            // Snap to grid
            newStartTick = (newStartTick / snapSize) * snapSize;
            // Clamp to positive values
            if (newStartTick < 0) newStartTick = 0;
            region.setStartTick(newStartTick);
            update();
            break;
        }
        
        case InteractionMode::ResizingLeft: {
            Tick newStartTick = dragStartTick_ + deltaTicks;
            newStartTick = (newStartTick / snapSize) * snapSize;
            
            Tick originalEnd = dragStartTick_ + dragOriginalLength_;
            Tick newLength = originalEnd - newStartTick;
            
            // Minimum length: 1 beat
            if (newLength >= PPQ && newStartTick >= 0) {
                // Check for overlap with previous region
                if (draggedRegion_.regionIndex > 0) {
                    Region& prevRegion = regions[draggedRegion_.regionIndex - 1];
                    if (newStartTick < prevRegion.getEndTick()) {
                        // Shrink previous region to make room
                        Tick prevNewLength = newStartTick - prevRegion.getStartTick();
                        if (prevNewLength >= PPQ) {
                            prevRegion.setLengthTicks(prevNewLength);
                        } else {
                            // Can't shrink further, limit our resize
                            newStartTick = prevRegion.getStartTick() + PPQ;
                            newLength = originalEnd - newStartTick;
                        }
                    }
                }
                
                region.setStartTick(newStartTick);
                region.setLengthTicks(newLength);
                update();
            }
            break;
        }
        
        case InteractionMode::ResizingRight: {
            Tick newLength = dragOriginalLength_ + deltaTicks;
            newLength = (newLength / snapSize) * snapSize;
            
            // Minimum length: 1 beat
            if (newLength >= PPQ) {
                Tick newEndTick = region.getStartTick() + newLength;
                
                // Check for overlap with next region
                if (draggedRegion_.regionIndex < regions.size() - 1) {
                    Region& nextRegion = regions[draggedRegion_.regionIndex + 1];
                    if (newEndTick > nextRegion.getStartTick()) {
                        // Shrink next region from the left
                        Tick nextOriginalEnd = nextRegion.getEndTick();
                        Tick nextNewLength = nextOriginalEnd - newEndTick;
                        
                        if (nextNewLength >= PPQ) {
                            nextRegion.setStartTick(newEndTick);
                            nextRegion.setLengthTicks(nextNewLength);
                        } else {
                            // Can't shrink further, limit our resize
                            newLength = nextRegion.getStartTick() - region.getStartTick();
                        }
                    }
                }
                
                region.setLengthTicks(newLength);
                update();
            }
            break;
        }
        
        default:
            break;
    }
}

void TimelineCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        interactionMode_ = InteractionMode::None;
        updateCursor(event->pos());
    }
    QWidget::mouseReleaseEvent(event);
}

void TimelineCanvas::leaveEvent(QEvent* event) {
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

void TimelineCanvas::contextMenuEvent(QContextMenuEvent* event) {
    auto hitInfo = hitTestRegion(event->pos());
    
    if (!hitInfo.isValid || !project_) {
        return;
    }
    
    // Get region info for display
    Track* track = project_->getTrack(hitInfo.trackIndex);
    if (!track || hitInfo.regionIndex >= track->getRegions().size()) {
        return;
    }
    
    const Region& region = track->getRegions()[hitInfo.regionIndex];
    QString regionName = QString::fromStdString(region.getPatternId());
    if (regionName.isEmpty()) {
        regionName = "Region";
    }
    
    // Create context menu
    QMenu contextMenu(this);
    
    QAction* setTimeSignatureAction = contextMenu.addAction("Set Time Signature...");
    contextMenu.addSeparator();
    
    QAction* deleteAction = contextMenu.addAction(QString("Delete '%1'").arg(regionName));
    deleteAction->setIcon(QIcon::fromTheme("edit-delete"));
    
    contextMenu.addSeparator();
    
    QAction* propertiesAction = contextMenu.addAction("Properties...");
    propertiesAction->setEnabled(false); // TODO: Implement in future
    
    // Show menu and handle action
    QAction* selectedAction = contextMenu.exec(event->globalPos());
    
    if (selectedAction == deleteAction) {
        deleteRegion(hitInfo.trackIndex, hitInfo.regionIndex);
    } else if (selectedAction == setTimeSignatureAction) {
        setRegionTimeSignature(hitInfo.trackIndex, hitInfo.regionIndex);
    }
}

void TimelineCanvas::deleteRegion(size_t trackIndex, size_t regionIndex) {
    if (!project_) return;
    
    Track* track = project_->getTrack(trackIndex);
    if (!track) return;
    
    // Get region ID before deletion
    const auto& regions = track->getRegions();
    if (regionIndex >= regions.size()) return;
    
    const std::string regionId = regions[regionIndex].getId();
    
    // Remove the region
    track->removeRegion(regionId);
    
    // Update display
    update();
}

void TimelineCanvas::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (selectedRegion_.isValid) {
            deleteRegion(selectedRegion_.trackIndex, selectedRegion_.regionIndex);
            selectedRegion_ = RegionHitInfo(); // Clear selection
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void TimelineCanvas::wheelEvent(QWheelEvent* event) {
    // Use wheel for zooming with Ctrl modifier, or allow default scrolling
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom in/out based on wheel direction
        double zoomFactor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
        double newZoom = pixelsPerBeat_ * zoomFactor;
        
        // Clamp zoom level
        newZoom = std::max(10.0, std::min(200.0, newZoom));
        
        if (newZoom != pixelsPerBeat_) {
            pixelsPerBeat_ = newZoom;
            setMinimumSize(sizeHint());
            updateGeometry();
            update();
        }
        
        event->accept();
    } else {
        // Allow default scroll behavior
        QWidget::wheelEvent(event);
    }
}

void TimelineCanvas::setRegionTimeSignature(size_t trackIndex, size_t regionIndex) {
    if (!project_ || !engine_) return;
    
    Track* track = project_->getTrack(trackIndex);
    if (!track) return;
    
    const auto& regions = track->getRegions();
    if (regionIndex >= regions.size()) return;
    
    const Region& region = regions[regionIndex];
    Tick regionStart = region.getStartTick();
    
    // Get current time signature at region start (need non-const access)
    MeterMap& meterMap = const_cast<Project*>(project_)->getMeterMap();
    TimeSignature currentSig = meterMap.getSignatureAt(regionStart);
    
    // Create dialog for custom time signature
    QDialog dialog(this);
    dialog.setWindowTitle("Set Time Signature");
    dialog.setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QLabel* infoLabel = new QLabel(
        QString("Set time signature at bar %1:").arg(meterMap.getBarIndexAt(regionStart) + 1),
        &dialog
    );
    layout->addWidget(infoLabel);
    
    // Numerator spinbox
    QHBoxLayout* numeratorLayout = new QHBoxLayout();
    QLabel* numeratorLabel = new QLabel("Beats per bar:", &dialog);
    QSpinBox* numeratorSpinBox = new QSpinBox(&dialog);
    numeratorSpinBox->setRange(1, 16);
    numeratorSpinBox->setValue(currentSig.numerator);
    numeratorLayout->addWidget(numeratorLabel);
    numeratorLayout->addWidget(numeratorSpinBox);
    layout->addLayout(numeratorLayout);
    
    // Denominator spinbox
    QHBoxLayout* denominatorLayout = new QHBoxLayout();
    QLabel* denominatorLabel = new QLabel("Beat unit:", &dialog);
    QSpinBox* denominatorSpinBox = new QSpinBox(&dialog);
    denominatorSpinBox->setRange(1, 16);
    denominatorSpinBox->setValue(currentSig.denominator);
    denominatorLayout->addWidget(denominatorLabel);
    denominatorLayout->addWidget(denominatorSpinBox);
    layout->addLayout(denominatorLayout);
    
    // Preview label
    QLabel* previewLabel = new QLabel(&dialog);
    previewLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #00aaff;");
    auto updatePreview = [&]() {
        previewLabel->setText(QString("%1/%2").arg(numeratorSpinBox->value()).arg(denominatorSpinBox->value()));
    };
    QObject::connect(numeratorSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), updatePreview);
    QObject::connect(denominatorSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), updatePreview);
    updatePreview();
    layout->addWidget(previewLabel, 0, Qt::AlignCenter);
    
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        TimeSignature newSig{numeratorSpinBox->value(), denominatorSpinBox->value()};
        
        // Snap to bar boundary
        Tick barStartTick = meterMap.getBarStartAt(regionStart);
        
        // Add meter change
        meterMap.addChange(barStartTick, newSig);
        
        // Determine pattern style from original pattern
        std::string originalPatternId = region.getPatternId();
        std::string patternStyle = "groove"; // default
        
        if (originalPatternId.find("fill") != std::string::npos) {
            patternStyle = "fill";
        } else if (originalPatternId.find("halftime") != std::string::npos) {
            patternStyle = "halftime";
        }
        
        // Generate pattern ID based on style and time signature
        Tick barLength = TimeUtils::ticksPerBar(newSig);
        std::string patternId = patternStyle + "_" + std::to_string(newSig.numerator) + "_" + std::to_string(newSig.denominator);
        
        // Check if pattern already exists, if not create it
        if (!const_cast<Project*>(project_)->getPatternLibrary().hasPattern(patternId)) {
            // Capitalize first letter for display
            QString styleDisplay = QString::fromStdString(patternStyle);
            styleDisplay[0] = styleDisplay[0].toUpper();
            
            QString displayName = QString("%1 (%2/%3)")
                .arg(styleDisplay)
                .arg(newSig.numerator)
                .arg(newSig.denominator);
            
            Pattern newPattern(patternId, displayName.toStdString(), barLength);
            
            if (patternStyle == "fill") {
                // Fill pattern: rapid snare hits with increasing velocity, crash at end
                int subdivisions = 16;
                for (int i = 0; i < subdivisions; ++i) {
                    Tick pos = (barLength * i) / subdivisions;
                    float velocity = 0.6f + (i * 0.02f);
                    newPattern.addNote({1, pos, velocity}); // Snare
                }
                newPattern.addNote({3, barLength - 10, 0.9f}); // Crash at end
                
            } else if (patternStyle == "halftime") {
                // Half-time pattern: kick on 1, snare on 3, hi-hat on half beats
                newPattern.addNote({0, 0, 0.9f}); // Kick on 1
                if (newSig.numerator >= 3) {
                    newPattern.addNote({1, PPQ * 2, 0.85f}); // Snare on 3
                }
                // Hi-hat on half the beats
                for (int beat = 0; beat < newSig.numerator; beat += 2) {
                    newPattern.addNote({2, PPQ * beat, 0.65f});
                }
                
            } else {
                // Groove pattern: standard kick/snare/hi-hat
                // Kick on beat 1
                newPattern.addNote({0, 0, 0.9f});
                
                // Snare on backbeats
                if (newSig.numerator >= 4) {
                    newPattern.addNote({1, PPQ * 2, 0.8f});  // Beat 3
                    newPattern.addNote({1, PPQ * (newSig.numerator - 1), 0.8f});  // Last beat
                } else if (newSig.numerator == 3) {
                    newPattern.addNote({1, PPQ * 2, 0.8f});  // Beat 3
                } else if (newSig.numerator == 2) {
                    newPattern.addNote({1, PPQ, 0.8f});  // Beat 2
                }
                
                // Hi-hat on every beat
                for (int beat = 0; beat < newSig.numerator; ++beat) {
                    float velocity = (beat == 0) ? 0.7f : 0.55f;
                    newPattern.addNote({2, PPQ * beat, velocity});
                }
            }
            
            const_cast<Project*>(project_)->getPatternLibrary().addPattern(newPattern);
        }
        
        // Update the region to use the new pattern
        Track* mutableTrack = const_cast<Track*>(track);
        auto& mutableRegions = const_cast<std::vector<Region>&>(mutableTrack->getRegions());
        if (regionIndex < mutableRegions.size()) {
            mutableRegions[regionIndex].setPatternId(patternId);
            mutableRegions[regionIndex].setLengthTicks(barLength);
        }
        
        // Update display
        update();
    }
}

int TimelineCanvas::hitTestTrack(const QPoint& pos) const {
    if (!project_ || pos.y() < RULER_HEIGHT) {
        return -1;
    }
    
    int y = RULER_HEIGHT + TRACK_SPACING;
    
    for (size_t i = 0; i < project_->getTrackCount(); ++i) {
        if (pos.y() >= y && pos.y() < y + TRACK_HEIGHT) {
            return static_cast<int>(i);
        }
        y += TRACK_HEIGHT + TRACK_SPACING;
    }
    
    return -1;
}

void TimelineCanvas::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasText()) {
        QString data = event->mimeData()->text();
        if (data.startsWith("pattern:")) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void TimelineCanvas::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasText()) {
        QString data = event->mimeData()->text();
        if (data.startsWith("pattern:")) {
            int trackIndex = hitTestTrack(event->position().toPoint());
            if (trackIndex >= 0) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void TimelineCanvas::dropEvent(QDropEvent* event) {
    if (!project_ || !event->mimeData()->hasText()) {
        event->ignore();
        return;
    }
    
    QString data = event->mimeData()->text();
    if (!data.startsWith("pattern:")) {
        event->ignore();
        return;
    }
    
    // Extract pattern ID
    QString patternId = data.mid(8); // Remove "pattern:" prefix
    
    // Find drop position
    QPoint pos = event->position().toPoint();
    int trackIndex = hitTestTrack(pos);
    
    if (trackIndex < 0) {
        event->ignore();
        return;
    }
    
    Track* track = project_->getTrack(trackIndex);
    if (!track) {
        event->ignore();
        return;
    }
    
    // Convert pixel position to tick (snap to beat grid)
    Tick dropTick = pixelToTick(pos.x());
    Tick snapSize = PPQ; // Snap to beats
    dropTick = (dropTick / snapSize) * snapSize;
    if (dropTick < 0) dropTick = 0;
    
    // Get pattern to determine default length
    const Pattern* pattern = project_->getPatternLibrary().getPattern(patternId.toStdString());
    Tick regionLength = PPQ * 4; // Default: 1 bar in 4/4
    if (pattern) {
        regionLength = pattern->getLengthTicks();
    }
    
    // Create new region
    static int regionCounter = 1000; // Simple ID generator
    std::string regionId = "region_" + std::to_string(regionCounter++);
    
    // Infer region type from pattern name
    RegionType regionType = RegionType::Groove;  // Default
    QString patternName = patternId.toLower();
    if (patternName.contains("fill")) {
        regionType = RegionType::Fill;
    } else if (patternName.contains("groove") || patternName.contains("beat") || patternName.contains("basic")) {
        regionType = RegionType::Groove;
    }
    
    Region newRegion(regionId, regionType, dropTick, regionLength);
    newRegion.setPatternId(patternId.toStdString());
    
    // Auto-detect time signature from pattern length and add meter change
    if (pattern && engine_) {
        // Infer time signature from pattern length
        Tick patternLen = pattern->getLengthTicks();
        TimeSignature inferredSig{4, 4}; // Default
        
        // Common time signatures
        if (patternLen == PPQ * 3) {
            inferredSig = {3, 4};
        } else if (patternLen == PPQ * 5) {
            inferredSig = {5, 4};
        } else if (patternLen == PPQ * 6) {
            inferredSig = {6, 8};
        } else if (patternLen == PPQ * 7) {
            inferredSig = {7, 8};
        } else if (patternLen == PPQ * 2) {
            inferredSig = {2, 4};
        }
        
        // Get current time signature at drop position
        auto& meterMap = project_->getMeterMap();
        TimeSignature currentSig = meterMap.getSignatureAt(dropTick);
        
        // Only add meter change if different from current signature
        if (inferredSig.numerator != currentSig.numerator || 
            inferredSig.denominator != currentSig.denominator) {
            // Snap to bar boundary for meter change
            Tick barStartTick = meterMap.getBarStartAt(dropTick);
            meterMap.addChange(barStartTick, inferredSig);
        }
    }
    
    // Check for overlaps and adjust if needed
    const auto& existingRegions = track->getRegions();
    for (const auto& existing : existingRegions) {
        if (newRegion.overlaps(existing)) {
            // Try to place after the overlapping region
            dropTick = existing.getEndTick();
            newRegion.setStartTick(dropTick);
        }
    }
    
    // Add region to track
    track->addRegion(newRegion);
    
    event->acceptProposedAction();
    update();
}

// ============================================================================
// TimelineWidget Implementation
// ============================================================================

TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar with zoom controls
    QWidget* toolbar = new QWidget(this);
    toolbar->setStyleSheet("QWidget { background-color: #2a2a2a; border-bottom: 1px solid #3a3a3a; }");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);
    
    QLabel* zoomLabel = new QLabel("Zoom:", this);
    zoomLabel->setStyleSheet("color: #b0b0b0;");
    
    QPushButton* zoomInBtn = new QPushButton("+", this);
    zoomInBtn->setMaximumWidth(35);
    zoomInBtn->setMaximumHeight(28);
    zoomInBtn->setStyleSheet("font-size: 18px; font-weight: bold;");
    zoomInBtn->setToolTip("Zoom In (Ctrl++)");
    connect(zoomInBtn, &QPushButton::clicked, this, &TimelineWidget::zoomIn);
    
    QPushButton* zoomOutBtn = new QPushButton("-", this);
    zoomOutBtn->setMaximumWidth(35);
    zoomOutBtn->setMaximumHeight(28);
    zoomOutBtn->setStyleSheet("font-size: 18px; font-weight: bold;");
    zoomOutBtn->setToolTip("Zoom Out (Ctrl+-)");
    connect(zoomOutBtn, &QPushButton::clicked, this, &TimelineWidget::zoomOut);
    
    QPushButton* resetZoomBtn = new QPushButton("⊙", this);
    resetZoomBtn->setMaximumWidth(35);
    resetZoomBtn->setMaximumHeight(28);
    resetZoomBtn->setStyleSheet("font-size: 16px;");
    resetZoomBtn->setToolTip("Reset Zoom (Ctrl+0)");
    connect(resetZoomBtn, &QPushButton::clicked, this, &TimelineWidget::resetZoom);
    
    toolbarLayout->addWidget(zoomLabel);
    toolbarLayout->addWidget(zoomInBtn);
    toolbarLayout->addWidget(zoomOutBtn);
    toolbarLayout->addWidget(resetZoomBtn);
    toolbarLayout->addStretch();
    
    mainLayout->addWidget(toolbar);
    
    // Timeline canvas in scroll area
    canvas_ = new TimelineCanvas(this);
    
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidget(canvas_);
    scrollArea_->setWidgetResizable(false);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Install event filter to intercept wheel events for zooming
    scrollArea_->viewport()->installEventFilter(this);
    
    mainLayout->addWidget(scrollArea_, 1);
}

void TimelineWidget::setProject(Project* project) {
    canvas_->setProject(project);
    canvas_->setMinimumSize(canvas_->sizeHint());
    canvas_->updateGeometry();
    canvas_->update();
}

void TimelineWidget::setEngine(Engine* engine) {
    canvas_->setEngine(engine);
}

void TimelineWidget::updatePlayhead() {
    if (canvas_) {
        canvas_->update();
    }
}

void TimelineWidget::zoomIn() {
    double currentZoom = canvas_->getPixelsPerBeat();
    canvas_->setPixelsPerBeat(currentZoom * 1.2);
    canvas_->setMinimumSize(canvas_->sizeHint());
    canvas_->updateGeometry();
    canvas_->update();
}

void TimelineWidget::zoomOut() {
    double currentZoom = canvas_->getPixelsPerBeat();
    canvas_->setPixelsPerBeat(currentZoom / 1.2);
    canvas_->setMinimumSize(canvas_->sizeHint());
    canvas_->updateGeometry();
    canvas_->update();
}

void TimelineWidget::resetZoom() {
    canvas_->setPixelsPerBeat(40.0);
    canvas_->setMinimumSize(canvas_->sizeHint());
    canvas_->updateGeometry();
    canvas_->update();
}

bool TimelineWidget::eventFilter(QObject* obj, QEvent* event) {
    // Intercept wheel events on the scroll area viewport
    if (event->type() == QEvent::Wheel && obj == scrollArea_->viewport()) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        // Handle zoom with or without Ctrl modifier
        double zoomFactor = (wheelEvent->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
        double currentZoom = canvas_->getPixelsPerBeat();
        double newZoom = currentZoom * zoomFactor;
        
        // Clamp zoom level
        newZoom = std::max(10.0, std::min(200.0, newZoom));
        
        if (newZoom != currentZoom) {
            canvas_->setPixelsPerBeat(newZoom);
            canvas_->setMinimumSize(canvas_->sizeHint());
            canvas_->updateGeometry();
            canvas_->update();
        }
        
        return true; // Event handled, don't pass to scroll area
    }
    return QWidget::eventFilter(obj, event);
}

void TimelineWidget::wheelEvent(QWheelEvent* event) {
    // Handle Ctrl+Wheel for zooming
    if (event->modifiers() & Qt::ControlModifier) {
        double zoomFactor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
        double currentZoom = canvas_->getPixelsPerBeat();
        double newZoom = currentZoom * zoomFactor;
        
        // Clamp zoom level
        newZoom = std::max(10.0, std::min(200.0, newZoom));
        
        if (newZoom != currentZoom) {
            canvas_->setPixelsPerBeat(newZoom);
            canvas_->setMinimumSize(canvas_->sizeHint());
            canvas_->updateGeometry();
            canvas_->update();
        }
        
        event->accept();
    } else {
        // Allow default scroll behavior
        QWidget::wheelEvent(event);
    }
}

} // namespace beater
