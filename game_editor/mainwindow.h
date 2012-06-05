#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qpandaapplication.h>
#include <QTreeWidgetItem>
#include <dialognewscript.h>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <qangelscriptedit.h>
#include <QDir>
#include "datatree.hpp"
#include <QListWidgetItem>
#include "dialogsplashscreen.h"
#include "dialognewmap.h"
#include "wizardobject.h"
#include "dialogobject.h"
#include "dialogwaypointgenerate.h"
#include "world.h"
#include "tabscript.h"
#include "tabdialog.h"
#include "tabl18n.h"
#include "serializer.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QPandaApplication* app, QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent*);

    void LoadAllMaps(void);

    bool CloseAllScript(void);

    int currentHoveredCaseX;
    int currentHoveredCaseY;

signals:
    void Closed(void);

public slots:
    void CaseHovered(int, int);
    void UnitHovered(NodePath);
    //void WaypointHovered(NodePath);
    void PandaButtonPressed(QMouseEvent*);
    void PandaButtonRelease(QMouseEvent*);

private slots:
    void PandaInitialized(void);
    void LoadProject(void);
    void FilterInit(void);

    // MAPS
    void LoadMap(const QString&);
    void SaveMap();

    // WAYPOINTS
    void WaypointVisible(void);
    void WaypointAdd(void);
    void WaypointDelete(void);
    void WaypointConnect(void);
    void WaypointDisconnect(void);
    void WaypointUpdateX(void);
    void WaypointUpdateY(void);
    void WaypointUpdateZ(void);
    void WaypointUpdateSelX(void);
    void WaypointUpdateSelY(void);
    void WaypointUpdateSelZ(void);
    void WaypointSelDelete(void);
    void WaypointHovered(NodePath);
    void WaypointSelect(Waypoint*);
    void UpdateSelection(void);

    // OBJECTS
    void ObjectAdd(void);

    // MAPOBJECTS
    void MapObjectVisible(void);
    void MapObjectWizard(void);
    void MapObjectAdd(void);
    void MapObjectDelete(void);
    void MapObjectHovered(NodePath);
    void MapObjectNameChanged(QString);
    void MapObjectUpdateX(void);
    void MapObjectUpdateY(void);
    void MapObjectUpdateZ(void);
    void MapObjectRotationX(void);
    void MapObjectRotationY(void);
    void MapObjectRotationZ(void);
    void MapObjectScaleX(void);
    void MapObjectScaleY(void);
    void MapObjectScaleZ(void);

    // DYNAMICOBJECTS
    void DynamicObjectVisible(void);
    void DynamicObjectWizard(void);
    void DynamicObjectAdd(void);
    void DynamicObjectDelete(void);
    void DynamicObjectHovered(NodePath);
    void DynamicObjectNameChanged(QString);
    void DynamicObjectUpdateX(void);
    void DynamicObjectUpdateY(void);
    void DynamicObjectUpdateZ(void);
    void DynamicObjectRotationX(void);
    void DynamicObjectRotationY(void);
    void DynamicObjectRotationZ(void);
    void DynamicObjectScaleX(void);
    void DynamicObjectScaleY(void);
    void DynamicObjectScaleZ(void);
    void DynamicObjectSetWaypoint(void);

public:
    void DrawMap(void);

private:
    Ui::MainWindow*          ui;
    TabScript                tabScript;
    TabDialog                tabDialog;
    TabL18n                  tabL18n;

    DialogSplashscreen       splashScreen;

    QString                  levelName;
    World*                   world;
    std::list<Waypoint*>     waypointsSelection;
    Waypoint*                waypointSelected;
    Waypoint*                waypointHovered;
    float                    waypointSelX, waypointSelY, waypointSelZ;
    DialogWaypointGenerate   waypointGenerate;

    WizardObject             wizardObject;

    bool                     wizardMapObject;
    MapObject*               mapobjectSelected;
    MapObject*               mapobjectHovered;
    void                     MapObjectSelect(void);

    bool                     wizardDynObject;
    DynamicObject*           dynamicObjectSelected;
    DynamicObject*           dynamicObjectHovered;
    void                     DynamicObjectSelect(void);

    DialogNewMap             dialogNewMap;

    DialogObject             dialogObject;
    DataTree*                objectFile;
};

#endif // MAINWINDOW_H
