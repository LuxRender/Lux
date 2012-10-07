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

#include <QtGui/QMainWindow>
#include <QtGui/QProgressBar>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QCheckBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
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
#include <QCursor>
#include <QAbstractListModel>
#include <QList>
#include <QCompleter>
#include <QVariant>
#include <QtGui/QTabBar>
#include <QtGui/QProgressDialog>
#include <QtGui/QStandardItemModel>
#include <QThread>
#include <QTableWidget>
#include <QMap>

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
#include "advancedinfowidget.hxx"

#define FLOAT_SLIDER_RES 512.f

#define CHIU_RADIUS_MIN 1
#define CHIU_RADIUS_MAX 9

#define LG_SCALE_LOG_MIN -4.f
#define LG_SCALE_LOG_MAX 4.f
#define LG_TEMPERATURE_MIN 1000.f
#define LG_TEMPERATURE_MAX 10000.f


enum LuxGuiRenderState
{
	WAITING,
	PARSING,
	RENDERING,
	STOPPING,
	STOPPED,
	ENDING,
	ENDED,
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

class BatchEvent: public QEvent {
public:
	BatchEvent(const QString &currentFile, const int &numCompleted, const int &total);

	const QString& getCurrentFile() const { return _currentFile; }
	const int& getNumCompleted() const { return _numCompleted; }
	const int& getTotal() const { return _total; }
	bool isFinished() const { return (_numCompleted == _total); }

private:
	QString _currentFile;
	int _numCompleted, _total;
};

class NetworkUpdateTreeEvent: public QEvent {
public:
	NetworkUpdateTreeEvent();
};

template <class T>
class QMRUListModel : public QAbstractListModel {
public:
	QMRUListModel() : QAbstractListModel(), maxCount(0) { }
	QMRUListModel(int count, QObject *parent = 0) : QAbstractListModel(parent), maxCount(count)  { }
	QMRUListModel(const QMRUListModel<T> &other) 
		: maxCount(other.maxCount), mruList(other.mruList), QAbstractListModel(other.parent()) {
	}

	int count() const {
		return maxCount;
	}

	void setCount(int count) {
		maxCount = count;
		prune(maxCount);
	}

	void add(const T& value) {
		int i = mruList.indexOf(value);

		if (i == 0)
			return;

		if (i > 0) {
			beginRemoveRows(QModelIndex(), i, i);
			mruList.erase(mruList.begin() + i);
			endRemoveRows();			
		}

		beginInsertRows(QModelIndex(), 0, 0);
		mruList.insert(0, value);
		endInsertRows();

		prune(maxCount);
	}

	QList<T> list() const {
		return mruList;
	}

	void setList(const QList<T>& list) {
		prune(0);
		beginInsertRows(QModelIndex(), 0, 0);
		mruList = list.mid(0, maxCount);
		endInsertRows();
	}

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const {
		return mruList.length();
	}

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const {		
		if (!index.isValid())
			return QVariant();
		if (index.row() >= mruList.length() || index.column() != 0)
			return QVariant();

		if (role == Qt::DisplayRole)
			return QVariant(mruList[index.row()]);

		return QVariant();
	}

private:
	void prune(int count) {
		if (mruList.length() > count) {
			beginRemoveRows(QModelIndex(), count, mruList.length());
			mruList.erase(mruList.begin() + count, mruList.begin() + mruList.length());
			endRemoveRows();
		}
	}

	int maxCount;
	QList<T> mruList;
};

typedef QMRUListModel<QString> QStringMRUListModel;

void updateWidgetValue(QSlider *slider, int value);
void updateWidgetValue(QDoubleSpinBox *spinbox, double value);
void updateWidgetValue(QSpinBox *spinbox, int value);
void updateWidgetValue(QCheckBox *checkbox, bool checked);

void updateParam(luxComponent comp, luxComponentParameters param, double value, int index = 0);
void updateParam(luxComponent comp, luxComponentParameters param, const char* value, int index = 0);
double retrieveParam (bool useDefault, luxComponent comp, luxComponentParameters param, int index = 0);

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0, bool copylog2console = FALSE);
	~MainWindow();

	void SetRenderThreads(int num);
	void updateStatistics();
	void showRenderresolution();
	void showZoomfactor();
	void renderScenefile(const QString& sceneFilename, const QString& flmFilename = "");
	void renderNewScenefile(const QString& sceneFilename, const QString& flmFilename = "");
	void openQueueFile(const QString& fileName);
	void changeRenderState (LuxGuiRenderState state);
	void endRenderingSession(bool abort = true);
	
	void updateTonemapWidgetValues ();

	void resetToneMappingFromFilm (bool useDefaults);

	void UpdateNetworkTree();

	void ShowTabLogIcon( int tabID , const QIcon & icon);
	
	bool m_auto_tonemap;

	void loadFile(const QString &fileName);
	bool addFileToRenderQueue(const QString &sceneFileName);
	bool RenderNextFileInQueue();

	void AddNetworkSlaves(const QVector<QString> &slaves);
	void RemoveNetworkSlaves(const QVector<QString> &slaves);

	void setServerUpdateInterval(int interval);

