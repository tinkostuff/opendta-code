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
#include <QtGlobal>
#include <QtDebug>

#include <qmath.h>
#include <QStringList>

#include "dtafile/dtafile.h"

/*---------------------------------------------------------------------------
* Construktor
*---------------------------------------------------------------------------*/
DtaFile::DtaFile(QString fileName, QObject *parent) :
    DataFile(fileName,parent)
{
   this->m_dtaFile = NULL;
}

/*---------------------------------------------------------------------------
* Destructor
*---------------------------------------------------------------------------*/
DtaFile::~DtaFile()
{
   // Datei-Handler loeschen
   if(m_dtaFile != NULL) delete m_dtaFile;
}

/*---------------------------------------------------------------------------
* DTA-Datei oeffnen
*---------------------------------------------------------------------------*/
bool DtaFile::open()
{
   // gibt es die Daten
   if( !QFile::exists(m_fileName))
   {
      qWarning() << QString(tr("FEHLER: Datei '%1' nicht gefunden!")).arg(m_fileName);
      return false;
   }

   // Datei oeffnen
   m_dtaFile = new QFile(m_fileName);
   m_dtaFile->open(QIODevice::ReadOnly);

   // Groesse der Datei ueberpruefen
   qint16 dsCount = (m_dtaFile->size() - DTA_HEADER_LENGTH) / DTA_DATASET_LENGTH;
   if( dsCount != DTA_DATASET_COUNT) {
      qWarning() << QString(tr("FEHLER %3: Anzahl der Datensaetze (%1) weicht vom erwarteten Wert (%2) ab!"))
                        .arg(dsCount)
                        .arg(DTA_DATASET_COUNT)
                        .arg(m_fileName);
      return false;
   }

   // Stream erstellen
   m_dtaStream.setDevice(m_dtaFile);
   m_dtaStream.setByteOrder(QDataStream::LittleEndian);

   // Dateikoepf lesen und verwerfen (damit der Dateizeiger auf dem
   // ersten Datensatz steht
   m_dtaStream.skipRawData(DTA_HEADER_LENGTH);

   return true;
}

