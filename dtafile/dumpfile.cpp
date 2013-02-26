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
* - Klasse zum Lesen und Decodieren von Dump-Dateien
*---------------------------------------------------------------------------*/

#include "dumpfile.h"

/*---------------------------------------------------------------------------
* Constructor
*---------------------------------------------------------------------------*/
DumpFile::DumpFile(QString fileName, QObject *parent) : DataFile(fileName,parent)
{
}

DumpFile::~DumpFile()
{
   bzcat.close();
}

/*---------------------------------------------------------------------------
* Datei oeffnen
*---------------------------------------------------------------------------*/
bool DumpFile::open()
{
   // gibt es die Daten
   if( !QFile::exists(m_fileName))
   {
      errorMsg = QString(tr("FEHLER: Datei '%1' nicht gefunden!")).arg(m_fileName);
      qWarning() << errorMsg;
      return false;
   }

   // Process oeffnen
   bzcat.start("bzcat", QStringList() << m_fileName);
   bzcat.setReadChannel(QProcess::StandardOutput);
   if(!bzcat.waitForReadyRead())
   {
      errorMsg = QString(tr("FEHLER: 'bzcat' kann nicht gestarted werden!"));
      qWarning() << errorMsg;
      return false;
   }

   return true;
}

/*---------------------------------------------------------------------------
* alle Datensaetze lesen
*---------------------------------------------------------------------------*/
void DumpFile::readDatasets(DataMap *data)
{
   QVarLengthArray<qint32> rawValues;
   QTextStream strm(&bzcat);
   bool firstLine = true;
   QString line = strm.readLine();
   quint32 lineCount = 1;

   qreal heatEnergy = 0.0;
   qreal elEnergy1 = 0.0;
   qreal elEnergy2 = 0.0;
   quint32 lastTS = 0;
   qreal lastVD1 = 0.0;

   while(!line.isNull())
   {
      // Zeile lesen und decodieren
      QStringList fields = line.split( ',', QString::SkipEmptyParts);

      // Groesse des Arrays bestimmen
      if(firstLine)
      {
         rawValues.resize(fields.size());
         firstLine = false;
      }

      for( int i=0; i<fields.size(); ++i)
      {
         QStringList l = fields.at(i).split( '=', QString::SkipEmptyParts);
         if( l.at(0).startsWith("elt"))
         {
            // EltMon fields
            QString fname = l.at(0);
            quint16 pos = rawValues.size() - fname.right(1).toInt();
            rawValues[pos] = l.at(1).toInt(0,10);
         } else {
            // Lxt2Mon fields
            rawValues[l.at(0).toInt()] = l.at(1).toInt(0,10);
         }
      }

      // Daten auslesen und speichern
      DataFieldValues values(m_fieldCount); // Werte-Array
      for( int j=0; j<m_fieldCount; j++) values[j] = 0.0; // initiale Werte
      quint32 ts = rawValues[134];

      values[0 ] = rawValues[39]; // HUP
      values[1 ] = rawValues[47]; // ZUP
      values[2 ] = rawValues[38]; // BUP
      values[3 ] = rawValues[49]; // ZW2
      values[4 ] = rawValues[40]; // MA1
      values[5 ] = rawValues[41]; // MZ1
      values[6 ] = rawValues[46]; // ZIP
      values[7 ] = rawValues[44]; // VD1
      values[8 ] = rawValues[45]; // VD2
      values[9 ] = rawValues[42]; // VENT
      values[10] = rawValues[37]; // AV
      values[11] = rawValues[43]; // VBS
      values[12] = rawValues[48]; // ZW1
      values[13] = rawValues[32] ? 0 : 1; // HD
      values[14] = rawValues[34] ? 0 : 1; // ND
      values[15] = rawValues[33] ? 0 : 1; // MOT
      values[16] = rawValues[29] ? 0 : 1; // ASD
      values[17] = rawValues[31] ? 0 : 1; // EVU
      values[18] = rawValues[21]/10.0; // TFB1
      values[19] = rawValues[17]/10.0; // TBW
      values[20] = rawValues[15]/10.0; // TA
      values[21] = rawValues[13]/10.0; // TRLext
      values[22] = rawValues[11]/10.0; // TRL
      values[23] = rawValues[10]/10.0; // TVL
      values[24] = rawValues[14]/10.0; // THG
      values[25] = rawValues[20]/10.0; // TWQaus
      values[26] = rawValues[19]/10.0; // TWQein
      values[27] = rawValues[12]/10.0; // TRLsoll
      values[28] = rawValues[22]/10.0; // TMK1soll
      values[29] = rawValues[0]; // AI1DIV
      values[30] = rawValues[53]; // SUP
      values[31] = rawValues[51]; // FUP2
      values[32] = rawValues[55]; // MA2
      values[33] = rawValues[54]; // MZ2
      values[34] = rawValues[139]; // MA3
      values[35] = rawValues[138]; // MZ3
      values[36] = rawValues[140]; // FUP3
      values[37] = rawValues[50]; // ZW3
      values[38] = rawValues[52]; // SLP
      values[39] = rawValues[156]/100.0; // AO1
      values[40] = rawValues[157]/100.0; // AO2
      values[41] = rawValues[36] ? 0 : 1; // SWT
      values[42] = rawValues[147]/100.0; // AI1
      values[43] = rawValues[155]/60.0; // DF (Umrechnung l/h in l/min)

      values[65] = rawValues[rawValues.size()-1]/1.0; // Pe1
      values[66] = rawValues[rawValues.size()-2]/1.0; // Pe2

      //
      // berechnete Felder
      //

      // Durchfluss Heizkreis
      // Werte fuer Durchflussmesser Grundfoss VFS 5-100
      const quint8 posAI1 = 42;
      if( values[posAI1] < 0.5) values[57] = 0.0;
      else
         values[57] = qRound( (values[posAI1] * 32.0 - 12.0) * 2) / 2.0 + 1.5;

      // Spreizung Heizkreis
      const quint8 posHUP = 0;
      const quint8 posTRL = 22;
      const quint8 posTVL = 23;
      if(values[posHUP]) values[44] = values[posTVL] - values[posTRL];
      else values[44] = 0;

      // Spreizung Waermequelle
      const quint8 posVBS = 11;
      const quint8 posTWQein = 26;
      const quint8 posTWQaus = 25;
      if(values[posVBS]) values[45] = values[posTWQein] - values[posTWQaus];
      else values[45] = 0;

      // thermische Leistung
      // Qth = Durchfluss[l/min] * Spreizung[K] / 60 * c[kJ/kg] * Dichte[kg/l]
      //   c(Wasser) = 4.18kJ/kg bei 30 Grad C
      //   Dichte = 1.0044^-1 kg/l bei 30 Grad C
      const quint8 posDF = 43;
      const quint8 posSpHz = 44;
      values[46] = qRound( values[posDF]*values[posSpHz]/60.0 * 4.18*995.6 * 100)/100.0;

      //
      // Werte, die nur das Web-Interface liefert
      //
      values[47] = qRound(rawValues[56]/360.0)/10.0; // Betriebsstunden VD1
      values[48] = qRound(rawValues[63]/360.0)/10.0; // Betriebsstunden WP
      values[49] = qRound(rawValues[64]/360.0)/10.0; // Betriebsstunden Hz
      values[50] = qRound(rawValues[65]/360.0)/10.0; // Betriebsstunden BW
      values[51] = qRound(rawValues[66]/360.0)/10.0; // Betriebsstunden Kuehlung
      values[52] = rawValues[57]; // Impulse VD1
      values[54] = rawValues[151]/10.0; // WMZ Heizung
      values[55] = rawValues[152]/10.0; // WMZ BW
      values[53] = values[54] + values[55]; // WMZ gesamt
      values[56] = rawValues[16]/10.0; // TAm

      //
      // Temperaturen der Comfortplatine
      //
      //values[57] = rawValues[27]/10.0;  // TSS
      values[58] = rawValues[26]/10.0;  // TSK
      values[59] = rawValues[24]/10.0;  // TFB2
      values[60] = rawValues[137]/10.0; // TFB3
      values[61] = rawValues[28]/10.0;  // TEE
      values[62] = rawValues[25]/10.0;  // TMK2soll
      values[63] = rawValues[136]/10.0; // TMK3soll

      //
      // Berechnung Waermemenge
      //
      const quint8 posVD1 = 7;
      const quint8 posQth = 46;
      if( lastVD1==0.0 && values[posVD1]==1) heatEnergy = 0.0;
      if(ts-lastTS > MISSING_DATA_GAP)
         heatEnergy = 0.0;
      else
         heatEnergy += values[posQth]/1000.0 * (ts-lastTS) / 3600.0;
      values[64] = heatEnergy;

      //
      // Berechnung Arbeitszahl
      //
      const quint8 posPe1 = 65;
      const quint8 posPe2 = 66;
      // - aufpassen, dass Pe != 0.0
      // - Arbeitszahl nur berechnen, wenn Verdichter laeuft
      // - unrealistische Arbeitszahlen zu Beginn/Ende eines Kompressorstarts
      //   ausblenden
      if( values[posVD1]==0 || values[posPe1]==0.0) values[67] = 0.0;
      else {
         values[67] = values[posQth] / values[posPe1];
         if( values[67] > 10 || values[67] < 0) values[67] = 0.0;
      }
      if( values[posVD1]==0 || values[posPe2] == 0.0) values[68] = 0.0;
      else {
         values[68] = values[posQth] / (values[posPe1] + values[posPe2]);
         if( values[68] > 10 || values[68] < 0) values[68] = 0.0;
      }

      //
      // Berechnung Elektroenergie
      //
      if( lastVD1==0.0 && values[posVD1]==1) {
         elEnergy1 = 0.0;
         elEnergy2 = 0.0;
      }
      if(ts-lastTS > MISSING_DATA_GAP) {
         elEnergy1 = 0.0;
         elEnergy2 = 0.0;
      } else {
         elEnergy1 += values[posPe1]/1000.0 * (ts-lastTS) / 3600.0;
         elEnergy2 += values[posPe2]/1000.0 * (ts-lastTS) / 3600.0;
      }
      values[69] = elEnergy1;
      values[70] = elEnergy2;

      lastTS = ts;
      lastVD1 = values[posVD1];

      // insert dataset into data map
      data->insert( ts, values);

      bzcat.waitForReadyRead(); // auf Daten von bzcat warten
      line = strm.readLine(); // naechste Zeile
      lineCount++;
   }
}
