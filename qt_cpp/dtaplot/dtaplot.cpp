/*
*---------------------------------------------------------------------------
* Copyright (C) 2011  opendta@gmx.de
*
* Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
* der GNU General Public License, wie von der Free Software Foundation
* veroeffentlicht, weitergeben und/oder modifizieren, entweder gemaess
* Version 3 der Lizenz oder (nach Ihrer Option) jeder spaeteren Version.
*
* Die Veroeffentlichung dieses Programms erfolgt in der Hoffnung, dass es
* Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, sogar ohne die
* implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT FUER EINEN
* BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License.
*
* Sie sollten ein Exemplar der GNU General Public License zusammen mit
* diesem Programm erhalten haben. Falls nicht,
* siehe <http://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
* $Id$
*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
* DtaPlot = QwtPlot mit speziellen Anpassungen fuer die Darstellung der
*           DTA-Signale
*---------------------------------------------------------------------------*/

#include <QDateTime>
#include <QPen>
#include <QWheelEvent>
#include <QMouseEvent>

#include <qwt_scale_draw.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_legend_item.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "dtaplot.h"
#include "datetimescaleengine.h"

/*---------------------------------------------------------------------------
* DateTimeScaleDraw - Beschriftung der X-Achse im Zeitformat
*---------------------------------------------------------------------------*/
class DateTimeScaleDraw: public QwtScaleDraw
{
public:
    DateTimeScaleDraw() {}
    virtual QwtText label(double v) const { return QDateTime::fromTime_t((uint)v).toString("dd.MM. hh:mm"); }
};

/*---------------------------------------------------------------------------
* DateTimePlotPicker - Beschriftung des Cursors im Zeitformat
*---------------------------------------------------------------------------*/
class DateTimePlotZoomer: public QwtPlotZoomer
{
public:
   DateTimePlotZoomer( int xAxis, int yAxis, QwtPlotCanvas *canvas): QwtPlotZoomer( xAxis, yAxis, canvas) {}
   virtual QwtText trackerText(const QwtDoublePoint &pos) const
   {
      QString label;
      QDateTime dt = QDateTime::fromTime_t(uint(pos.x()));
      switch(rubberBand())
      {
      case HLineRubberBand:
         label = QString("%1").arg(pos.y(), 0, 'f', 1);
         break;
      case VLineRubberBand:
         label = QString("%1").arg(dt.toString("dd.MM.yy hh:mm"));
         break;
      default:
         //label = QString("%1, %2").arg(dt.toString("dd.MM.yy hh:mm")).arg(pos.y(), 0, 'f', 1);
         label = QString("%1, %2").arg(dt.toString("hh:mm:ss")).arg(pos.y(), 0, 'f', 1);
      }
      return label;
   }
};

/*---------------------------------------------------------------------------
* DtaLegend - sizeHint anpassen, damit eine einheitliche Groesse der
*             Legenden erreicht werden kann (Ausrichtung der Plots)
*---------------------------------------------------------------------------*/
class DtaLegend: public QwtLegend
{
public:
   virtual QSize sizeHint() const { return QSize(minimumWidth(),minimumHeight()); }
};

