/* 
 * File:   mapoverlay.cpp
 * Author: Madison Treat <madison.treat@tamu.edu>
 * 
 * Created on June 15, 2015, 2:36 PM
 */

#include "mapoverlay.h"

#include <algorithm>
#include <math.h>
#include <QPainter>
#include <QSvgRenderer>

#include "mapsettings.h"


using namespace std;

MapOverlay::MapOverlay(
  MapSettings* _mapSettings,
  ACMap* _acMap,
  QWidget* _parent
)
: QWidget(_parent),
mapSettings(_mapSettings),
acMap(_acMap),
heading(0.0),
showTraffic(false),
satMapActive(false) {
  initRangeTicks();

  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_TransparentForMouseEvents);

  overlayColor = Qt::yellow;
  aircraftIcon = QImage(":/ac/icons/airplane.png");

  setAircraftType(AC_GA);
  setMinimumSize(QSize(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT));
}

//MapOverlay::MapOverlay(const MapOverlay& orig) {
//}

MapOverlay::~MapOverlay() {
  delete ownship;
}


QSize MapOverlay::sizeHint() const {
  return QSize(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT);
}


void MapOverlay::resize(int w, int h) {
  resize(QSize(w, h));
}

bool MapOverlay::northUp() const {
  return mapSettings->mapOrientation() == NORTH_UP;
}


/*
 * This widget should always be a square.
 */
void MapOverlay::resize(const QSize& size) {
  QWidget::resize(size);

  // Update the range ticks
  initRangeTicks(min(size.width(), size.height()));
}


void MapOverlay::setZoom(int level) {
  // TODO: set circle radius in proportion to zoom level
}


void MapOverlay::panToLocation(float lat, float lon) {
  // TODO: change circle position or something
}


void MapOverlay::displayTraffic(bool show) {
  showTraffic = show;
}


/*
 * Returns the draw angle in radians, which just wraps around the circle.
 * The angle must be given in degrees.
 */
double MapOverlay::getDrawAngleRad(double deg) {
  // Convert angle to radians, also add 90 to orient it correctly
  // (0 deg is up, not right, as drawn by default)
  double drawAng = deg - 90;
  if (drawAng < 0) {
     drawAng = deg + 270;
  }
  double rad = drawAng * 3.14159/180;
  return rad;
}


/*
 * Returns a line object originating at the center of the overlay window, with
 * the following properties:
 *    deg  = the angle in degrees
 *    from = inside radius to draw in pixels
 *    to   = outside outside to draw in pixels
 *    cx   = center point x in pixels; default = 0
 *    cy   = center point y in pixels; default = 0
 * 
 * Basically, the line is centered at (cx, cy), and goes out at an angle deg, 
 * and is only drawn between the radius' from and to.
 */
QLineF MapOverlay::getLine(double deg, int from, int to, int cx, int cy) {
  double rad = getDrawAngleRad(deg);

  // First point is on the radius circle
  double x1 = cx + (to * cos(rad));
  double y1 = cy + (to * sin(rad));

  // Second point is the aircraft position (0)
  double x2 = cx + (from * cos(rad));
  double y2 = cy + (from * sin(rad));

  return QLineF(x1, y1, x2, y2);
}


void MapOverlay::initRangeTicks(int diameter) {
  // Ensure the vector is empty
  if (rangeCircleTicks.size() > 0) {
    rangeCircleTicks.clear();
  }

  int mapRad = diameter / 2;
  int cx = mapRad;
  int cy = mapRad;
  int cradius = cx - COMPASS_PADDING;

  //// NORTH UP
  // Generate a line/tick for each degree on the compass
  for (int i = 0; i < 360; i++) {
    int len = 5;
    if      (i % 10 == 0) { len = 12; }
    else if (i %  5 == 0) { len = 8;  }

    QLineF line = getLine(i, cradius - len, cradius);
    rangeCircleTicks.append(line);
  }
}


void MapOverlay::paintEvent(QPaintEvent*) {
  QPainter p(this);
  int centerX = width()/2;
  int centerY = height()/2;

  // Draw the center airplane icon, using the current orientation setting
  if (northUp()) {
    p.translate(centerX, centerY);
    p.rotate(heading);
    int x = 0 - (ownshipIcon.width()/2);
    int y = 0 - (ownshipIcon.height()/2);
    p.drawImage(QPoint(x,y), ownshipIcon);
    p.resetTransform();
  }
  else {
    // Draw the plane facing up
    int x = centerX - (ownshipIcon.width()/2);
    int y = centerY - (ownshipIcon.height()/2);
    p.drawImage(QPoint(x,y), ownshipIcon);
  }

  // Draw the range circle
  drawRangeCircle(p);
}


