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
* Klasse zur Ermittlung der Verdichter Starts und zur Berechnung von
* statistischen Werten zu diesen Starts
*---------------------------------------------------------------------------*/
#ifndef DTACOMPSTARTSSTATISTICS_H
#define DTACOMPSTARTSSTATISTICS_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <QStringList>

#include "dtafile/dtafile.h"

#define UNDEFINED_VALUE -1024

/*---------------------------------------------------------------------------
* Klasse fuer einen Verdichter Start
*---------------------------------------------------------------------------*/
class DtaCompStart : public QObject
{
   Q_OBJECT
public:
   enum CompStartFields { fStart, fLength, fMode, fPause, fTVL, fTRL, fSpHz, fTWQein, fTWQaus, fSpWQ, fTA};
   enum CompStartModes { mHz, mBW, mEVUstop, mAfterEVU};

   DtaCompStart(QObject *parent = 0);
   DtaCompStart(const DtaCompStart &cs, QObject *parent=0);
   static const QStringList fieldNames() { return m_fieldNames;}
   static const QString fieldName(CompStartFields field) { return m_fieldNames[field];}
   static const int fieldCount() { return (int)fTA+1;}
   static const QStringList modeStringList() { return m_modeStringList;}
   static const QString modeString(CompStartModes mode) { return m_modeStringList.at((int)mode);}
   static const int modeCount() { return (int)mAfterEVU+1;}
   inline QVariant value(const CompStartFields &field) const { return m_data[field];}
   inline void setValue(const CompStartFields &field, const QVariant &value) { m_data[field] = value;}
   DtaCompStart operator=(const DtaCompStart &cs);

   inline quint32 start() const { return m_data[fStart].toInt();}
   inline quint32 length() const { return m_data[fLength].toInt();}
   inline CompStartModes mode() const { return CompStartModes(m_data[fMode].toInt());}
   inline QString modeString() const { return m_modeStringList[m_data[fMode].toInt()];}
   inline quint32 pause() const { return m_data[fPause].toInt();}
   inline qreal TVL() const { return m_data[fTVL].toReal();}
   inline qreal TRL() const { return m_data[fTRL].toReal();}
   inline qreal SpHz() const { return m_data[fSpHz].toReal();}
   inline qreal TWQein() const { return m_data[fTWQein].toReal();}
   inline qreal TWQaus() const { return m_data[fTWQaus].toReal();}
   inline qreal SpWQ() const { return m_data[fSpWQ].toReal();}
   inline qreal TA() const { return m_data[fTA].toReal();}

   inline void setStart( const quint32 &value) { m_data[fStart] = value;}
   inline void setLength( const quint32 &value) { m_data[fLength] = value;}
   inline void setMode( const CompStartModes &value) { m_data[fMode] = value;}
   inline void setPause( const quint32 &value) { m_data[fPause] = value;}
   inline void setTVL( const qreal &value) { m_data[fTVL] = value;}
   inline void setTRL( const qreal &value) { m_data[fTRL] = value;}
   inline void setSpHz( const qreal &value) { m_data[fSpHz] = value;}
   inline void setTWQein( const qreal &value) { m_data[fTWQein] = value;}
   inline void setTWQaus( const qreal &value) { m_data[fTWQaus] = value;}
   inline void setSpWQ( const qreal &value) { m_data[fSpWQ] = value;}
   inline void setTA( const qreal &value) { m_data[fTA] = value;}
	
private:
   QHash<CompStartFields,QVariant> m_data;
   static const QStringList m_fieldNames;
   static const QList<CompStartFields> m_fieldList;
   static const QStringList m_modeStringList;
   static QList<CompStartFields> initFieldList();
   static QStringList initFieldNames();
   static QStringList initModeStringList();
};

/*---------------------------------------------------------------------------
* DtaCompStartsStatistics
*---------------------------------------------------------------------------*/
class DtaCompStartsStatistics : public QObject
{
    Q_OBJECT
public:
    DtaCompStartsStatistics(QObject *parent = 0);
    DtaCompStartsStatistics( DataMap::const_iterator iteratorStart,
                             DataMap::const_iterator iteratorEnd,
                             QObject *parent = 0);

    void calcStatistics( DataMap::const_iterator iteratorStart,
                         DataMap::const_iterator iteratorEnd);

    // Zeitspanne
    inline quint32 timeRange() const { return m_dataEnd - m_dataStart;}
    inline quint32 timeStart() const { return m_dataStart;}
    inline quint32 timeEnd() const { return m_dataEnd;}
    inline quint32 datasets() const { return m_datasets;}

    // Aufzeichnungsluecken
    inline quint32 missingCount() const { return m_missingCount;}
    inline quint32 missingSum() const { return m_missingSum;}

    // Verdichter Starts
    inline const QList<DtaCompStart> *runs() const { return &m_runs;}
    inline const int runCount() const { return m_runs.size();}
    inline const int runCount(DtaCompStart::CompStartModes mode) const { return m_runCounts[mode];}

    // Statistik der Starts
    inline const qreal statMin( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const {return m_statValues[mode][field][sMin];}
    inline const qreal statMax( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const {return m_statValues[mode][field][sMax];}
    inline const qreal statAvg( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const {return m_statValues[mode][field][sAvg];}
    inline const qreal statMedian( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const {return m_statValues[mode][field][sMedian];}
    inline const qreal statStdev( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const {return m_statValues[mode][field][sStdev];}

    inline const QString statMinStr( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const { return statStr(mode,field,sMin);}
    inline const QString statMaxStr( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const { return statStr(mode,field,sMax);}
    inline const QString statAvgStr( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const { return statStr(mode,field,sAvg);}
    inline const QString statMedianStr( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const { return statStr(mode,field,sMedian);}
    inline const QString statStdevStr( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field) const { return statStr(mode,field,sStdev);}
private:

    enum states {stateOff, stateOn, stateHz, stateBW};

    // Starts
    QList<DtaCompStart> m_runs;

    // Start, Ende
    quint32 m_dataStart;
    quint32 m_dataEnd;
    quint32 m_datasets;

    // Luecken
    quint32 m_missingCount;
    quint32 m_missingSum;

    // Statistik
    enum statValueFields {sMin, sMax, sAvg, sMedian, sStdev};
    QHash< DtaCompStart::CompStartModes, QHash< DtaCompStart::CompStartFields, QHash< statValueFields, qreal> > > m_statValues;
    QHash< DtaCompStart::CompStartModes, quint32> m_runCounts;

    const QString statStr( DtaCompStart::CompStartModes mode, DtaCompStart::CompStartFields field, statValueFields valueField) const;
};

#endif // DTACOMPSTARTSSTATISTICS_H
