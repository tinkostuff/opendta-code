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
* Klasse zur Berechnung von statistischen Groessen fuer die Felder der
* DTA Dateien (des Datenarrays)
*
* Besonderheit fuer Mittelwert, Median und Standardabweichung bei analoge
* Signalen (die Datenpunkte muessen nicht unbedingt equidistant sein)
* - Mittelwert:
*     Mittelwert = Summe(Wert * Schrittweite)/Zeitspanne
* - Median:
*     - in einer QMap werden alle aufgetretenen (sortiert) Werte mit der Summe
*       der Zeitspannen, in der sie aufgetreten sind, gespeichert
*     - der Median ist der Wert, an dem die Summe der Zeitspannen genau
*       die Haelfte des gesamten Zeitbereiches entspricht
* - Standardabweichung
*     - es wird die gleiche QMap benutzt, wie beim Median
*     - stdev = Wurzel( Summe( Zeitspanne * (Wert - Mittelwert)^2 ) / Zeitbereich)
*---------------------------------------------------------------------------*/

#include <QtCore/qmath.h>

#include <QtGlobal>
#include <QDebug>

#include "dtafieldstatistics.h"

/*---------------------------------------------------------------------------
* Constructor
*---------------------------------------------------------------------------*/
DtaFieldStatistics::DtaFieldStatistics( QObject *parent) : QObject(parent)
{
   m_analogFields.clear();
   m_analogStaticFields.clear();
   m_digitalFields.clear();
   m_digitalStaticFields.clear();
   m_dataStart = 0;
   m_dataEnd = 0;
   m_datasets = 0;
   m_missingCount = 0;
   m_missingSum = 0;
}
DtaFieldStatistics::DtaFieldStatistics( DtaDataMap::const_iterator iteratorStart,
                                        DtaDataMap::const_iterator iteratorEnd,
                                        QObject *parent) : QObject(parent)
{
   this->calcStatistics(iteratorStart,iteratorEnd);
}

