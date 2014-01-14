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
* (sinnvolle) Achseneinteilung fuer eine Zeitachse berechnen
* - die meisten Funktionen wurden von QwtLinearScaleEngine kopiert,
*   da nicht alle benoetigen Funktionen virtual waren
*---------------------------------------------------------------------------*/

#include <QVarLengthArray>

#include "qwt_math.h"

#include "datetimescaleengine.h"

/*---------------------------------------------------------------------------
* eigene Funktionen
*---------------------------------------------------------------------------*/
DateTimeScaleEngine::DateTimeScaleEngine(int offsetUTCToLocalTime) : QwtLinearScaleEngine()
{
   setAttribute(QwtScaleEngine::Floating, true);
   this->offsetUTCToLocalTime = offsetUTCToLocalTime;
}

/*---------------------------------------------------------------------------
* Intervall sinnvoll in Zeitabschnitte teilen
*---------------------------------------------------------------------------*/
double DateTimeScaleEngine::divideInterval(
    double intervalSize, int numSteps) const
{
   if ( numSteps <= 0 )
      return 0.0;

   // Liste mit Zoom-Schritten
   const qint32 zoomStepsSize = 14;
   const qint32 zoomSteps[14] = {
             60*1,         // 1 min
             60*2,         // 2 min
             60*5,         // 5 min
             60*10,        // 10 min
             60*15,        // 15 min
             60*30,        // 30 min
             60*60*1,      // 1 h
             60*60*2,      // 2 h
             60*60*6,      // 6 h
             60*60*12,     // 12 h
             60*60*24*1,   // 1 day
             60*60*24*2,   // 2 days
             60*60*24*7,   // 1 week
             60*60*24*14,  // 2 week
          };

   // Zoom-Schritt finden, der am besten passt
   double idealStep = intervalSize / numSteps;
   double step = zoomSteps[0];
   for( int i=1; i<zoomStepsSize; i++)
      if( qAbs(idealStep-zoomSteps[i]) < qAbs(idealStep-step))
         step = zoomSteps[i];
   return step;
}

/*---------------------------------------------------------------------------
* Haupt-Grid bauen
*---------------------------------------------------------------------------*/
QList<double> DateTimeScaleEngine::buildMajorTicks( const QwtInterval &interval,
                                                   double stepSize) const
{
   int numTicks = qRound(interval.width() / stepSize) + 1;
   if ( numTicks > 10000 )
      numTicks = 10000;

   // add UTC offset for steps > 1h
   double offset = 0;
   if( stepSize > 60*60) offset = offsetUTCToLocalTime;

   QList<double> ticks;

   ticks += interval.minValue() + offset;
   for (int i = 1; i < numTicks; i++)
      ticks += interval.minValue() + i * stepSize + offset;

   return ticks;
}

/*---------------------------------------------------------------------------
* Funktionen von QwtLinearScaleEngine kopiert
*---------------------------------------------------------------------------*/
void DateTimeScaleEngine::autoScale(int maxNumSteps,
                                    double &x1,
                                    double &x2,
                                    double &stepSize) const
{
   QwtInterval interval(x1, x2);
   interval = interval.normalized();

   interval.setMinValue(interval.minValue() - lowerMargin());
   interval.setMaxValue(interval.maxValue() + upperMargin());

   if (testAttribute(QwtScaleEngine::Symmetric))
      interval = interval.symmetrize(reference());

   if (testAttribute(QwtScaleEngine::IncludeReference))
      interval = interval.extend(reference());

   if (interval.width() == 0.0)
      interval = buildInterval(interval.minValue());

   stepSize = divideInterval(interval.width(), qMax(maxNumSteps, 1));

   if ( !testAttribute(QwtScaleEngine::Floating) )
      interval = align(interval, stepSize);

   x1 = interval.minValue();
   x2 = interval.maxValue();
   //qDebug() << int(interval.minValue()) << int(interval.maxValue());

   if (testAttribute(QwtScaleEngine::Inverted))
   {
      qSwap(x1, x2);
      stepSize = -stepSize;
   }
}

QwtScaleDiv DateTimeScaleEngine::divideScale(double x1,
                                             double x2,
                                             int maxMajSteps,
                                             int maxMinSteps,
                                             double stepSize) const
{
   QwtInterval interval = QwtInterval(x1, x2).normalized();
   if (interval.width() <= 0 )
      return QwtScaleDiv();

   stepSize = qAbs(stepSize);
   if ( stepSize == 0.0 )
   {
      if ( maxMajSteps < 1 )
         maxMajSteps = 1;

      stepSize = divideInterval(interval.width(), maxMajSteps);
   }

   QwtScaleDiv scaleDiv;

   if ( stepSize != 0.0 )
   {
      QList<double> ticks[QwtScaleDiv::NTickTypes];
      buildTicks(interval, stepSize, maxMinSteps, ticks);

      scaleDiv = QwtScaleDiv(interval, ticks);
   }

   if ( x1 > x2 )
      scaleDiv.invert();

   return scaleDiv;
}

void DateTimeScaleEngine::buildTicks( const QwtInterval& interval,
                                      double stepSize,
                                      int maxMinSteps,
                                      QList<double> ticks[QwtScaleDiv::NTickTypes]) const
{
   const QwtInterval boundingInterval = align(interval, stepSize);

   ticks[QwtScaleDiv::MajorTick] = buildMajorTicks(boundingInterval, stepSize);

   if ( maxMinSteps > 0 )
   {
      buildMinorTicks(ticks[QwtScaleDiv::MajorTick], maxMinSteps, stepSize,
                      ticks[QwtScaleDiv::MinorTick], ticks[QwtScaleDiv::MediumTick]);
   }

   for ( int i = 0; i < QwtScaleDiv::NTickTypes; i++ )
   {
      ticks[i] = strip(ticks[i], interval);

      // ticks very close to 0.0 are
      // explicitely set to 0.0

      for ( int j = 0; j < (int)ticks[i].count(); j++ )
      {
         if ( qwtFuzzyCompare(ticks[i][j], 0.0, stepSize) == 0 )
            ticks[i][j] = 0.0;
      }
   }
}

void DateTimeScaleEngine::buildMinorTicks( const QList<double>& majorTicks,
                                           int maxMinSteps,
                                           double stepSize,
                                           QList<double> &minorTicks,
                                           QList<double> &mediumTicks) const
{
   double minStep = divideInterval(stepSize, maxMinSteps);
   if (minStep == 0.0)
      return;

   // # ticks per interval
   int numTicks = (int)::ceil(qAbs(stepSize / minStep)) - 1;

   // Do the minor steps fit into the interval?
   if ( qwtFuzzyCompare((numTicks +  1) * qAbs(minStep),
                                       qAbs(stepSize), stepSize) > 0)
   {
      numTicks = 1;
      minStep = stepSize * 0.5;
   }

   int medIndex = -1;
   if ( numTicks % 2 )
      medIndex = numTicks / 2;

   // calculate minor ticks

   for (int i = 0; i < (int)majorTicks.count(); i++)
   {
      double val = majorTicks[i];
      for (int k = 0; k < numTicks; k++)
      {
         val += minStep;

         double alignedValue = val;
         if (qwtFuzzyCompare(val, 0.0, stepSize) == 0)
            alignedValue = 0.0;

         if ( k == medIndex )
            mediumTicks += alignedValue;
         else
            minorTicks += alignedValue;
      }
   }
}