/*---------------------------------------------------------------------------
*----------------------------------------------------------------------------
* DtaPlot
*----------------------------------------------------------------------------
*---------------------------------------------------------------------------*/
DtaPlot::DtaPlot(QWidget *parent, bool xAxisVisible) :
    QwtPlot(parent)
{
   // Initialisierung
   this->hasAnalogCurves = false;
   this->allowReplot = true;

   // Drag&Drop aktivieren
   setAcceptDrops(true);

   // Eigene Klasse fuer die X-Achseneinteilung mit Zeitangaben
   QDateTime dt1 = QDateTime::currentDateTime();
   QDateTime dt2 = dt1.toUTC();
   dt1.setTimeSpec(Qt::UTC);
   int offsetUTCToLocalTime = dt1.secsTo(dt2);
   setAxisScaleEngine( QwtPlot::xBottom, new DateTimeScaleEngine(offsetUTCToLocalTime));

   // xAxis
   setAxisScaleDraw( QwtPlot::xBottom, new DateTimeScaleDraw());
   setAxisLabelRotation( QwtPlot::xBottom, -45.0);
   setAxisLabelAlignment( QwtPlot::xBottom, Qt::AlignLeft|Qt::AlignVCenter);
   // obere x-Achse ausschalten
   enableAxis( QwtPlot::xTop, false);
   // untere x-Achse einschalten, aber nicht anzeigen, wenn nicht benoetigt
   enableAxis( QwtPlot::xBottom, true);
   QwtScaleDraw *sd = axisScaleDraw(QwtPlot::xBottom);
   sd->enableComponent( QwtScaleDraw::Backbone, xAxisVisible);
   sd->enableComponent( QwtScaleDraw::Ticks, xAxisVisible);
   sd->enableComponent( QwtScaleDraw::Labels, xAxisVisible);

   // Plot fuer digitale Signale klein machen
   setAxisMaxMajor( QwtPlot::yLeft, 1);
   setAxisMaxMinor( QwtPlot::yLeft, 0);

   // Gitternetz
   QwtPlotGrid *grid = new QwtPlotGrid;
   grid->enableXMin(true);
   grid->enableYMin(true);
   grid->setMajPen(QPen(Qt::gray, 0, Qt::SolidLine));
   grid->setMinPen(QPen(Qt::lightGray, 0 , Qt::DotLine));
   grid->attach(this);

   // Scrollen per Drag&Drop
   QwtPlotPanner *panner = new QwtPlotPanner(canvas());
   panner->setMouseButton( Qt::LeftButton, Qt::ControlModifier);
   panner->setEnabled(true);

   // Zoomen mit der Maus
   zoomer = new DateTimePlotZoomer( QwtPlot::xBottom,
                                    QwtPlot::yLeft,
                                    canvas());
   zoomer->setSelectionFlags(QwtPicker::PointSelection | QwtPicker::DragSelection);
   zoomer->setTrackerMode(QwtPicker::AlwaysOn);
   zoomer->setRubberBand(QwtPicker::RectRubberBand);
   zoomer->setMousePattern( QwtEventPattern::MouseSelect1,
                            Qt::RightButton);
   zoomer->setMousePattern( QwtEventPattern::MouseSelect2,
                            Qt::RightButton,
                            Qt::ControlModifier);
   zoomer->setMousePattern( QwtEventPattern::MouseSelect3,
                            Qt::RightButton,
                            Qt::ShiftModifier);

   // Legende
   DtaLegend *legend = new DtaLegend;
   legend->setItemMode(QwtLegend::CheckableItem);
   insertLegend(legend, QwtPlot::RightLegend);

   //
   // signal/slot
   //

   // Legende mit Kruven verbinden
   connect( this, SIGNAL(legendChecked(QwtPlotItem *, bool)),
            this, SLOT(showCurve(QwtPlotItem *, bool)));
}

/*---------------------------------------------------------------------------
* Destructor
*---------------------------------------------------------------------------*/
DtaPlot::~DtaPlot()
{
   delete zoomer;
}

/*---------------------------------------------------------------------------
* showCurve - Kurven (un)sichtbar machen
*---------------------------------------------------------------------------*/
void DtaPlot::showCurve(QwtPlotItem *item, bool on)
{
   item->setVisible(on);
   QWidget *w = legend()->find(item);
   if ( w && w->inherits("QwtLegendItem") )
      ((QwtLegendItem *)w)->setChecked(on);

   if(allowReplot) replot();
}
void DtaPlot::showCurve(QString name, bool on)
{
   if( !curves.contains(name)) return;
   QwtPlotCurve *curve = curves[name];
   showCurve(curve, on);
}
bool DtaPlot::isCurveVisible(QString name)
{
   if( !curves.contains(name)) return false;
   QwtPlotCurve *curve = curves[name];
   return curve->isVisible();
}