void MapOverlay::drawRangeCircle(QPainter& p) {
  QPen origPen = p.pen();
  int cx = width()/2;  // center x
  int cy = height()/2; // center y
  int cradius = min(cx, cy) - COMPASS_PADDING; // circle radius - give some padding for drawing outside it

  QPen pen(Qt::SolidLine);
  pen.setColor(overlayColor);
  pen.setWidth(2);
  p.setPen(pen);

  // Draw the range circle
  // circles are 0 -> (5760=16*360), in 1/16th deg (integer) increments
  QRect mapr = rect();
  QRect rangeCircle = mapr;
  int hAdjust = cx - cradius;//COMPASS_PADDING;
  int vAdjust = cy - cradius;//COMPASS_PADDING;
  rangeCircle.adjust(hAdjust, vAdjust, -hAdjust, -vAdjust);
  p.drawArc(rangeCircle, 0, 5760);

  // Draw the tick marks, rotating if TRACK_UP
  p.translate(cx, cy);
  if (!northUp()) {
    p.rotate(-heading);
  }
  pen.setWidth(1);
  p.setPen(pen);
  p.drawLines(rangeCircleTicks);

  // Draw the heading line
  QLineF headingLine = getLine(heading, 20, cradius);
  //p.drawLine(headingLine);

  // Do some fancy un-rotating and angle - heading for proper textual display
  if (!northUp()) {
    // if TRACK_UP, reset the rotation for the heading line
    p.rotate(heading);
  }

  // Draw the heading number
  QString headingText = QString("%1").arg(heading, 1, 'f', 1);
  double hrad = getDrawAngleRad(northUp() ? heading : 0); // radians, not radius
  int hradius = cradius - 60;
  int hwidth = 8 * headingText.length();
  double hx = 0 + (hradius * cos(hrad)) - hwidth/2; // shift left width/2 pixels
  double hy = 0 + (hradius * sin(hrad)) + 6; // lower 6 pixels
  //p.drawText(hx, hy, headingText);

  // Draw the tick mark text
  for (int i = 0; i < 360; i += 30) {
    // Convert angle to radians
    double ang = northUp() ? i : i - heading;
    double rad = getDrawAngleRad(ang);

    // First point is on the radius circle
    int radius = cradius - 25;
    int width = 8*QString::number(i).length();
    // center is (0, 0) since we are already translated
    double x1 = 0 + (radius * cos(rad)) - width/2; // shift left width/2 pixels
    double y1 = 0 + (radius * sin(rad)) + 6; // lower 6 pixels

    p.drawText(x1, y1, QString::number(i));
  }

  // Draw traffic markers on the compass (skip ac 0 = this one)
  if (showTraffic) {
    for (int i = 1; i < acMap->size(); i++) {
      Aircraft* ac = acMap->value(i);
      if (!ac) continue;
      double ber = ac->getBer();
      double ang = northUp() ? ber : ber - heading;
      double rad = getDrawAngleRad(ang);

      // The text to display for this AC
      QString text = QString("%1").arg(i);

      // Draw a line on the compass to mark this ac
      int length = 5;
      int radius = cradius + length;
      QLineF acLine = getLine(ang, cradius, radius);
      p.drawLine(acLine);

      // Draw the text on the compass
      radius += 2*length;
      int width = 8*text.length();
      double x1 = 0 + (radius * cos(rad)) - width/2; // shift left width/2 pixels
      double y1 = 0 + (radius * sin(rad)) + 6; // lower 6 pixels

      p.drawText(x1, y1, text);
    }
  }

  // Reset for next drawing
  p.setPen(origPen);
  p.resetTransform();
}

void MapOverlay::satButtonClicked(bool checked) {
  satMapActive = checked;
  overlayColor = checked ? Qt::yellow : Qt::darkMagenta;
}

void MapOverlay::setAircraftType(AircraftType _type) {
  type = _type;

  ownshipPath = ":/ac/sprites/ownship/";
  QSize iconSize(32, 32);
  if (_type & AC_BOMBER) {
    ownshipPath += "bomber";
    iconSize = QSize(20, 32);
  }
  else if (_type & (AC_TANKER | AC_AIRLINER)) {
    ownshipPath += "civil";
    iconSize = QSize(32, 24);
  }
  else if (_type & AC_FIGHTER) {
    ownshipPath += "fighter";
    iconSize = QSize(32, 20);
  }
  else if (_type & AC_GA) {
    ownshipPath += "ga";
    iconSize = QSize(32, 22);
  }
  else if (_type & AC_GLIDER) {
    ownshipPath += "glider";
    iconSize = QSize(16, 36);
  }
  else if (_type & AC_HELICOPTER) {
    ownshipPath += "helo";
    iconSize = QSize(36, 30);
  }
  else if (_type & AC_CARGO) {
    ownshipPath += "transport";
    iconSize = QSize(32, 32);
  }
  else if (_type & AC_UAV) {
    ownshipPath += "uas";
    iconSize = QSize(16, 32);
  }
  ownshipPath += ".svg";

  ownship = new QGraphicsSvgItem(ownshipPath);

  ownshipIcon = QImage(iconSize, QImage::Format_ARGB32);
  ownshipIcon.fill(0x00aa0033);

  QPainter imageP(&ownshipIcon);
  QSvgRenderer renderer(ownshipPath);
  renderer.render(&imageP);
//  ownshipIcon.fill(overlayColor);
}
