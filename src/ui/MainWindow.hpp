#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

namespace beater {

class Engine;
class TimelineWidget;
class Project;
class PatternPalette;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;
    
    // Set the engine instance (owned externally)
    void setEngine(Engine* engine);
    void setProject(Project* project);
    
private slots:
    void onPlayClicked();
    void onStopClicked();
    void onRewindClicked();
    void updatePlayhead();
    void onTempoChanged(double value);
    void onLoopToggled(bool checked);
    void onSettingsClicked();
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    
private:
    void setupUI();
    void applyDarkTheme();
    QWidget* createTransportControls();
    QWidget* createTransportInfo();
    QWidget* createTimelineView();
    void createMenuBar();
    void createStatusBar();
    
    Engine* engine_ = nullptr;
    Project* project_ = nullptr;
    QString currentFilePath_;
    
    // UI elements
    QPushButton* playButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPushButton* rewindButton_ = nullptr;
    QLabel* positionLabel_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QLabel* barBeatLabel_ = nullptr;
    QLabel* tempoLabel_ = nullptr;
    QLabel* meterLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QDoubleSpinBox* tempoSpinBox_ = nullptr;
    QCheckBox* loopCheckBox_ = nullptr;
    QSpinBox* loopStartSpinBox_ = nullptr;
    QSpinBox* loopEndSpinBox_ = nullptr;
    QTimer* updateTimer_ = nullptr;
    TimelineWidget* timelineWidget_ = nullptr;
    PatternPalette* patternPalette_ = nullptr;
};

} // namespace beater