/*---------------------------------------------------------------------------
* wheelEvent - Mausrad
*  - Zoomen
*      Strg + Mausrad         = X+Y
*      Strg + Shift + Mausrad = X
*      Strg + Alt + Mausrad   = Y
*      Strg + Meta + Mausrad  = Y
*  - Scrollen
*      Mausrad         = Y
*      Shift + Mausrad = X
*---------------------------------------------------------------------------*/
void DtaPlot::wheelEvent(QWheelEvent *event)
{
   // Delta in 1/8 Grad, ein Schritt ist normalerweise 15 Grad
   double steps = event->delta() / 8.0 / 15.0;

   // Zoom
   if( event->modifiers().testFlag(Qt::ControlModifier))
   {
      if(event->modifiers()==(Qt::ShiftModifier|Qt::ControlModifier))
         // nur X
         zoom( DtaPlot::xDirection, steps);
      else if((event->modifiers()==(Qt::AltModifier|Qt::ControlModifier)) ||
              (event->modifiers()==(Qt::MetaModifier|Qt::ControlModifier)))
         // nur Y
         zoom( DtaPlot::yDirection, steps);
      else if(event->modifiers()==Qt::ControlModifier)
         // X und Y
         zoom( DtaPlot::xyDirection, steps);
   }

   // Scrollen
   else if( event->orientation()==Qt::Vertical && event->modifiers()==Qt::NoModifier)
      // Y
      scroll( DtaPlot::yDirection, steps);
   else if( (event->orientation()==Qt::Horizontal && event->modifiers()==Qt::NoModifier) ||
            (event->orientation()==Qt::Vertical && event->modifiers()==Qt::ShiftModifier))
      // X
      scroll( DtaPlot::xDirection, -1.0*steps);
}

/*---------------------------------------------------------------------------
* scroll
*---------------------------------------------------------------------------*/
void DtaPlot::scroll(Directions dir, double steps)
{
   if( steps == 0.0) return;

   for( int axis=0; axis<QwtPlot::axisCnt; axis++)
   {
      if( !axisEnabled(axis)) continue;

      // nicht benutzte Achsen ueberspringen
      if( dir.testFlag(xDirection) && (axis==QwtPlot::yLeft || axis==QwtPlot::yRight))
         continue;
      if( dir.testFlag(yDirection) && (axis==QwtPlot::xBottom || axis==QwtPlot::xTop))
         continue;

      // neue Skalierung berechnen
      const double min = axisScaleDiv(axis)->lowerBound();
      const double max = axisScaleDiv(axis)->upperBound();
      double delta = (max-min) * steps * WHEEL_SCROLL_RATIO;

      // Skalierung setzen und neu zeichnen
      setAxisScale( axis, min+delta, max+delta);
      if( !autoReplot() && allowReplot) replot();
   }
}

/*---------------------------------------------------------------------------
* zoom
*---------------------------------------------------------------------------*/
void DtaPlot::zoom(Directions dir, double steps)
{
   if( steps == 0.0) return;

   for( int axis=0; axis<QwtPlot::axisCnt; axis++)
   {
      if( !axisEnabled(axis)) continue;

      // nicht benutzte Achsen ueberspringen
      if( dir.testFlag(xDirection) && (axis==QwtPlot::yLeft || axis==QwtPlot::yRight))
         continue;
      if( dir.testFlag(yDirection) && (axis==QwtPlot::xBottom || axis==QwtPlot::xTop))
         continue;

      // neue Skalierung berechnen
      const double range = axisScaleDiv(axis)->upperBound() - axisScaleDiv(axis)->lowerBound();
      const double center = axisScaleDiv(axis)->lowerBound() + range/2;
      double scale = 1 + WHEEL_ZOOM_RATIO * qwtAbs(steps);
      double newRange = 0.0;
      if( steps < 0)
         // reinzoomen
         newRange = range * scale;
      else
         // rauszoomen
         newRange = range / scale;

      // Skalierung setzen und neu zeichnen
      setAxisScale( axis, center-newRange/2, center+newRange/2);
      if( !autoReplot() && allowReplot) replot();
   }
}

/*---------------------------------------------------------------------------
* Zoom fit
*---------------------------------------------------------------------------*/
void DtaPlot::fit(Directions dir)
{
   if( dir.testFlag(xDirection) || dir.testFlag(xyDirection))
   {
      if(axisEnabled(QwtPlot::xBottom)) setAxisAutoScale(QwtPlot::xBottom);
      if(axisEnabled(QwtPlot::xTop)) setAxisAutoScale(QwtPlot::xTop);
   }
   else if( dir.testFlag(yDirection) || dir.testFlag(xyDirection))
   {
      if(axisEnabled(QwtPlot::yLeft)) setAxisAutoScale(QwtPlot::yLeft);
      if(axisEnabled(QwtPlot::yRight)) setAxisAutoScale(QwtPlot::yRight);
   }
   if(allowReplot) replot();
}

