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
* (sinnvolle) Achseneinteilung fuer eine Zeitachse berechnen
*---------------------------------------------------------------------------*/

#ifndef DATETIMESCALEENGINE_H
#define DATETIMESCALEENGINE_H

#include <qwt_scale_engine.h>

/*---------------------------------------------------------------------------
* siehe QwtLinearScaleEngine
*---------------------------------------------------------------------------*/
class DateTimeScaleEngine : public QwtLinearScaleEngine
{
public:
   DateTimeScaleEngine(int offsetUTCToLocalTime);
   virtual void autoScale(int maxSteps,
                          double &x1,
                          double &x2,
                          double &stepSize) const;

   virtual QwtScaleDiv divideScale(double x1,
                                   double x2,
                                   int numMajorSteps,
                                   int numMinorSteps,
                                   double stepSize = 0.0) const;

protected:
   double divideInterval(double interval, int numSteps) const;

private:
   void buildTicks( const QwtDoubleInterval &,
                    double stepSize,
                    int maxMinSteps,
                    QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

   void buildMinorTicks( const QwtValueList& majorTicks,
                         int maxMinMark,
                         double step,
                         QwtValueList &, QwtValueList &) const;

   QwtValueList buildMajorTicks( const QwtDoubleInterval &interval,
                                 double stepSize) const;
   int offsetUTCToLocalTime;
};

#endif // DATETIMESCALEENGINE_H
