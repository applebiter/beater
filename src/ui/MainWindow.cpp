#include "ui/MainWindow.hpp"
#include "ui/TimelineWidget.hpp"
#include "ui/PatternPalette.hpp"
#include "engine/Engine.hpp"
#include "domain/Project.hpp"
#include "serialization/ProjectSerializer.hpp"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QTimer>
#include <QStatusBar>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFrame>
#include <QScrollArea>
#include <QIcon>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>

namespace beater {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    applyDarkTheme();
    setupUI();
    
    // Create update timer for playhead position
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updatePlayhead);
    updateTimer_->start(50);  // Update at 20 Hz
}

void MainWindow::applyDarkTheme() {
    QString darkStyleSheet = R"(
        QMainWindow {
            background-color: #1e1e1e;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #e0e0e0;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            background-color: #252525;
            border: 1px solid #3a3a3a;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: bold;
            color: #00aaff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 2px 8px;
            color: #00aaff;
        }
        QPushButton {
            background-color: #3a3a3a;
            color: #e0e0e0;
            border: 1px solid #505050;
            border-radius: 4px;
            padding: 8px 20px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
            border: 1px solid #00aaff;
        }
        QPushButton:pressed {
            background-color: #2a2a2a;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #666666;
        }
        QPushButton#playButton {
            background-color: #2d5a2d;
            border: 1px solid #4a8a4a;
        }
        QPushButton#playButton:hover {
            background-color: #3d6a3d;
            border: 1px solid #5aaa5a;
        }
        QPushButton#stopButton {
            background-color: #5a2d2d;
            border: 1px solid #8a4a4a;
        }
        QPushButton#stopButton:hover {
            background-color: #6a3d3d;
            border: 1px solid #aa5a5a;
        }
        QLabel {
            background-color: transparent;
            color: #e0e0e0;
        }
        QSpinBox, QDoubleSpinBox {
            background-color: #2a2a2a;
            color: #e0e0e0;
            border: 1px solid #3a3a3a;
            border-radius: 3px;
            padding: 4px 8px;
            selection-background-color: #00aaff;
        }
        QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #00aaff;
        }
        QCheckBox {
            color: #e0e0e0;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid #505050;
            border-radius: 3px;
            background-color: #2a2a2a;
        }
        QCheckBox::indicator:checked {
            background-color: #00aaff;
            border: 1px solid #0088cc;
        }
        QCheckBox::indicator:hover {
            border: 1px solid #00aaff;
        }
        QStatusBar {
            background-color: #252525;
            color: #b0b0b0;
            border-top: 1px solid #3a3a3a;
        }
        QMenuBar {
            background-color: #252525;
            color: #e0e0e0;
            border-bottom: 1px solid #3a3a3a;
        }
        QMenuBar::item:selected {
            background-color: #3a3a3a;
        }
        QMenu {
            background-color: #252525;
            color: #e0e0e0;
            border: 1px solid #3a3a3a;
        }
        QMenu::item:selected {
            background-color: #00aaff;
        }
        QScrollArea {
            border: none;
            background-color: #1e1e1e;
        }
        QFrame {
            background-color: #252525;
        }
    )";
    setStyleSheet(darkStyleSheet);
}

void MainWindow::setEngine(Engine* engine) {
    engine_ = engine;
    
    if (engine_) {
        statusLabel_->setText(QString("🟢 Engine Ready | Sample Rate: %1 Hz | Buffer: %2 frames")
                            .arg(engine_->getSampleRate())
                            .arg(engine_->getBufferSize()));
        
        // Set initial tempo from engine
        const auto& state = engine_->getTransport().getState();
        tempoSpinBox_->setValue(state.bpm);
        meterLabel_->setText(QString("%1/%2").arg(state.signature.numerator).arg(state.signature.denominator));
        
        // Connect timeline widget to engine and project
        if (timelineWidget_) {
            timelineWidget_->setEngine(engine_);
            // Access project from engine - assuming engine has getProject()
            // For now we'll need to pass project separately
        }
    }
}

