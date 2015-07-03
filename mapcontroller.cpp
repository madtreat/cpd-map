/* 
 * File:   mapcontroller.cpp
 * Author: Madison Treat <madison.treat@tamu.edu>
 * 
 * Created on June 11, 2015, 12:33 PM
 */

#include "mapcontroller.h"

#include <QDebug>

#include "core/aircraft.h"
#include "core/hddsettings.h"
#include "qt-google-maps/mapsettings.h"
#include "mapwidget.h"

MapController::MapController(HDDSettings* _hddSettings, ACMap* _acMap, QObject* _parent)
: QObject(_parent),
  hddSettings(_hddSettings),
  acMap(_acMap)
{
   orientation = TRACK_UP;
   settings = new MapSettings("maps.ini", this);
   mapWidget = new MapWidget(hddSettings, settings, acMap);
   
   qDebug() << "Can enable maps?" << settings->canEnableMaps();
}

//MapController::MapController(const MapController& orig)
//{
//}

MapController::~MapController()
{
}

void MapController::setZoom(int level)
{
   if (level >= ZOOM_MAX) {
      emit zoomMaxReached(true);
      return;
   }
   else if (level <= ZOOM_MIN) {
      emit zoomMinReached(true);
      return;
   }
   settings->setZoom(level);
   mapWidget->setZoom(level);
   emit zoomEither(true);
}

void MapController::increaseZoom()
{
   int newLevel = settings->zoom() + 1;
   setZoom(newLevel);
}

void MapController::decreaseZoom()
{
   int newLevel = settings->zoom() - 1;
   setZoom(newLevel);
}

void MapController::panToLocation(float lat, float lon)
{
   mapWidget->panToLocation(lat, lon);
}

void MapController::setOrientation(MapOrientation mo)
{
   hddSettings->setMapOrientation(mo);
   mapWidget->setOrientation(mo);
}

