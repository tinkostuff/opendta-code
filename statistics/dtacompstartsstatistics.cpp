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

#include "dtacompstartsstatistics.h"

/*---------------------------------------------------------------------------
*---------------------------------------------------------------------------
* DtaCompStart
*---------------------------------------------------------------------------
*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
* Consrtuctor
*---------------------------------------------------------------------------*/
DtaCompStart::DtaCompStart(QObject *parent) : QObject(parent)
{
   this->m_data.clear();
   for( int i=0; i<m_fieldList.size(); ++i)
      this->m_data.insert( m_fieldList.at(i), QVariant());
}
DtaCompStart::DtaCompStart(const DtaCompStart &cs, QObject *parent) : QObject(parent)
{
   this->m_data.clear();
   for( int i=0; i<m_fieldList.size(); ++i)
   {
      CompStartFields field = m_fieldList.at(i);
      this->m_data.insert( field, cs.value(field));
   }
}

/*---------------------------------------------------------------------------
* operators
*---------------------------------------------------------------------------*/
DtaCompStart DtaCompStart::operator=(const DtaCompStart &cs)
{
   DtaCompStart res;
   for( int i=0; i<m_fieldList.size(); ++i)
   {
      CompStartFields field = m_fieldList.at(i);
      res.m_data.insert( field, cs.value(field));
   }
   return res;
}

/*---------------------------------------------------------------------------
* Liste mit Feldern
*---------------------------------------------------------------------------*/
QList<DtaCompStart::CompStartFields> DtaCompStart::initFieldList()
{
   QList<CompStartFields> res;
   res << fStart << fLength << fMode << fPause << fTVL << fTRL << fSpHz
       << fTWQein << fTWQaus << fSpWQ << fDF << fTA << fWM << fE1 << fE2
       << fAZ1 << fAZ2;
   return res;
}
const QList<DtaCompStart::CompStartFields> DtaCompStart::m_fieldList = DtaCompStart::initFieldList();

/*---------------------------------------------------------------------------
* Liste mit Feldnamen
*---------------------------------------------------------------------------*/
QStringList DtaCompStart::fieldNames()
{
   QStringList res;
   res << tr("Start")
       << tr("L\344nge [min]")
       << tr("Modus")
       << tr("Pause [min]")
       << tr("TVL [\260C]")
       << tr("TRL [\260C]")
       << tr("SpHz [K]")
       << tr("TWQein [\260C]")
       << tr("TWQaus [\260C]")
       << tr("SpWQ [K]")
       << tr("DF [l/min]")
       << tr("TA [\260C]")
       << tr("WM [kWh]")
       << tr("E1 [kWh]")
       << tr("E2 [kWh]")
       << tr("AZ1 []")
       << tr("AZ2 []");
   return res;
}

/*---------------------------------------------------------------------------
* Liste mit Modi
*---------------------------------------------------------------------------*/
QStringList DtaCompStart::modeStringList()
{
   QStringList res;
   res << tr("Heizung")
       << tr("Brauchwasser")
       << tr("von EVU unterbrochen")
       << tr("nach EVU-Sperre");
   return res;
}

/*---------------------------------------------------------------------------
* conversion to string
*---------------------------------------------------------------------------*/
const QString DtaCompStart::toString()
{
   QStringList res;
   QHashIterator<CompStartFields,QVariant> i(m_data);
   while(i.hasNext())
   {
      i.next();
      res << QString("%1=%2").arg(fieldName(i.key()).split(" ").at(0)).arg(i.value().toString());
   }
   return res.join(" ");
}

/*---------------------------------------------------------------------------
*---------------------------------------------------------------------------
* DtaCompStartsStatistics
*---------------------------------------------------------------------------
*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
* Constructor
*---------------------------------------------------------------------------*/
DtaCompStartsStatistics::DtaCompStartsStatistics(QObject *parent) :
    QObject(parent)
{
   m_dataStart = 0;
   m_dataEnd = 0;
   m_datasets = 0;
   m_missingCount = 0;
   m_missingSum = 0;
   m_runs.clear();
}
DtaCompStartsStatistics::DtaCompStartsStatistics( DataMap::const_iterator iteratorStart,
                                                  DataMap::const_iterator iteratorEnd,
                                                  QObject *parent) : QObject(parent)
{
   this->calcStatistics(iteratorStart,iteratorEnd);
}

