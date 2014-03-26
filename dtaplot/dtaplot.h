/*
*---------------------------------------------------------------------------
* Copyright (C) 2014  opendta@gmx.de
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
#ifndef DTAPLOT_H
#define DTAPLOT_H

#include <qwt_plot.h>
#include <QHash>

#define WHEEL_SCROLL_RATIO 0.3 // Seiten, die pro Mausrad-Schitt gescrollt werden
#define WHEEL_ZOOM_RATIO 0.2   // Verhaeltnis fuer das Zoomen mit dem Mausrad

class QwtPlotMagnifier;
class QwtPlotCurve;
class DateTimePlotZoomer;

/*---------------------------------------------------------------------------
* DtaPlot
*---------------------------------------------------------------------------*/
class DtaPlot : public QwtPlot
{
    Q_OBJECT
public:
   // Flags zum bestimmen der Scroll/Zoom-Achsen
   enum Direction
   {
      xDirection = 0,
      yDirection = 1,
      xyDirection = 2
                 };
   Q_DECLARE_FLAGS( Directions, Direction)

   explicit DtaPlot(QWidget *parent, bool xAxisVisible=true);
   ~DtaPlot();

   // Kurven
   //  analoge/digitale Kurven unterscheiden sich in der Achseneinteilung
   void addCurve(QString name, QString dispName, QPolygonF *data, QColor color, bool analog=false, bool symbols=false);
   void removeCurve(QString name);
   QStringList curveNames();  // Namen aller Kurven
   bool isCurveVisible(QString name); // Kurve sichtbar
   void updateCurveData(QString name, QPolygonF * data); // neue Daten

   // Farbe und Linienstaerke
   void setCurvePen(QString name, QPen pen);
   QPen curvePen(QString name);

   // Datenpunkte anzeigen
   void setSymbols(bool on);

   void fit(Directions dir); // zoom fit

   // neuzeichen erlauben/verhindern
   void setAllowReplot(bool on);

signals:
   // Skalierung einer Achse hat sich geaendert (zum Synchronisieren von mehreren Plots)
   void axisScaleChanged( int axisId, double min, double max);

public slots:
   // Kurve (un)sichtbar machen
   void showCurve(QwtPlotItem *, bool on);
   void showCurve(QString name, bool on);
   void legendChecked(const QVariant &itemInfo, bool on);

private:
   QHash<QString,QwtPlotCurve*> curves;
   DateTimePlotZoomer *zoomer;
   Directions trackDirection;
   bool hasAnalogCurves;
   bool allowReplot;

   void scroll( Directions dir, double steps);
   void zoom( Directions dir, double steps);
   void wheelEvent(QWheelEvent *event);
};

#endif // DTAPLOT_H