void MainWindow::setProject(Project* project) {
    project_ = project;
    if (timelineWidget_) {
        timelineWidget_->setProject(project);
    }
    if (patternPalette_) {
        patternPalette_->setProject(project);
    }
}

void MainWindow::setupUI() {
    setWindowTitle("Beater Drum Machine v0.1.0");
    // Note: Icon resource temporarily disabled - will fix in Phase 6
    // setWindowIcon(QIcon(":/beater.svg"));
    resize(1400, 900);
    
    // Menu bar
    createMenuBar();
    
    // Central widget with vertical layout
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    // Top section: Transport controls, info, and pattern palette
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(8);
    topLayout->addWidget(createTransportControls());
    topLayout->addWidget(createTransportInfo());
    
    // Pattern palette (inline, horizontal)
    patternPalette_ = new PatternPalette(this);
    topLayout->addWidget(patternPalette_, 1);
    
    mainLayout->addLayout(topLayout);
    
    // Timeline view (full width)
    mainLayout->addWidget(createTimelineView(), 1);
    
    // Status bar
    createStatusBar();
    
    setCentralWidget(centralWidget);
}

QWidget* MainWindow::createTransportControls() {
    QGroupBox* transportGroup = new QGroupBox("Transport Controls", this);
    transportGroup->setMaximumWidth(500);
    QVBoxLayout* mainLayout = new QVBoxLayout(transportGroup);
    
    // Transport buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // Play button
    playButton_ = new QPushButton("▶  Play", this);
    playButton_->setObjectName("playButton");
    playButton_->setMinimumHeight(50);
    playButton_->setMinimumWidth(120);
    connect(playButton_, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    
    // Stop button
    stopButton_ = new QPushButton("⏹  Stop", this);
    stopButton_->setObjectName("stopButton");
    stopButton_->setMinimumHeight(50);
    stopButton_->setMinimumWidth(120);
    stopButton_->setEnabled(false);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    
    // Rewind button
    rewindButton_ = new QPushButton("⏮  Rewind", this);
    rewindButton_->setMinimumHeight(50);
    rewindButton_->setMinimumWidth(120);
    connect(rewindButton_, &QPushButton::clicked, this, &MainWindow::onRewindClicked);
    
    buttonLayout->addWidget(rewindButton_);
    buttonLayout->addWidget(playButton_);
    buttonLayout->addWidget(stopButton_);
    mainLayout->addLayout(buttonLayout);
    
    // Loop controls
    QHBoxLayout* loopLayout = new QHBoxLayout();
    loopCheckBox_ = new QCheckBox("Loop", this);
    connect(loopCheckBox_, &QCheckBox::toggled, this, &MainWindow::onLoopToggled);
    
    loopStartSpinBox_ = new QSpinBox(this);
    loopStartSpinBox_->setPrefix("Bar ");
    loopStartSpinBox_->setMinimum(1);
    loopStartSpinBox_->setMaximum(100);
    loopStartSpinBox_->setValue(1);
    loopStartSpinBox_->setEnabled(false);
    
    QLabel* toLabel = new QLabel("to", this);
    
    loopEndSpinBox_ = new QSpinBox(this);
    loopEndSpinBox_->setPrefix("Bar ");
    loopEndSpinBox_->setMinimum(1);
    loopEndSpinBox_->setMaximum(100);
    loopEndSpinBox_->setValue(14);
    loopEndSpinBox_->setEnabled(false);
    
    loopLayout->addWidget(loopCheckBox_);
    loopLayout->addWidget(loopStartSpinBox_);
    loopLayout->addWidget(toLabel);
    loopLayout->addWidget(loopEndSpinBox_);
    loopLayout->addStretch();
    mainLayout->addLayout(loopLayout);
    
    return transportGroup;
}

QWidget* MainWindow::createTransportInfo() {
    QGroupBox* infoGroup = new QGroupBox("Project Info", this);
    QVBoxLayout* mainLayout = new QVBoxLayout(infoGroup);
    
    // Position display
    QGroupBox* posGroup = new QGroupBox("Position", this);
    QHBoxLayout* posLayout = new QHBoxLayout(posGroup);
    
    timeLabel_ = new QLabel("0:00.000", this);
    timeLabel_->setStyleSheet("font-size: 28px; font-weight: bold; color: #00aaff; font-family: 'Consolas', monospace;");
    
    barBeatLabel_ = new QLabel("Bar 1 | Beat 1", this);
    barBeatLabel_->setStyleSheet("font-size: 18px; color: #b0b0b0;");
    
    positionLabel_ = new QLabel("Tick: 0", this);
    positionLabel_->setStyleSheet("font-size: 14px; color: #808080;");
    
    QVBoxLayout* posVLayout = new QVBoxLayout();
    posVLayout->addWidget(timeLabel_);
    posVLayout->addWidget(barBeatLabel_);
    posVLayout->addWidget(positionLabel_);
    posLayout->addLayout(posVLayout);
    mainLayout->addWidget(posGroup);
    
    // Tempo and meter
    QHBoxLayout* tempoMeterLayout = new QHBoxLayout();
    
    // Tempo control
    QVBoxLayout* tempoLayout = new QVBoxLayout();
    QLabel* tempoTitleLabel = new QLabel("Tempo (BPM)", this);
    tempoTitleLabel->setStyleSheet("font-size: 11px; color: #808080;");
    tempoSpinBox_ = new QDoubleSpinBox(this);
    tempoSpinBox_->setRange(40.0, 300.0);
    tempoSpinBox_->setValue(120.0);
    tempoSpinBox_->setDecimals(1);
    tempoSpinBox_->setSuffix(" BPM");
    tempoSpinBox_->setMinimumWidth(120);
    connect(tempoSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onTempoChanged);
    tempoLayout->addWidget(tempoTitleLabel);
    tempoLayout->addWidget(tempoSpinBox_);
    
    // Meter display
    QVBoxLayout* meterLayout = new QVBoxLayout();
    QLabel* meterTitleLabel = new QLabel("Time Signature", this);
    meterTitleLabel->setStyleSheet("font-size: 11px; color: #808080;");
    meterLabel_ = new QLabel("4/4", this);
    meterLabel_->setStyleSheet("font-size: 20px; font-weight: bold; color: #00aaff;");
    meterLabel_->setAlignment(Qt::AlignCenter);
    meterLayout->addWidget(meterTitleLabel);
    meterLayout->addWidget(meterLabel_);
    
    tempoMeterLayout->addLayout(tempoLayout);
    tempoMeterLayout->addSpacing(20);
    tempoMeterLayout->addLayout(meterLayout);
    tempoMeterLayout->addStretch();
    mainLayout->addLayout(tempoMeterLayout);
    
    return infoGroup;
}

QWidget* MainWindow::createTimelineView() {
    QGroupBox* timelineGroup = new QGroupBox("Timeline", this);
    QVBoxLayout* timelineLayout = new QVBoxLayout(timelineGroup);
    timelineLayout->setContentsMargins(4, 8, 4, 4);
    
    // Create timeline widget
    timelineWidget_ = new TimelineWidget(this);
    timelineLayout->addWidget(timelineWidget_, 1);
    
    return timelineGroup;
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    QAction* newAction = fileMenu->addAction("&New Project");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewProject);
    QAction* openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenProject);
    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProject);
    QAction* saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveProjectAs);
    fileMenu->addSeparator();
    QAction* settingsAction = fileMenu->addAction("Se&ttings...");
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Edit menu
    QMenu* editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction("&Undo")->setShortcut(QKeySequence::Undo);
    editMenu->addAction("&Redo")->setShortcut(QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction("Cu&t")->setShortcut(QKeySequence::Cut);
    editMenu->addAction("&Copy")->setShortcut(QKeySequence::Copy);
    editMenu->addAction("&Paste")->setShortcut(QKeySequence::Paste);
    
    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    viewMenu->addAction("Zoom &In")->setShortcut(QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom &Out")->setShortcut(QKeySequence::ZoomOut);
    viewMenu->addAction("&Reset Zoom");
    
    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About Beater");
    
    setMenuBar(menuBar);
}

void MainWindow::createStatusBar() {
    statusLabel_ = new QLabel("🔴 Engine: Not connected", this);
    statusBar()->addWidget(statusLabel_, 1);
}

void MainWindow::onPlayClicked() {
    if (engine_) {
        // Play from current transport position
        Tick currentTick = engine_->getTransport().getState().tick;
        engine_->playFromTick(currentTick);
        playButton_->setEnabled(false);
        stopButton_->setEnabled(true);
    }
}

void MainWindow::onStopClicked() {
    if (engine_) {
        engine_->stopPlayback();
        playButton_->setEnabled(true);
        stopButton_->setEnabled(false);
    }
}

void MainWindow::onRewindClicked() {
    if (engine_) {
        bool wasPlaying = engine_->isPlaying();
        if (wasPlaying) {
            engine_->stopPlayback();
        }
        engine_->playFromTick(0);
        if (!wasPlaying) {
            engine_->stopPlayback();
        }
    }
}

void MainWindow::onTempoChanged(double value) {
    if (engine_) {
        engine_->getTransport().setTempo(value);
    }
}

void MainWindow::onLoopToggled(bool checked) {
    loopStartSpinBox_->setEnabled(checked);
    loopEndSpinBox_->setEnabled(checked);
    // TODO: Implement loop functionality in engine
}

void MainWindow::onSettingsClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Settings");
    dialog.setModal(true);
    dialog.resize(600, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    
    // Sample directories section
    QGroupBox* sampleDirsGroup = new QGroupBox("Sample Directories", &dialog);
    QVBoxLayout* groupLayout = new QVBoxLayout(sampleDirsGroup);
    
    QLabel* infoLabel = new QLabel(
        "Add folders containing drum samples (.wav files).\n"
        "The application will search these folders for available samples.",
        &dialog
    );
    infoLabel->setWordWrap(true);
    groupLayout->addWidget(infoLabel);
    
    // List of directories
    QListWidget* dirList = new QListWidget(&dialog);
    
    // TODO: Load saved directories from settings
    // For now, add the default Hydrogen path
    dirList->addItem("/usr/share/hydrogen/data/drumkits");
    
    groupLayout->addWidget(dirList);
    
    // Buttons to add/remove directories
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* addButton = new QPushButton("Add Directory...", &dialog);
    connect(addButton, &QPushButton::clicked, [&]() {
        QString dir = QFileDialog::getExistingDirectory(
            &dialog,
            "Select Sample Directory",
            QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
        if (!dir.isEmpty()) {
            dirList->addItem(dir);
        }
    });
    
    QPushButton* removeButton = new QPushButton("Remove Selected", &dialog);
    connect(removeButton, &QPushButton::clicked, [&]() {
        qDeleteAll(dirList->selectedItems());
    });
    
    QPushButton* rescanButton = new QPushButton("Rescan Samples Now", &dialog);
    rescanButton->setStyleSheet("font-weight: bold; background-color: #00aaff; color: white;");
    connect(rescanButton, &QPushButton::clicked, [&]() {
        // Collect all directories
        QStringList dirs;
        for (int i = 0; i < dirList->count(); ++i) {
            dirs << dirList->item(i)->text();
        }
        
        if (dirs.isEmpty()) {
            QMessageBox::warning(&dialog, "No Directories", 
                "Please add at least one sample directory first.");
            return;
        }
        
        // Scan for samples
        QStringList foundSamples;
        for (const QString& dir : dirs) {
            QDir directory(dir);
            if (!directory.exists()) continue;
            
            // Recursively find all .wav files
            QDirIterator it(dir, QStringList() << "*.wav" << "*.WAV", 
                          QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                foundSamples << it.next();
            }
        }
        
        if (foundSamples.isEmpty()) {
            QMessageBox::information(&dialog, "No Samples Found",
                "No .wav files were found in the specified directories.");
            return;
        }
        
        // Show progress
        QMessageBox::information(&dialog, "Samples Found",
            QString("Found %1 sample files.\n\nSamples will be reloaded when you click OK.")
                .arg(foundSamples.size()));
    });
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(rescanButton);
    buttonLayout->addStretch();
    groupLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(sampleDirsGroup);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        // TODO: Save directories to settings file
        // For now, just show confirmation
        QStringList dirs;
        for (int i = 0; i < dirList->count(); ++i) {
            dirs << dirList->item(i)->text();
        }
        
        QMessageBox::information(
            this,
            "Settings Saved",
            QString("Sample directories updated:\n\n%1\n\n"
                    "Note: Restart the application to rescan samples.")
                .arg(dirs.join("\n"))
        );
        
        // TODO: Actually implement saving to a config file
        // and reloading samples from these directories
    }
}

void MainWindow::updatePlayhead() {
    if (engine_ && engine_->isPlaying()) {
        const auto& state = engine_->getTransport().getState();
        
        // Calculate time position
        double seconds = static_cast<double>(state.frame) / state.sampleRate;
        int minutes = static_cast<int>(seconds) / 60;
        double secs = seconds - (minutes * 60);
        
        // Calculate bar/beat position
        Tick ticksPerBeat = PPQ;
        Tick ticksPerBar = TimeUtils::ticksPerBar(state.signature);
        int bar = static_cast<int>(state.tick / ticksPerBar) + 1;
        int beat = static_cast<int>((state.tick % ticksPerBar) / ticksPerBeat) + 1;
        
        timeLabel_->setText(QString("%1:%2")
                          .arg(minutes)
                          .arg(secs, 6, 'f', 3, '0'));
        
        barBeatLabel_->setText(QString("Bar %1 | Beat %2")
                             .arg(bar)
                             .arg(beat));
        
        positionLabel_->setText(QString("Tick: %1").arg(state.tick));
        
        // Update timeline visualization
        if (timelineWidget_) {
            timelineWidget_->updatePlayhead();
        }
    } else {
        timeLabel_->setText("0:00.000");
        barBeatLabel_->setText("Bar 1 | Beat 1");
        positionLabel_->setText("Tick: 0");
        
        if (timelineWidget_) {
            timelineWidget_->updatePlayhead();
        }
    }
}

void MainWindow::onNewProject() {
    // TODO: Prompt to save current project if modified
    // For now, just clear the current filepath
    currentFilePath_.clear();
    setWindowTitle("Beater Drum Machine v0.1.0 - New Project");
    // Note: Would need to clear/reset project state, but Project doesn't have a clear() method yet
}

void MainWindow::onOpenProject() {
    if (!project_) return;
    
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Open Project",
        QString(),
        "Beater Projects (*.beater);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    if (ProjectSerializer::loadFromFile(*project_, filename.toStdString())) {
        currentFilePath_ = filename;
        setWindowTitle(QString("Beater Drum Machine v0.1.0 - %1").arg(QFileInfo(filename).fileName()));
        
        // Refresh UI
        if (timelineWidget_) {
            timelineWidget_->setProject(project_);
        }
        if (patternPalette_) {
            patternPalette_->setProject(project_);
        }
        
        QMessageBox::information(this, "Project Loaded", "Project loaded successfully!");
    } else {
        QMessageBox::critical(this, "Load Error", "Failed to load project file.");
    }
}

void MainWindow::onSaveProject() {
    if (currentFilePath_.isEmpty()) {
        onSaveProjectAs();
    } else {
        if (project_ && ProjectSerializer::saveToFile(*project_, currentFilePath_.toStdString())) {
            QMessageBox::information(this, "Project Saved", "Project saved successfully!");
        } else {
            QMessageBox::critical(this, "Save Error", "Failed to save project file.");
        }
    }
}

void MainWindow::onSaveProjectAs() {
    if (!project_) return;
    
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Save Project As",
        QString(),
        "Beater Projects (*.beater);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    // Add .beater extension if not present
    if (!filename.endsWith(".beater", Qt::CaseInsensitive)) {
        filename += ".beater";
    }
    
    if (ProjectSerializer::saveToFile(*project_, filename.toStdString())) {
        currentFilePath_ = filename;
        setWindowTitle(QString("Beater Drum Machine v0.1.0 - %1").arg(QFileInfo(filename).fileName()));
        QMessageBox::information(this, "Project Saved", "Project saved successfully!");
    } else {
        QMessageBox::critical(this, "Save Error", "Failed to save project file.");
    }
}

} // namespace beater
