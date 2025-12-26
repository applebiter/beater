#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>

namespace beater {

class Project;

// Draggable pattern item for the palette
class PatternPaletteItem : public QWidget {
    Q_OBJECT
    
public:
    PatternPaletteItem(const QString& patternId, const QString& displayName, 
                       const QString& color, QWidget* parent = nullptr);
    
    QString getPatternId() const { return patternId_; }
    QString getDisplayName() const { return displayName_; }
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    
private:
    QString patternId_;
    QString displayName_;
    QColor color_;
};

// Pattern and block palette
class PatternPalette : public QWidget {
    Q_OBJECT
    
public:
    explicit PatternPalette(QWidget* parent = nullptr);
    
    void setProject(Project* project);
    void refreshPatterns();
    
signals:
    void patternDragStarted(const QString& patternId);
    
private:
    void setupUI();
    
    Project* project_ = nullptr;
    QWidget* patternListWidget_ = nullptr;
};

} // namespace beater
