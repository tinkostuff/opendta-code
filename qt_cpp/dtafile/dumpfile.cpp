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

#include <QFile>
#include <QVarLengthArray>
#include <QTextStream>

#include <QtGlobal>
#include <QDebug>

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
      qWarning() << QString(tr("FEHLER: Datei '%1' nicht gefunden!")).arg(m_fileName);
      return false;
   }

   // Process oeffnen
   bzcat.start("bzcat", QStringList() << m_fileName);
   bzcat.setReadChannel(QProcess::StandardOutput);
   if(!bzcat.waitForReadyRead())
   {
      qWarning() << QString(tr("FEHLER: 'bzcat' kann nicht gestarted werden!"));
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


   while(!line.isNull())
   {
      // Zeile lesen und decodieren
      QStringList fields = line.split( ',', QString::SkipEmptyParts);

      // Groesse des Arrays bestimmen
      if(firstLine)
      {
         quint32 max = fields.last().split('=',QString::SkipEmptyParts)[0].toInt();
         rawValues.resize(max+1);
         firstLine = false;
      }

      for( int i=0; i<fields.size(); ++i)
      {
         QStringList l = fields.at(i).split( '=', QString::SkipEmptyParts);
         rawValues[l.at(0).toInt()] = l.at(1).toInt(0,10);
      }

      // Daten auslesen und speichern
      DataFieldValues values(m_fieldCount); // Werte-Array
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
      values[39] = rawValues[156]/10.0; // AO1
      values[40] = rawValues[157]/10.0; // AO2
      values[41] = rawValues[36] ? 0 : 1; // SWT
      values[42] = rawValues[147]/10.0; // AI1
      values[43] = rawValues[155]/60.0; // DF (Umrechnung l/h in l/min)

      //
      // berechnete Felder
      //

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
      values[46] = qRound( values[posDF]*values[posSpHz]/60.0 * 4.18*0.9956 * 10)/10.0;

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

      // insert dataset into data map
      data->insert( ts, values);

      bzcat.waitForReadyRead(); // auf Daten von bzcat warten
      line = strm.readLine(); // naechste Zeile
      lineCount++;
   }
}