/*---------------------------------------------------------------------------
* Statistik berechnen
*---------------------------------------------------------------------------*/
void DtaFieldStatistics::calcStatistics( DtaDataMap::const_iterator iteratorStart,
                                    DtaDataMap::const_iterator iteratorEnd)
{
   // locale Variablen
   DtaDataMap::const_iterator iterator = iteratorStart;
   quint32 lastTS = 0;
   bool firstDataset = true; // erster Datansatz in Bearbeitung
   QHash<QString, QMap<qreal,quint32> > analogValueMap; // Liste mit Werten zur Ermittlung des Median und der Standardabweichung
   QHash<QString,quint32> digitalLastTransition; // Zeitpunkt des letzten Zustandswechsels
   QHash<QString,quint32> digitalAvgOnCount;
   QHash<QString,quint32> digitalAvgOffCount;

   do
   {
      quint32 ts = iterator.key();
      DtaFieldValues data = iterator.value();

      if(firstDataset)
      {
         // Initialisierung
         m_dataStart = ts;
         m_datasets = 0;
         m_missingSum = m_missingCount = 0;
         m_analogFields.clear();
         m_analogStaticFields.clear();
         m_analogValues.clear();
         m_digitalFields.clear();
         m_digitalStaticFields.clear();
         m_digitalValues.clear();

         // Feldliste durchlaufen und analog/digitale Felder suchen
         for( int i=0; i<data.size(); ++i)
         {
            QString field = DtaFile::fieldName(i);
            qreal value = data[i];
            const DtaFieldInfo *info = DtaFile::fieldInfo(i);

            if(info->analog)
            {
               // analoges Feld: min, max, Durchschnitt, Median, Standardabweichung
               m_analogFields << field;
               m_analogValues.insert( field, QVarLengthArray<qreal>(aStdev+1));
               m_analogValues[field][aMin]    = value;
               m_analogValues[field][aMax]    = value;
               m_analogValues[field][aAvg]    = 0.0;
               m_analogValues[field][aMedian] = 0.0;
               m_analogValues[field][aStdev]  = 0.0;
               analogValueMap.insert( field, QMap<qreal,quint32>());
            }
            else
            {
               // digitale Felder: Anzahl von Impulsen, Zeit (in)aktive, Durschnittliche Laufzeit
               m_digitalFields << field;
               m_digitalValues.insert( field, QVarLengthArray<quint32>(dOff+1));
               m_digitalValues[field][dActOn] = 0;
               m_digitalValues[field][dActOff] = 0;
               m_digitalValues[field][dLast] = qRound(value);
               m_digitalValues[field][dMinOn] = 0;
               m_digitalValues[field][dMaxOn] = 0;
               m_digitalValues[field][dAvgOn] = 0;
               m_digitalValues[field][dMinOff] = 0;
               m_digitalValues[field][dMaxOff] = 0;
               m_digitalValues[field][dAvgOff] = 0;
               m_digitalValues[field][dOn] = 0;
               m_digitalValues[field][dOff] = 0;
               digitalLastTransition.insert( field, 0);
               digitalAvgOnCount.insert( field, 0);
               digitalAvgOffCount.insert( field, 0);
            }
         } // for i (Felder)

         firstDataset = false;
      }
      else // !firstDataset
      {
         // Luecken suchen
         bool missingFound = false;
         if(ts-lastTS > 65) // 65 Sekunden fuer ein bisschen Spielraum
         {
            m_missingCount++;
            m_missingSum += ts - lastTS - 60;
            missingFound = true;
         }

         // analoge Signale bearbeiten
         for( int i=0; i<m_analogFields.size(); i++)
         {
            QString field = m_analogFields.at(i);
            qreal value = DtaFile::fieldValueReal(data,field);
            if( value < m_analogValues[field][aMin])
               m_analogValues[field][aMin]=value;
            if( value > m_analogValues[field][aMax])
               m_analogValues[field][aMax]=value;
            m_analogValues[field][aAvg] += value * (ts-lastTS);
            if(analogValueMap[field].contains(value))
               analogValueMap[field][value] += ts-lastTS;
            else
               analogValueMap[field][value] = ts-lastTS;
         } // for analogFields

         // digitale Signale bearbeiten
         for( int i=0; i<m_digitalFields.size(); i++)
         {
            QString field = m_digitalFields.at(i);
            qint32 value = DtaFile::fieldValueInt(data,field);

            if(missingFound)
               // Luecke entdeckt
               digitalLastTransition[field] = 0;

//            if(!missingFound)
//            {
               qint32 last = m_digitalValues[field][dLast];
               if(value == 1)
               {
                  if(last==0)
                  {
                     // Ende einer AUS-Phase
                     m_digitalValues[field][dActOff]++;
                     if(digitalLastTransition[field] > 0)
                     {
                        quint32 runtime = ts - digitalLastTransition[field];
                        if( (m_digitalValues[field][dMinOff]==0) ||
                            (runtime < m_digitalValues[field][dMinOff]))
                           m_digitalValues[field][dMinOff] = runtime;
                        if( (m_digitalValues[field][dMaxOff]==0) ||
                            (runtime > m_digitalValues[field][dMaxOff]))
                           m_digitalValues[field][dMaxOff] = runtime;
                        m_digitalValues[field][dAvgOff] += runtime;
                        digitalAvgOffCount[field]++;
                     }
                     digitalLastTransition[field] = ts;
                  }
                  // EIN-Phase haelt an
                  m_digitalValues[field][dOn] += ts - lastTS;
               }
               else
               {
                  if(last==1)
                  {
                     // Ende einer EIN-Phase
                     m_digitalValues[field][dActOn]++;
                     if(digitalLastTransition[field] > 0)
                     {
                        quint32 runtime = ts - digitalLastTransition[field];
                        if( (m_digitalValues[field][dMinOn]==0) ||
                            (runtime < m_digitalValues[field][dMinOn]))
                           m_digitalValues[field][dMinOn] = runtime;
                        if( (m_digitalValues[field][dMaxOn]==0) ||
                            (runtime > m_digitalValues[field][dMaxOn]))
                           m_digitalValues[field][dMaxOn] = runtime;
                        m_digitalValues[field][dAvgOn] += runtime;
                        digitalAvgOnCount[field]++;
                     }
                     digitalLastTransition[field] = ts;
                  }
                  // AUS-Phase haelt an
                  m_digitalValues[field][dOff] += ts - lastTS;
               } // if/else value==1/0
//            } // if !missingFound

            // letzten Wert Speichern
            m_digitalValues[field][dLast] = value;
         }

      } // if/else firstDataset

      // Ende der Daten merken
      m_dataEnd = ts;
      m_datasets++;

      // letzten Zeitstempel merken
      lastTS = ts;

      // naechster Datensatz
      iterator++;
   } while( iterator != iteratorEnd);

   // Nachbearbeitung analoge Signale
   for( int i=0; i<m_analogFields.size(); i++)
   {
      QString field = m_analogFields.at(i);

      // statische Signale aussortieren
      if( m_analogValues[field][aMin] == m_analogValues[field][aMax])
      {
         m_analogStaticFields << field;
         continue;
      }

      // Durchschnitt
      m_analogValues[field][aAvg] = m_analogValues[field][aAvg]/(m_dataEnd-m_dataStart);

      quint32 range = m_dataEnd - m_dataStart;
      quint32 sumTime = 0;
      bool medianFound = false;
      QMap<qreal,quint32>::const_iterator iterator = analogValueMap[field].begin();
      QMap<qreal,quint32>::const_iterator iteratorEnd = analogValueMap[field].end();
      qreal lastValue = iterator.value();
      do
      {
         // Median
         if( !medianFound && (sumTime==range/2) )
         {
            m_analogValues[field][aMedian] = (iterator.key() + lastValue)/2;
            medianFound = true;
         }
         sumTime += iterator.value();
         if( !medianFound && (sumTime > range/2) )
         {
            m_analogValues[field][aMedian] = iterator.key();
            medianFound = true;
         }
         lastValue = iterator.key();

         // Standardabweichung
         m_analogValues[field][aStdev] +=
               iterator.value() * qPow(iterator.key() - m_analogValues[field][aAvg], 2);

         // naechster Datenpunkt
         iterator++;
      } while( iterator != iteratorEnd);

      // Standardabweichung
      m_analogValues[field][aStdev] =
            qSqrt( m_analogValues[field][aStdev] / qreal(sumTime));
   } // for analogFields

   // Nachbearbeitung digitalen Signale
   for( int i=0; i<m_digitalFields.size(); i++)
   {
      QString field = m_digitalFields.at(i);
      if( (m_digitalValues[field][dActOn]==0) && (m_digitalValues[field][dActOff]==0))
      {
         m_digitalStaticFields << field;
         continue;
      }

      // Mittelwert
      m_digitalValues[field][dAvgOn] =
            qRound( qreal(m_digitalValues[field][dAvgOn]) / qreal(digitalAvgOnCount[field]));
      m_digitalValues[field][dAvgOff] =
            qRound( qreal(m_digitalValues[field][dAvgOff]) / qreal(digitalAvgOffCount[field]));

   } // for digitalFields

   // statische Signal aus Liste entfernen
   for( int i=0; i<m_analogStaticFields.size(); i++)
      m_analogFields.removeOne(m_analogStaticFields.at(i));
   for( int i=0; i<m_digitalStaticFields.size(); i++)
      m_digitalFields.removeOne(m_digitalStaticFields.at(i));

   // Felder sortieren
   m_analogFields.sort();
   m_analogStaticFields.sort();
   m_digitalFields.sort();
   m_digitalStaticFields.sort();
}