/*---------------------------------------------------------------------------
* alle Datensaetze der Datei lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDatasets(DataMap *data)
{
   quint16 value;

   // Puffer zum Dummy-Lesen (ist schneller als skipRawData)
   char buffer[50];

   // jeden Datensatz lesen
   for( int i=0; i<DTA_DATASET_COUNT; i++)
   {
      quint8 pos = 0; // Position im Array
      DataFieldValues values(m_fieldCount); // Werte-Array
      quint32 ts; // Zeitstempel

      // Felder einlesen
      m_dtaStream >> ts;                                                                              // [0  :3  ] - Datum
      m_dtaStream.readRawData( buffer, 4);                                                            // [4  :7  ]
      m_dtaStream >> value; for( int j=0; j<=12; j++) { values[pos]=calcBitData(value,j); pos++; }    // [8  :9  ] - Status Ausgaenge
      m_dtaStream.readRawData( buffer, 34);                                                           // [10 :43 ]
      m_dtaStream >> value; for( int j=0; j<=4; j++) { values[pos]=calcBitDataInv(value,j); pos++; }  // [44 :45 ] - Status Eingaenge
      m_dtaStream.readRawData( buffer, 6);                                                            // [46 :51 ]
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[0]); pos++;                         // [52 :53 ] - TFB1
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[0]); pos++;                         // [54 :55 ] - TBW
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[1]); pos++;                         // [56 :57 ] - TA
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[0]); pos++;                         // [58 :59 ] - TRLext
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[0]); pos++;                         // [60 :61 ] - TRL
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[0]); pos++;                         // [62 :63 ] - TVL
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[2]); pos++;                         // [64 :65 ] - THG
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[1]); pos++;                         // [66 :67 ] - TWQaus
      m_dtaStream.readRawData( buffer, 2);                                                            // [68 :69 ]
      m_dtaStream >> value; values[pos] = calcLUTData( value, LUT[1]); pos++;                         // [70 :71 ] - TWQein
      m_dtaStream.readRawData( buffer, 8);                                                            // [72 :79 ]
      m_dtaStream >> value; values[pos] = calcLinearData( value, 0.1, 0.0, 10); pos++;                // [80 :81 ] - TRLsoll
      m_dtaStream.readRawData( buffer, 2);                                                            // [82 :83 ]
      m_dtaStream >> value; values[pos] = calcLinearData( value, 0.1, 0.0, 10); pos++;                // [84 :85 ] - TMK1soll
      m_dtaStream.readRawData( buffer, 46);                                                           // [86 :131]
      m_dtaStream >> value; for( int j=6; j<=15; j++) { values[pos]=calcBitData(value,j); pos++; }    // [132:133] - Status Ausgaenge ComfortPlatine
      m_dtaStream.readRawData( buffer, 2);                                                            // [134:135]
      m_dtaStream >> value; values[pos] = calcLinearData( value, 0.002619, 0.0, 100); pos++;          // [136:137] - AO1
      m_dtaStream >> value; values[pos] = calcLinearData( value, 0.002619, 0.0, 100); pos++;          // [138:139] - AO2
      m_dtaStream >> value; values[pos] = calcBitDataInv(value, 4); pos++;                            // [140:141] - Status Eingaenge ComfortPlatine
      m_dtaStream.readRawData( buffer, 16);                                                           // [142:157]
      m_dtaStream >> value; values[pos] = calcLinearData( value, 0.003631, 0.0, 100); pos++;          // [158:159] - AI1
      m_dtaStream.readRawData( buffer, 8);                                                            // [160:167]

      //
      // berechnete Felder
      //

      // Durchfluss Heizkreis
      // Werte fuer Durchflussmesser Grundfoss VFS 5-100
      const quint8 posAI1 = 42;
      values[pos] = calcLinearData( 1, values[posAI1] * 32.0, -12.0, 100); pos++;

      // Spreizung Heizkreis
      const quint8 posHUP = 0;
      const quint8 posTRL = 22;
      const quint8 posTVL = 23;
      if(values[posHUP]) values[pos] = values[posTVL] - values[posTRL];
      else values[pos] = 0;
      pos ++;

      // Spreizung Waermequelle
      const quint8 posVBS = 11;
      const quint8 posTWQein = 26;
      const quint8 posTWQaus = 25;
      if(values[posVBS]) values[pos] = values[posTWQein] - values[posTWQaus];
      else values[pos] = 0;
      pos ++;

      // thermische Leistung
      // Qth = Durchfluss[l/min] * Spreizung[K] / 60 * c[kJ/kg] * Dichte[kg/l]
      //   c(Wasser) = 4.18kJ/kg bei 30 Grad C
      //   Dichte = 1.0044^-1 kg/l bei 30 Grad C
      const quint8 posDF = 43;
      const quint8 posSpHz = 44;
      values[pos] = qRound( values[posDF]*values[posSpHz]/60.0 * 4.18*0.9956 * 10)/10.0;
      pos++;

      // Datensatz in Map einfuegen
      data->insert( ts, values);
   }
}

/*---------------------------------------------------------------------------
* Feld mit Wertetabelle berechnen
*---------------------------------------------------------------------------*/
qreal DtaFile::calcLUTData(const quint16 &value, const DtaLUTInfo &info)
{
   // Position in Tabelle finden
    quint16 idx = (value - info.offset) / info.delta;
    quint32 size = sizeof(info.data)/sizeof(qint16);
    if( idx > (size-2)) idx = size-2;

    // Gerade fuer lineare Approximation ermitteln
    qint16 x1 = idx * info.delta + info.offset;
    qint16 x2 = (idx+1) * info.delta + info.offset;
    qint16 y1 = info.data[idx];
    qint16 y2 = info.data[idx+1];

    qreal m = qreal(y2-y1)/qreal(x2-x1);
    qreal n = y1 - m*x1;

    // Wert ausrechnen
    qreal res = m*value + n;
    return qRound(res)/qreal(info.precision);
}