protected:
	
	int getTabIndex(int tabID);

	bool saveCurrentImageHDR(const QString &outFile);
	bool saveAllLightGroups(const QString &outFilename, const bool &asHDR);
	void setCurrentFile(const QString& filename);
	void updateRecentFileActions();
	void createActions();
	void closeEvent(QCloseEvent *event);

private:
	
	Ui::MainWindow *thorizontalLayout_5, *thorizontalLayout_log;
	
	QLabel *resinfoLabel;
	QLabel *zoominfoLabel; 
	
	Ui::MainWindow *ui;

	QLabel *activityLabel;
	QLabel *statusLabel;
	QLabel *statsLabel;
	QLabel *activityMessage;
	QLabel *statusMessage;
	QProgressBar *statusProgress;
	QSpacerItem *spacer;

	QFrame *statsBox;
	QHBoxLayout *statsBoxLayout;

	RenderView *renderView;
	QString m_CurrentFile;
	QString m_CurrentFileBaseName;

	enum { NumPanes = 6 };

	PaneWidget *panes[NumPanes];

	enum { NumAdvPanes = 1 };

	PaneWidget *advpanes[NumAdvPanes];

	ToneMapWidget *tonemapwidget;
	LensEffectsWidget *lenseffectswidget;
	ColorSpaceWidget *colorspacewidget;
	GammaWidget *gammawidget;
	NoiseReductionWidget *noisereductionwidget;
	HistogramWidget *histogramwidget;
	AdvancedInfoWidget *advancedinfowidget;

	QVector<PaneWidget*> m_LightGroupPanes;

	QProgressDialog *batchProgress;

	int m_numThreads;
	bool m_copyLog2Console;

	QRegExp reUpdateInterval;

	LuxGuiRenderState m_guiRenderState;
	
	QTimer *m_renderTimer, *m_statsTimer, *m_loadTimer, *m_saveTimer, *m_netTimer, *m_blinkTimer;
	
	class FlmLoadThread : public QThread {
	public:
		FlmLoadThread(MainWindow *mw, const QString &fn) :
			mainWindow(mw), filename(fn) { }
		virtual ~FlmLoadThread() { }
		virtual void run();
		MainWindow *mainWindow;
		QString filename;
	} *m_flmloadThread;
	class FlmSaveThread : public QThread {
	public:
		FlmSaveThread(MainWindow *mw, const QString &fn) :
			mainWindow(mw), filename(fn) { }
		virtual ~FlmSaveThread() { }
		virtual void run();
		MainWindow *mainWindow;
		QString filename;
	} *m_flmsaveThread;
	class BatchProcessThread : public QThread {
	public:
		BatchProcessThread(MainWindow *mw, const QString &id,
			const QString &od, const QString &oe, bool alg, bool ah) :
			mainWindow(mw), inDir(id), outDir(od), outExtension(oe),
			allLightGroups(alg), asHDR(ah) { }
		virtual ~BatchProcessThread() { }
		virtual void run();
		MainWindow *mainWindow;
		QString inDir, outDir, outExtension;
		bool allLightGroups, asHDR;
	} *m_batchProcessThread;
	class UpdateThread : public QThread {
	public:
		UpdateThread(MainWindow *mw) : mainWindow(mw) { }
		virtual ~UpdateThread() { }
		virtual void run();
		MainWindow *mainWindow;
	} *m_updateThread;
	class EngineThread : public QThread {
	public:
		EngineThread(MainWindow *mw, const QString &fn) :
			mainWindow(mw), filename(fn) { }
		virtual ~EngineThread() { }
		virtual void run();
		MainWindow *mainWindow;
		QString filename;
	} *m_engineThread;
	enum ChangeSlavesAction { AddSlaves, RemoveSlaves };
	class NetworkAddRemoveSlavesThread : public QThread {
	public:
		NetworkAddRemoveSlavesThread(MainWindow *mw,
			const QVector<QString> &s, ChangeSlavesAction a) :
			mainWindow(mw), slaves(s), action(a) { }
		virtual ~NetworkAddRemoveSlavesThread() { }
		virtual void run();
		MainWindow *mainWindow;
		QVector<QString> slaves;
		ChangeSlavesAction action;
	} *m_networkAddRemoveSlavesThread;

	bool openExrHalfFloats, openExrDepthBuffer;
	int openExrCompressionType;

	bool m_bTonemapPending;

	// Directory Handling
	enum { MaxRecentFiles = 5 };
	QString m_lastOpendir;
	QList<QFileInfo> m_recentFiles;
	QAction *m_recentFileActions[MaxRecentFiles];

	static void LuxGuiErrorHandler(int code, int severity, const char *msg);
	static QWidget *instance;
	

	enum { MaxRecentServers = 20 };
	QStringMRUListModel *m_recentServersModel;

	void addRemoveSlaves(QVector<QString> slaves, ChangeSlavesAction action);

	QMap<QString, int> networkSlaves;

	bool event (QEvent * event);

	void logEvent(LuxLogEvent *event);
	
	bool canStopRendering ();
    
	bool blink;
	int viewportw, viewporth;
	float zoomfactor;
	float factor;
    
	void UpdateLightGroupWidgetValues();
	void ResetLightGroups(void);
	void ResetLightGroupsFromFilm(bool useDefaults);

	void ReadSettings();
	void WriteSettings();

	// The Data Model for the Render Queue
	QStandardItemModel renderQueueData;
	bool IsFileInQueue(const QString &filename);
	bool IsFileQueued();
	bool RenderNextFileInQueue(int idx);
	void ClearRenderingQueue();
	QStringList renderQueueList;