/*---------------------------------------------------------------------------
* Kurve hinzufuegen
*  - Name der Kruve (hiermit wird eine Kurve identifiziert)
*  - Daten
*  - Farbe
*  - analog oder digitale Kruve (Groesse des Plots, Linien oder Stufen)
*---------------------------------------------------------------------------*/
void DtaPlot::addCurve(QString name, QPolygonF *data, QColor color, bool analog, bool symbols)
{
   // Kurve erstellen
   QwtPlotCurve *curve = new QwtPlotCurve(name);
   curve->setData(*data);
   curve->setRenderHint(QwtPlotItem::RenderAntialiased);
   curve->setPen(QPen(color));
   if(symbols)
      curve->setSymbol(QwtSymbol(QwtSymbol::XCross,QBrush(),QPen(color),QSize(5,5)));
   else
      curve->setSymbol(QwtSymbol());

   // digitale Kurven als Stufen zeichnen
   if(!analog) curve->setStyle(QwtPlotCurve::Steps);

   // gibt es diese Kruve bereits -> abhaengen
   if( curves.contains(name)) curves[name]->detach();

   // neue Kurve anhaengen
   curve->attach(this);
   showCurve(curve, true); // Synchronisation mit der Legende

   // Kurve zur Liste hinzufuegen
   curves[name] = curve;

   // Achseneinteilung festlegen
   if( !hasAnalogCurves && analog)
   {
      hasAnalogCurves = true;
      setAxisMaxMajor( QwtPlot::yLeft, 3);
      setAxisMaxMinor( QwtPlot::yLeft, 5);
   }

   // y-Achse neu skalieren
   if(axisEnabled(QwtPlot::yLeft))
      setAxisAutoScale(QwtPlot::yLeft);
   if(axisEnabled(QwtPlot::yRight))
      setAxisAutoScale(QwtPlot::yRight);
   setAxisAutoScale(QwtPlot::xBottom);

   // neu Zeichenen
   if(allowReplot) replot();

   // Zoom-Stufe merken
   zoomer->setZoomBase();
}

/*---------------------------------------------------------------------------
* Kurve entfernen
*---------------------------------------------------------------------------*/
void DtaPlot::removeCurve(QString name)
{
   if( !curves.contains(name)) return;
   curves[name]->detach();
   curves.remove(name);
   if(allowReplot) replot();
}

/*---------------------------------------------------------------------------
* Liste mit Kurvennamen
*---------------------------------------------------------------------------*/
QStringList DtaPlot::curveNames()
{
   return curves.keys();
}

/*---------------------------------------------------------------------------
* neue Daten fuer eine Kurve
*---------------------------------------------------------------------------*/
void DtaPlot::updateCurveData(QString name, QPolygonF *data)
{
   if( !curves.contains(name)) return;
   QwtPlotCurve *curve = curves[name];
   curve->setData(*data);
}

/*---------------------------------------------------------------------------
* Linienstaerke und Farbe einer Kurve
*---------------------------------------------------------------------------*/
QPen DtaPlot::curvePen(QString name)
{
   if( !curves.contains(name)) return QPen();
   QwtPlotCurve *curve = curves[name];
   return curve->pen();
}
void DtaPlot::setCurvePen(QString name, QPen pen)
{
   if( !curves.contains(name)) return;
   QwtPlotCurve *curve = curves[name];
   curve->setPen(pen);
   if(allowReplot) replot();
}

/*---------------------------------------------------------------------------
* Symbole der Datenpunkte an und abschalten
*---------------------------------------------------------------------------*/
void DtaPlot::setSymbols(bool on)
{
   QList<QwtPlotCurve*> cs = curves.values();
   for( int i=0; i<cs.size(); ++i)
   {
      QwtPlotCurve *c = cs.at(i);
      if(on)
         c->setSymbol(QwtSymbol(QwtSymbol::XCross,QBrush(),c->pen(),QSize(5,5)));
      else
         c->setSymbol(QwtSymbol());
   }
   this->replot();
}

/*---------------------------------------------------------------------------
* neu Zeichen erlauben/verhindern
*  - wenn viele Daten das Plot geaendert werden, dann wird das neu Zeichen
*    hiermit verhindert, um schneller zu sein
*---------------------------------------------------------------------------*/
void DtaPlot::setAllowReplot(bool on)
{
   this->allowReplot = on;
}