/*---------------------------------------------------------------------------
* Berechnung
*---------------------------------------------------------------------------*/
void DtaCompStartsStatistics::calcStatistics(DataMap::const_iterator iteratorStart, DataMap::const_iterator iteratorEnd)
{
   DataMap::const_iterator iterator = iteratorStart;
   quint32 lastTS = 0;
   bool firstDataset = true; // erster Datansatz in Bearbeitung

   states state;
   states stateEVU;
   states lastState = stateOff;
   states lastEVUState = stateOff;

   QHash<DtaCompStart::CompStartModes, bool> firstRun;
   QHash<DtaCompStart::CompStartModes, quint32> lastRunEnd;
   bool inlineRun = false;
   quint32 inlineRunLength = 0;
   bool invalidRun = false;
   quint32 lastEVUEnd = 0;

   //
   // Verdichter Starts ermitteln
   //

   DtaCompStart cmprun;
   DtaCompStart cmprunSave;

   m_missingCount = 0;
   m_missingSum = 0;
   m_datasets = 0;
   m_dataStart = 0;
   m_dataEnd = 0;
   m_runs.clear();
   m_runCounts.clear();
   m_statValues.clear();

   while( iterator != iteratorEnd)
   {
      quint32 ts = iterator.key();
      DataFieldValues data = iterator.value();

      qint32 vd1 = DataFile::fieldValueInt( data, "VD1");
      qint32 evu = DataFile::fieldValueInt( data, "EVU");
      qint32 bup = DataFile::fieldValueInt( data, "BUP");
      qint32 av = DataFile::fieldValueInt( data, "AV");

      // aktuellen Zustand ermitteln
      if(vd1==0) state = stateOff;
      else
      {
         if(bup==0) state = stateHz;
         else state = stateBW;
      }
      if(evu==1) stateEVU = stateOn;
      else stateEVU = stateOff;

      if(firstDataset)
      {
         // Anfang der Daten merken
         m_dataStart = ts;
         // Lauf zuruecksetzen
         if(vd1==1) invalidRun = true;

         firstRun.insert( DtaCompStart::mHz, true);
         firstRun.insert( DtaCompStart::mBW, true);
         lastRunEnd.insert( DtaCompStart::mHz, 0);
         lastRunEnd.insert( DtaCompStart::mBW, 0);

         firstDataset = false;
      }
      else
      {
         // Luecken suchen
         bool missingFound = false;
         if(ts-lastTS > MISSING_DATA_GAP)
         {
            m_missingCount++;
            m_missingSum += ts - lastTS - 60;
            missingFound = true;

            firstRun.insert( DtaCompStart::mHz, true);
            firstRun.insert( DtaCompStart::mBW, true);

            // Lauf zuruecksetzen
            if(vd1==1) invalidRun = true;
         }

         // Ende von EVU-Unterbrechung merken
         if( (stateEVU==stateOff) && (lastEVUState==stateOn))
            lastEVUEnd = ts;

         // ungueltigen Lauf zuruecksetzen und Ende merken
         if( invalidRun && (state==stateOff) && (lastState!=stateOff))
         {
            if(lastState==stateHz) lastRunEnd[DtaCompStart::mHz] = ts;
            else if(lastState==stateBW) lastRunEnd[DtaCompStart::mBW] = ts;
            invalidRun = false;
         }

         // Start eines Laufes
         //    AUS -> Hz oder BW
         //    Hz -> BW
         else if( ((lastState==stateOff) && ((state==stateHz) || (state==stateBW))) ||
                  ((lastState==stateHz) && (state==stateBW)))
         {
            // Modus waerend des Laufes gewechselt
            if (lastState==stateHz)
            {
               inlineRun = true;
               // aktuellen Lauf sichern - operator= funktioniert nicht, warum?
               for( int i=0; i<DtaCompStart::fieldCount(); ++i)
                  cmprunSave.setValue(DtaCompStart::CompStartFields(i), cmprun.value(DtaCompStart::CompStartFields(i)));
               //cmprunSave = cmprun; // aktuellen Lauf sichern
            }

            // Anfang eines neuen Laufes
            else
               inlineRun = false;

            cmprun.setStart(ts);
            if(state==stateBW)
               // Warmwasser
               cmprun.setMode(DtaCompStart::mBW);
            else
               cmprun.setMode(DtaCompStart::mHz);
         }

         // Verdichter laeuft und Zustand hat sich nicht geaendert
         else if( ((state==stateHz) || (state==stateBW)) && (state==lastState))
         {
            // Daten des Laufes speichern
            if(av == 0)
            {
               // Temperaturen nur merken, wenn nicht abgetaut wird
               cmprun.setTA(DataFile::fieldValueReal(data,"TA"));
               cmprun.setTRL(DataFile::fieldValueReal(data,"TRL"));
               cmprun.setTVL(DataFile::fieldValueReal(data,"TVL"));
               cmprun.setSpHz(DataFile::fieldValueReal(data,"SpHz"));
               cmprun.setTWQein(DataFile::fieldValueReal(data,"TWQein"));
               cmprun.setTWQaus(DataFile::fieldValueReal(data,"TWQaus"));
               cmprun.setSpWQ(DataFile::fieldValueReal(data,"SpWQ"));
            }
            cmprun.setDF(DataFile::fieldValueReal(data,"DF"));
            cmprun.setWM(DataFile::fieldValueReal(data,"WMCalc"));
            cmprun.setE1(DataFile::fieldValueReal(data,"E1"));
            cmprun.setE2(DataFile::fieldValueReal(data,"E2"));
         }

         // Ende eines Laufes
         //  Hz oder BW -> AUS
         //  BW -> Hz
         else if( ((state==stateOff) && ((lastState==stateHz) || (lastState==stateBW))) ||
                  ((state==stateHz) && (lastState==stateBW)))
         {
            bool moreRunsToSave = true;
            while(moreRunsToSave)
            {
               // Daten des Laufes speichern
               cmprun.setLength(ts-cmprun.start()-inlineRunLength);
               inlineRunLength = 0;
               cmprun.setAZ1( cmprun.WM() / cmprun.E1());
               cmprun.setAZ2( cmprun.WM() / (cmprun.E1() + cmprun.E2()));

               // von EVU unterbrochen?
               if(stateEVU==stateOn)
               {
                  firstRun[cmprun.mode()] = true;
                  cmprun.setMode(DtaCompStart::mEVUstop);
                  cmprun.setPause(0);
               }
               else
               {
                  // Pause berechnen
                  if(firstRun[cmprun.mode()])
                     cmprun.setPause(0);
                  else
                     cmprun.setPause(cmprun.start()-lastRunEnd[cmprun.mode()]);

                  // Ende des Laufes merken
                  lastRunEnd[cmprun.mode()] = ts;
                  firstRun[cmprun.mode()] = false;
               }

               // nach EVU unterbrechung
               if( (cmprun.mode()==DtaCompStart::mHz) &&
                   (cmprun.start()-lastEVUEnd < 480)) // 8min Netzeinschaltverzoegerung
               {
                  cmprun.setPause(0);
                  cmprun.setMode(DtaCompStart::mAfterEVU);
               }

               // Lauf an Liste anhaengen
               m_runs << cmprun;

               // inline Lauf auch speichern?
               if(inlineRun)
               {
                  inlineRunLength = cmprun.length();
                  // aeusseren Lauf wieder herstellen - operator= funktioniert nicht, warum?
                  for( int i=0; i<DtaCompStart::fieldCount(); ++i)
                     cmprun.setValue(DtaCompStart::CompStartFields(i), cmprunSave.value(DtaCompStart::CompStartFields(i)));
                  //cmprun = cmprunSave;
                  inlineRun = false;
                  // Inline-Lauf nur Speichern, wenn EVU unterbrochen hat
                  moreRunsToSave = (state==stateOff);
               }
               else
               {
                  moreRunsToSave = false;

                  // Behebung folgender Situaltion:
                  //  - WP hat angefangen WW zu bereiten
                  //  - WW ist beendet, aber Kompressor laeuft weiter (Heizen)
                  //  - dies ist ein ungueltiger Zustand, weil, wenn die WP nur
                  //    WW bereitet, sie erst einmal ein Pause macht, bevor sie
                  //    wieder in dem Modus Heizen startet
                  if(state==stateHz) invalidRun=true;
               }
            }
         }

      } // if/else firstDataset

      // letzten Zustand merken
      lastState = state;
      lastEVUState = stateEVU;

      // Ende der Daten merken
      m_dataEnd = ts;
      m_datasets++;

      // letzten Zeitstempel merken
      lastTS = ts;

      // naechster Datensatz
      iterator++;
   }

   //
   // Statistk der Verdichterstarts
   //
   QHash< DtaCompStart::CompStartModes, QHash< DtaCompStart::CompStartFields, QList<qreal> > > valueLists;

   // Daten initialisieren
   for( int i=0; i<DtaCompStart::modeCount(); ++i)
   {
      DtaCompStart::CompStartModes mode = (DtaCompStart::CompStartModes)i;

      m_statValues.insert( mode, QHash< DtaCompStart::CompStartFields, QHash< statValueFields, qreal> >());
      m_runCounts.insert( mode, 0);
      valueLists.insert( mode, QHash< DtaCompStart::CompStartFields, QList<qreal> >());

      for( int j=0; j<DtaCompStart::fieldCount(); ++j)
      {
         DtaCompStart::CompStartFields field = (DtaCompStart::CompStartFields)j;
         if( (field==DtaCompStart::fStart) || (field==DtaCompStart::fMode))
            continue;

         m_statValues[mode].insert( field, QHash<statValueFields,qreal>());
         m_statValues[mode][field].insert( sMin, 0.0);
         m_statValues[mode][field].insert( sMax, 0.0);
         m_statValues[mode][field].insert( sAvg, 0.0);
         m_statValues[mode][field].insert( sMedian, 0.0);
         m_statValues[mode][field].insert( sStdev, 0.0);

         valueLists[mode].insert( field, QList<qreal>());
      }
   }


   for( int i=0; i<m_runs.size(); ++i)
   {
      DtaCompStart run = m_runs.at(i);
      DtaCompStart::CompStartModes mode = run.mode();

      for( int j=0; j<DtaCompStart::fieldCount(); ++j)
      {
         DtaCompStart::CompStartFields field = (DtaCompStart::CompStartFields)j;

         if( (field==DtaCompStart::fStart) || (field==DtaCompStart::fMode))
            continue;

         qreal value = run.value(field).toDouble();

         if(m_runCounts[mode]==0)
         {
            m_statValues[mode][field][sMin] = value;
            m_statValues[mode][field][sMax] = value;
            m_statValues[mode][field][sAvg] = value;
            m_statValues[mode][field][sMedian] = 0.0;
            m_statValues[mode][field][sStdev] = 0.0;
         }
         else
         {
            if((field==DtaCompStart::fPause) && (value!=0))
            {
               if( (m_statValues[mode][field][sMin]==0) || (value<m_statValues[mode][field][sMin]))
                  m_statValues[mode][field][sMin] = value;
               if( (m_statValues[mode][field][sMax]==0) || (value>m_statValues[mode][field][sMax]))
                  m_statValues[mode][field][sMax] = value;
               m_statValues[mode][field][sAvg] += value;
            }
            else
            {
               if( value < m_statValues[mode][field][sMin])
                  m_statValues[mode][field][sMin] = value;
               if( value > m_statValues[mode][field][sMax])
                  m_statValues[mode][field][sMax] = value;
               m_statValues[mode][field][sAvg] += value;
            }
         }
         if( ((field==DtaCompStart::fPause) && (value!=0)) || (field!=DtaCompStart::fPause))
            valueLists[mode][field].append(value);
      } // for field
      m_runCounts[mode]++;
   } // for run

   // Nachbearbeitung der Statistik
   for( int i=0; i<DtaCompStart::modeCount(); ++i)
   {
      DtaCompStart::CompStartModes mode = (DtaCompStart::CompStartModes)i;
      for( int j=0; j<DtaCompStart::fieldCount(); ++j)
      {
         DtaCompStart::CompStartFields field = (DtaCompStart::CompStartFields)j;
         if( (field==DtaCompStart::fStart) || (field==DtaCompStart::fMode))
            continue;

         // Mittelwert
         if(valueLists[mode][field].size()!=0)
            m_statValues[mode][field][sAvg] =
                  m_statValues[mode][field][sAvg]/valueLists[mode][field].size();

         // Median und Standardabweichung
         qSort(valueLists[mode][field]);
         quint32 n = valueLists[mode][field].size();
         if( n == 0)
            m_statValues[mode][field][sMedian] = 0.0;
         else if( n%2 == 1)
            m_statValues[mode][field][sMedian] = valueLists[mode][field].at(n/2);
         else
            m_statValues[mode][field][sMedian] =
                  (valueLists[mode][field].at(n/2-1) + valueLists[mode][field].at(n/2))/2.0;

         // Standardabweichung
         m_statValues[mode][field][sStdev] = 0.0;
         if(n>1)
         {
            for( int k=0; k<valueLists[mode][field].size(); k++)
            {
               qreal v = valueLists[mode][field].at(k);
               m_statValues[mode][field][sStdev] +=
                     qPow(v - m_statValues[mode][field][sAvg],2);
            }
            m_statValues[mode][field][sStdev] =
                  qSqrt( m_statValues[mode][field][sStdev] / qreal(n-1));
         }
      }
   }

}

/*---------------------------------------------------------------------------
* Feld in String konvertieren und entsp. formatieren
*---------------------------------------------------------------------------*/
const QString DtaCompStartsStatistics::statStr( DtaCompStart::CompStartModes mode,
                                                DtaCompStart::CompStartFields field,
                                                statValueFields valueField) const
{
   qreal value = m_statValues[mode][field][valueField];
   switch(field)
   {
   case DtaCompStart::fLength:
   case DtaCompStart::fPause:
   {
      qint32 i = qRound(value);
      return QString("%1:%2")
               .arg(i/3600)
               .arg(QString("%1").arg((i%3600)/60.0,0,'f',0).rightJustified(2,'0'));
      break;
   }

   default:
   {
      return QString("%1").arg(value,0,'f',1);
      break;
   }
   } // switch
}