/*---------------------------------------------------------------------------
* Bit innerhalb eines Feldes ermitteln
*---------------------------------------------------------------------------*/
qreal DtaFile::calcBitData(const quint16 &value, const quint8 &pos)
{
   return qreal((value >> pos) & 1);
}
qreal DtaFile::calcBitDataInv(const quint16 &value, const quint8 &pos)
{
   if( ((value >> pos) & 1) == 1)
      return 1.0;
   else
      return 0.0;
}

/*---------------------------------------------------------------------------
* Feld mit linearer Berechnung ermitteln
*---------------------------------------------------------------------------*/
qreal DtaFile::calcLinearData(const quint16 &value, const qreal &m, const qreal &n, const qint8 &precision)
{
   return qreal(qRound((value * m + n) * precision))/qreal(precision);
}

/*---------------------------------------------------------------------------
*----------------------------------------------------------------------------
* Initialisierung statischer Variablen
*---------------------------------------------------------------------------
*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
* Wertetabellen zur Umrechnung
*---------------------------------------------------------------------------*/
const DtaLUTInfo DtaFile::LUT[3] = {
   // LUT for TRL, TVL, TBW, TFB1, TRLext
   {
      {1550, 1550, 1550, 1438, 1305, 1205, 1128,
          1063, 1007, 959, 916, 878, 843, 811, 783, 756, 732, 708,
          685, 664, 647, 625, 607, 590, 574, 558, 543, 529, 515, 501,
          487, 474, 461, 448, 436, 424, 412, 401, 390, 379, 368, 358,
          348, 338, 328, 318, 308, 299, 289, 279, 269, 260, 250, 241,
          232, 223, 214, 205, 196, 187, 178, 170, 161, 152, 144,
          135, 127, 118, 109, 100, 92, 83, 74, 65, 56, 47, 38, 29,
          20, 11, 1, -7, -17, -26, -37, -48, -58, -68, -78, -90,
          -102, -112, -124, -137, -150, -162, -175, -190, -205,
          -220, -237, -255, -273},
      0,  // offset
      10, // delta
      10  // precision
   },

   // LUT for TWQein, TWQaus, TA
   {
      {1550, 1435, 1133, 971, 862, 781, 718,
          664, 618, 579, 545, 514, 486, 460, 435, 413, 392, 372, 354,
          337, 321, 306, 290, 276, 261, 248, 235, 223, 210, 199, 188,
          177, 166, 156, 146, 136, 127, 117, 108, 99, 90, 82, 73, 65,
          57, 48, 40, 32, 25, 17, 9, 1, -5, -12, -20, -27, -35, -42,
          -50, -57, -63, -70, -77, -85, -92, -100, -107, -113, -120,
          -127, -134, -142, -150, -156, -163, -169, -177, -184, -192,
          -200, -207, -214, -221, -229, -237, -246, -254, -261, -269,
          -277, -286, -296, -304, -313, -322, -331, -342, -352, -362,
          -372, -384, -396, -408},
      0,  // offset
      10, // delta
      10  // precision
   },

   // LUT for THG
   {
      {1550, 1550, 1550, 1550, 1550, 1550, 1550,
          1537, 1468, 1409, 1357, 1311, 1268, 1229, 1193, 1160, 1130,
          1100, 1074, 1048, 1024, 1000, 978, 956, 936, 916, 896, 879,
          861, 843, 827, 811, 795, 780, 765, 750, 737, 723, 709, 695,
          681, 668, 655, 646, 630, 617, 605, 593, 581, 570, 558, 547,
          535, 524, 513, 502, 490, 479, 467, 456, 444, 433, 421, 410,
          398, 387, 376, 364, 353, 341, 330, 318, 306, 294, 282, 269,
          256, 243, 230, 217, 203, 189, 175, 161, 146, 131, 116, 99,
          83, 65, 47, 27, 7, -14, -37, -62, -90, -120, -155, -194,
          -240, -300, -378},
      0,  // offset
      10, // delta
      10  // precision
   }
};