public slots:

	void applyTonemapping (bool withlayercomputation = false);
	void resetToneMapping ();
	void indicateActivity (bool active = true);
	void setVerbosity(int choice);
	void userSamplingAddPen();
	void userSamplingSubPen();
	void userSamplingPenSize(int size);
	void userSamplingApply();
	void userSamplingUndo();
	void userSamplingReset();

private slots:

	void exitAppSave ();
	void exitApp ();
	void openFile ();
	void openRecentFile();
	void resumeFLM ();
	void loadFLM (QString flmFileName = QString());
	void saveFLM ();
	void resumeRender ();
	void pauseRender ();
	void stopRender ();
	void endRender ();
	void outputTonemapped ();
	void outputHDR ();
	void outputBufferGroupsTonemapped ();
	void outputBufferGroupsHDR ();
	void batchProcess ();
	void copyLog ();
	void clearLog ();
	void tabChanged (int);
	void outputTabChanged (int);
	void viewportChanged ();
	void fullScreen ();
	void normalScreen ();
	void showAlphaChanged (bool);
	void overlayStatsChanged (bool);
	void showUserSamplingMapChanged (bool);
	void aboutDialog ();
	void openDocumentation ();
	void openForums ();
	void openGallery ();
	void openBugTracker ();
	
	void renderTimeout ();
	void statsTimeout ();
	void loadTimeout ();
	void saveTimeout ();
	void netTimeout ();
	void blinkTrigger (bool active = true);

	void ThreadChanged(int value);

	// Tonemapping slots
	void toneMapParamsChanged();
	void forceToneMapUpdate();

	void autoEnabledChanged (int value);
	void overrideDisplayIntervalChanged(int value);

	void addServer();
	void removeServer();
	void resetServer();
	void updateIntervalChanged(int value);
	void updateIntervalChanged();
	void networknodeSelectionChanged();

	void addQueueHeaders();
	void addQueueFiles();  
	void removeQueueFiles();
	void overrideHaltSppChanged(int value);
	void overrideHaltTimeChanged(int value);
	void loopQueueChanged(int state);
	void overrideWriteFlmChanged(bool checked);

	void copyToClipboard ();

	void SavePanelSettings();
	void LoadPanelSettings();

	void setLightGroupSolo( int index );

	void ShowSidePanel(bool checked);
};

#endif // MAINWINDOW_H
