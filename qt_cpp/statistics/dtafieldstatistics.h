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
*---------------------------------------------------------------------------*/
#ifndef DTAFIELDSTATISTICS_H
#define DTAFIELDSTATISTICS_H

#include <QObject>
#include <QVarLengthArray>
#include <QHash>

#include "dtafile/dtafile.h"

class DtaFieldStatistics : public QObject
{
   Q_OBJECT
public:
   // Constructor
   explicit DtaFieldStatistics( QObject *parent = 0);
   explicit DtaFieldStatistics( DtaDataMap::const_iterator iteratorStart,
                                DtaDataMap::const_iterator iteratorEnd,
                                QObject *parent = 0);

   void calcStatistics( DtaDataMap::const_iterator iteratorStart,
                        DtaDataMap::const_iterator iteratorEnd);

   // Rueckgabe der Feldnamen
   inline const QStringList analogFields() { return m_analogFields;}
   inline const QStringList analogStaticFields() { return m_analogStaticFields;}
   inline const QStringList digitalFields() { return m_digitalFields;}
   inline const QStringList digitalStaticFields() { return m_digitalStaticFields;}

   // Rueckgabe der Werte fuer analoge Felder
   inline qreal analogMin(const QString &field) { return m_analogValues[field][aMin];}
   inline qreal analogMax(const QString &field) { return m_analogValues[field][aMax];}
   inline qreal analogAvg(const QString &field) { return m_analogValues[field][aAvg];}
   inline qreal analogMedian(const QString &field) { return m_analogValues[field][aMedian];}
   inline qreal analogStdev(const QString &field) { return m_analogValues[field][aStdev];}

   // Rueckgabe der Werte fuer digitale Felder
   inline quint32 digitalActOn(const QString &field) { return m_digitalValues[field][dActOn];}
   inline quint32 digitalMinOn(const QString &field) { return m_digitalValues[field][dMinOn];}
   inline quint32 digitalMaxOn(const QString &field) { return m_digitalValues[field][dMaxOn];}
   inline quint32 digitalAvgOn(const QString &field) { return m_digitalValues[field][dAvgOn];}
   inline quint32 digitalTimeOn(const QString &field) { return m_digitalValues[field][dOn];}
   inline quint32 digitalActOff(const QString &field) { return m_digitalValues[field][dActOff];}
   inline quint32 digitalMinOff(const QString &field) { return m_digitalValues[field][dMinOff];}
   inline quint32 digitalMaxOff(const QString &field) { return m_digitalValues[field][dMaxOff];}
   inline quint32 digitalAvgOff(const QString &field) { return m_digitalValues[field][dAvgOff];}
   inline quint32 digitalTimeOff(const QString &field) { return m_digitalValues[field][dOff];}
   inline quint32 digitalLastValue(const QString &field) { return m_digitalValues[field][dLast];}

   // Zeitspanne
   inline quint32 timeRange() { return m_dataEnd - m_dataStart;}
   inline quint32 timeStart() { return m_dataStart;}
   inline quint32 timeEnd() { return m_dataEnd;}
   inline quint32 datasets() { return m_datasets;}

   // Aufzeichnungsluecken
   inline quint32 missingCount() { return m_missingCount;}
   inline quint32 missingSum() { return m_missingSum;}

signals:

public slots:

private:
   // Start, Ende
   quint32 m_dataStart;
   quint32 m_dataEnd;
   quint32 m_datasets;

   // Luecken
   quint32 m_missingCount;
   quint32 m_missingSum;

   // Werte fuer analoge Felder
   QStringList m_analogFields;
   QStringList m_analogStaticFields;
   enum analogValueArrayFields {aMin, aMax, aAvg, aMedian, aStdev};
   QHash<QString, QVarLengthArray<qreal> > m_analogValues;

   // Werte fuer digitale Felder
   QStringList m_digitalFields;
   QStringList m_digitalStaticFields;
   enum digitalValueArrayFields {dLast, dActOn, dMinOn, dMaxOn, dAvgOn, dActOff, dMinOff, dMaxOff, dAvgOff, dOn, dOff};
   QHash<QString, QVarLengthArray<qint32> > m_digitalValues;
};

#endif // DTAFIELDSTATISTICS_H
