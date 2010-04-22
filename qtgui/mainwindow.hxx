/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <boost/thread.hpp>

#include <QtGui/QMainWindow>
#include <QtGui/QProgressDialog>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QCheckBox>
#include <QSpacerItem>
#include <QTimer>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QMatrix>
#include <QPoint>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QVector>
#include <QSettings>
#include <QTextCursor>

#include "api.h"
#include "renderview.hxx"
#include "panewidget.hxx"
#include "lightgroupwidget.hxx"
#include "tonemapwidget.hxx"
#include "lenseffectswidget.hxx"
#include "colorspacewidget.hxx"
#include "gammawidget.hxx"
#include "noisereductionwidget.hxx"
#include "histogramwidget.hxx"

#define FLOAT_SLIDER_RES 512.f

#define CHIU_RADIUS_MIN 1
#define CHIU_RADIUS_MAX 9

#define LG_SCALE_LOG_MIN -4.f
#define LG_SCALE_LOG_MAX 4.f
#define LG_TEMPERATURE_MIN 1000.f
#define LG_TEMPERATURE_MAX 10000.f

template<class T> inline T Clamp(T val, T low, T high) {
	return val > low ? (val < high ? val : high) : low;
}

enum LuxGuiRenderState
{
	WAITING,
	PARSING,
	RENDERING,
	STOPPING,
	STOPPED,
	PAUSED,
	FINISHED,
	TONEMAPPING // tonemapping an FLM file (not really a 'render' state)
};

namespace Ui
{
	class MainWindow;
}

class LuxLogEvent: public QEvent {
public:
	LuxLogEvent(int code,int severity,QString message);
 
	QString getMessage() { return _message; }
	int getSeverity() { return _severity; }
	int getCode() { return _code; }

private:
	QString _message;
	int _severity;
	int _code;
};

class luxTreeData
{
public:

	QString m_SlaveName;
	QString m_SlaveFile;
	QString m_SlavePort;
	QString m_SlaveID;

	unsigned int m_secsSinceLastContact;
	double m_numberOfSamplesReceived;
};

void updateWidgetValue(QSlider *slider, int value);
void updateWidgetValue(QDoubleSpinBox *spinbox, double value);
void updateWidgetValue(QSpinBox *spinbox, int value);

int ValueToLogSliderVal(float value, const float logLowerBound, const float logUpperBound);
float LogSliderValToValue(int sliderval, const float logLowerBound, const float logUpperBound);

void updateParam(luxComponent comp, luxComponentParameters param, double value, int index = 0);
void updateParam(luxComponent comp, luxComponentParameters param, const char* value, int index = 0);
double retrieveParam (bool useDefault, luxComponent comp, luxComponentParameters param, int index = 0);

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0, bool opengl = false, bool copylog2console = FALSE);
	~MainWindow();

	void SetRenderThreads(int num);
	void updateStatistics();
	void renderScenefile(const QString& sceneFilename, const QString& flmFilename);
	void renderScenefile(const QString& filename);
	void changeRenderState (LuxGuiRenderState state);
	void endRenderingSession ();
	
	void updateTonemapWidgetValues ();

	void resetToneMappingFromFilm (bool useDefaults);

	void UpdateNetworkTree();

    void ShowTabLogIcon( int index , const QIcon & icon);
    void ShowTabLogText( int index , const QString & label);
	bool m_auto_tonemap;

protected:
	
	void setCurrentFile(const QString& filename);
	void updateRecentFileActions();
	void createActions();
	
private:
	Ui::MainWindow *ui;

	QLabel *statusMessage;
	QLabel *statsMessage;
	QSpacerItem *spacer;
	
	RenderView *renderView;
	QString m_CurrentFile;

	enum { NumPanes = 6 };

	PaneWidget *panes[NumPanes];

	ToneMapWidget *tonemapwidget;
	LensEffectsWidget *lenseffectswidget;
	ColorSpaceWidget *colorspacewidget;
	GammaWidget *gammawidget;
	NoiseReductionWidget *noisereductionwidget;
	HistogramWidget *histogramwidget;

	QVector<PaneWidget*> m_LightGroupPanes;

	int m_numThreads;
	bool m_copyLog2Console;
	bool m_showParseWarningDialog;
	bool m_showParseErrorDialog;
	bool m_showWarningDialog;
	double m_samplesSec;
	
	LuxGuiRenderState m_guiRenderState;
	
	QProgressDialog *m_progDialog;
	QTimer *m_renderTimer, *m_statsTimer, *m_loadTimer, *m_saveTimer, *m_netTimer, *m_blinkTimer;
	
	boost::thread *m_engineThread, *m_updateThread, *m_flmloadThread, *m_flmsaveThread;

	// Directory Handling
	enum { MaxRecentFiles = 5 };
	QString m_lastOpendir;
	QStringList m_recentFiles;
	QAction *m_recentFileActions[MaxRecentFiles];

	bool m_opengl;
	
	static void LuxGuiErrorHandler(int code, int severity, const char *msg);
	
	void engineThread(QString filename);
	void updateThread();
	void flmLoadThread(QString filename);
	void flmSaveThread(QString filename);
	
	bool event (QEvent * event);

	void logEvent(LuxLogEvent *event);

	bool canStopRendering ();
    
    bool blink;
    
	void UpdateLightGroupWidgetValues();
	void ResetLightGroups(void);
	void ResetLightGroupsFromFilm(bool useDefaults);

	void ReadSettings();
	void WriteSettings();

public slots:

	void applyTonemapping (bool withlayercomputation = false);
	void resetToneMapping ();

private slots:

	void exitApp ();
	void openFile ();
	void openRecentFile();
	void resumeFLM ();
	void loadFLM ();
	void saveFLM ();
	void resumeRender ();
	void pauseRender ();
	void stopRender ();
	void copyLog ();
	void clearLog ();
	void fullScreen ();
    void normalScreen ();
	void aboutDialog ();
	void openDocumentation ();
	void openForums ();
	
	void renderTimeout ();
	void statsTimeout ();
	void loadTimeout ();
	void saveTimeout ();
	void netTimeout ();
    void blinkTimeout ();

	void addThread ();
	void removeThread ();

	// Tonemapping slots
	void toneMapParamsChanged();
	void forceToneMapUpdate();

	void autoEnabledChanged (int value);



	void addServer();
	void removeServer();
	void updateIntervalChanged(int value);
	void networknodeSelectionChanged();
	
	void copyToClipboard ();
};

#endif // MAINWINDOW_H
