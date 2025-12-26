#include "ui/PatternPalette.hpp"
#include "domain/Project.hpp"
#include "domain/Pattern.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

namespace beater {

// ============================================================================
// PatternPaletteItem Implementation
// ============================================================================

PatternPaletteItem::PatternPaletteItem(const QString& patternId, const QString& displayName,
                                       const QString& color, QWidget* parent)
    : QWidget(parent), patternId_(patternId), displayName_(displayName), color_(color) {
    setMinimumWidth(120);
    setMaximumWidth(180);
    setMinimumHeight(50);
    setMaximumHeight(50);
    setCursor(Qt::OpenHandCursor);
    
    setStyleSheet(QString(
        "PatternPaletteItem {"
        "    background-color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 4px;"
        "    margin: 2px;"
        "}"
        "PatternPaletteItem:hover {"
        "    border: 2px solid #00aaff;"
        "}"
    ).arg(color).arg(color_.lighter(150).name()));
}

void PatternPaletteItem::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData();
        
        mimeData->setText("pattern:" + patternId_);
        drag->setMimeData(mimeData);
        
        // Create drag pixmap
        QPixmap pixmap(width(), height());
        render(&pixmap);
        drag->setPixmap(pixmap);
        drag->setHotSpot(event->pos());
        
        setCursor(Qt::ClosedHandCursor);
        drag->exec(Qt::CopyAction);
        setCursor(Qt::OpenHandCursor);
    }
}

void PatternPaletteItem::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw pattern name centered
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(10);
    painter.setFont(font);
    
    painter.drawText(rect(), Qt::AlignCenter, displayName_);
}

// ============================================================================
// PatternPalette Implementation
// ============================================================================

PatternPalette::PatternPalette(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void PatternPalette::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);
    
    // Patterns section
    QGroupBox* patternsGroup = new QGroupBox("Patterns", this);
    QHBoxLayout* patternsLayout = new QHBoxLayout(patternsGroup);
    patternsLayout->setSpacing(8);
    
    patternListWidget_ = new QWidget(this);
    QHBoxLayout* listLayout = new QHBoxLayout(patternListWidget_);
    listLayout->setContentsMargins(8, 4, 8, 4);
    listLayout->setSpacing(8);
    
    // Add placeholder text
    QLabel* placeholderLabel = new QLabel("No patterns loaded", this);
    placeholderLabel->setStyleSheet("color: #808080; font-style: italic; padding: 4px;");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    listLayout->addWidget(placeholderLabel);
    
    patternsLayout->addWidget(patternListWidget_);
    mainLayout->addWidget(patternsGroup);
}

void PatternPalette::setProject(Project* project) {
    project_ = project;
    refreshPatterns();
}

void PatternPalette::refreshPatterns() {
    if (!project_) return;
    
    // Clear existing pattern items
    QLayout* listLayout = patternListWidget_->layout();
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    const PatternLibrary& library = project_->getPatternLibrary();
    const auto& patterns = library.getPatterns();
    
    if (patterns.empty()) {
        QLabel* emptyLabel = new QLabel("No patterns available.", this);
        emptyLabel->setStyleSheet("color: #808080; font-style: italic; padding: 20px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        listLayout->addWidget(emptyLabel);
        listLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        return;
    }
    
    // Add pattern items
    for (const auto& pattern : patterns) {
        QString color;
        QString name = QString::fromStdString(pattern.getName());
        
        // Color-code by pattern name heuristics
        if (name.contains("groove", Qt::CaseInsensitive) || 
            name.contains("beat", Qt::CaseInsensitive)) {
            color = "#2d5a2d";  // Green for grooves
        } else if (name.contains("fill", Qt::CaseInsensitive)) {
            color = "#5a3d1a";  // Orange-brown for fills
        } else if (name.contains("half", Qt::CaseInsensitive) ||
                   name.contains("time", Qt::CaseInsensitive)) {
            color = "#2d2d5a";  // Blue for variations
        } else {
            color = "#3a3a3a";  // Gray for others
        }
        
        PatternPaletteItem* item = new PatternPaletteItem(
            QString::fromStdString(pattern.getId()),
            name,
            color,
            this
        );
        
        listLayout->addWidget(item);
    }
    
    listLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

} // namespace beater
