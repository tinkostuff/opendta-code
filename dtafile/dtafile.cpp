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
#include <QFileInfo>

#include "dtafile/dtafile.h"

/*---------------------------------------------------------------------------
* Construktor
*---------------------------------------------------------------------------*/
DtaFile::DtaFile(QString fileName, QObject *parent) :
    DataFile(fileName,parent)
{
   this->m_dtaFile = NULL;
   this->m_dtaVersion = 0;
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
      errorMsg = QString(tr("FEHLER: Datei '%1' nicht gefunden!")).arg(m_fileName);
      qWarning() << errorMsg;
      return false;
   }

   // Datei oeffnen
   m_dtaFile = new QFile(m_fileName);
   m_dtaFile->open(QIODevice::ReadOnly);

   // Stream erstellen
   m_dtaStream.setDevice(m_dtaFile);
   m_dtaStream.setByteOrder(QDataStream::LittleEndian);

   // Dateikopf lesen
   quint32 header[2];
   m_dtaStream >> header[0];
   m_dtaStream >> header[1];

   // Kopf pruefen
   if( (header[0] != DTA1_HEADER_VALUE)
       && (header[0] != DTA2_HEADER_VALUE)
       && (header[0] != DTA3_HEADER_VALUE))
   {
      QFileInfo fi(m_fileName);
      errorMsg = QString(tr("FEHLER %2: DTA-Version %1 wird z.Z. noch nicht unterstuetzt!\nBei Interesse bitte die DTA-Datei (incl. CSV-Datei) an opendta@gmx.de schicken."))
                        .arg(header[0])
                        .arg(fi.fileName());
      qWarning() << errorMsg;
      return false;
   }

   // Version der DTA-Datei festhalten
   if( header[0] <= DTA1_HEADER_VALUE) m_dtaVersion = 1;
   else if( header[0] == DTA2_HEADER_VALUE) m_dtaVersion = 2;
   else if( header[0] >= DTA3_HEADER_VALUE) m_dtaVersion = 3;

   // Groesse der Datei ueberpruefen
   if( m_dtaVersion == 1) {
      if( (m_dtaFile->size() - DTA_HEADER_LENGTH) % DTA1_DATASET_LENGTH != 0) {
         errorMsg = QString(tr("FEHLER %1: Unerwartete Dateigroesse!"))
                            .arg(m_fileName);
         qWarning() << errorMsg;
         return false;
      }
      m_dsCount = (m_dtaFile->size() - DTA_HEADER_LENGTH) / DTA1_DATASET_LENGTH;
   }

   // Unter-Version ueberpruefen
   if( m_dtaVersion==2) {
      if( header[1] < DTA2_HEADER_VALUE_SUBVERSION) m_dtaSubVersion = 1;
      else m_dtaSubVersion = 2;
   }
   if( m_dtaVersion==3) {
      if( header[1] < DTA3_HEADER_VALUE_SUBVERSION) m_dtaSubVersion = 1;
      else m_dtaSubVersion = 2;
   }

   // Anzahl der Datensaetze lesen
   if( m_dtaVersion == 3)
   {
      m_dtaStream >> m_dsCount;
   }
   qDebug() << m_dtaVersion << m_dtaSubVersion << m_dsCount;

   // DTA-Version als String
   if ((m_dtaVersion==2) || (m_dtaVersion==3))
      m_dtaVersionStr = QString("DTA %1.%2").arg(header[0]).arg(header[1]);
   else
      m_dtaVersionStr = QString("DTA %1").arg(header[0]);

   return true;
}

/*---------------------------------------------------------------------------
* alle Datensaetze der Datei lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDatasets(DataMap *data)
{
   // DTA in Abhaengigkeit von Version lesen
   if( m_dtaVersion == 1) readDTA1(data);
   else if( m_dtaVersion == 2) readDTA2(data);
   else if( m_dtaVersion == 3) readDTA3(data);
   else {
      qWarning() << QString(tr("FEHLER: unbekannt DTA Version (%1)!").arg(m_dtaVersion));
   }
}

/*---------------------------------------------------------------------------
* DAT Version 1.x lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA1(DataMap *data)
{
   quint16 value;

   quint32 lastTS = 0;
   qreal heatEnergy = 0.0;
   qreal lastVD1 = 0.0;

   // Puffer zum Dummy-Lesen (ist schneller als skipRawData)
   char buffer[50];

   // jeden Datensatz lesen
   for( int i=0; i<m_dsCount; i++)
   {
      DataFieldValues values(m_fieldCount); // Werte-Array
      for( int j=0; j<m_fieldCount; j++) values[j] = 0.0; // initiale Werte
      quint32 ts; // Zeitstempel

      // Felder einlesen
      m_dtaStream >> ts;                                                                      // [0  :3  ] - Datum
      m_dtaStream.readRawData( buffer, 4);                                                    // [4  :7  ]
      m_dtaStream >> value; for( int j=0; j<=12; j++) values[0+j]=calcBitData(value,j);       // [8  :9  ] - Status Ausgaenge
      m_dtaStream.readRawData( buffer, 34);                                                   // [10 :43 ]
      m_dtaStream >> value; for( int j=0; j<=4; j++) values[13+j]=calcBitDataInv(value,j);    // [44 :45 ] - Status Eingaenge
      values[17] = values[17]==1.0 ? 0.0 : 1.0; // EVU invertieren
      m_dtaStream.readRawData( buffer, 6);                                                    // [46 :51 ]
      m_dtaStream >> value; values[18] = calcLUTData( value, LUT[0]);                         // [52 :53 ] - TFB1
      m_dtaStream >> value; values[19] = calcLUTData( value, LUT[0]);                         // [54 :55 ] - TBW
      m_dtaStream >> value; values[20] = calcLUTData( value, LUT[1]);                         // [56 :57 ] - TA
      m_dtaStream >> value; values[21] = calcLUTData( value, LUT[0]);                         // [58 :59 ] - TRLext
      m_dtaStream >> value; values[22] = calcLUTData( value, LUT[0]);                         // [60 :61 ] - TRL
      m_dtaStream >> value; values[23] = calcLUTData( value, LUT[0]);                         // [62 :63 ] - TVL
      m_dtaStream >> value; values[24] = calcLUTData( value, LUT[2]);                         // [64 :65 ] - THG
      m_dtaStream >> value; values[25] = calcLUTData( value, LUT[1]);                         // [66 :67 ] - TWQaus
      m_dtaStream.readRawData( buffer, 2);                                                    // [68 :69 ]
      m_dtaStream >> value; values[26] = calcLUTData( value, LUT[1]);                         // [70 :71 ] - TWQein
      m_dtaStream.readRawData( buffer, 8);                                                    // [72 :79 ]
      m_dtaStream >> value; values[27] = calcLinearData( value, 0.1, 0.0, 10);                // [80 :81 ] - TRLsoll
      m_dtaStream.readRawData( buffer, 2);                                                    // [82 :83 ]
      m_dtaStream >> value; values[28] = calcLinearData( value, 0.1, 0.0, 10);                // [84 :85 ] - TMK1soll
      m_dtaStream.readRawData( buffer, 46);                                                   // [86 :131]
      m_dtaStream >> value; for( int j=6; j<=15; j++) values[29+j-6]=calcBitData(value,j);    // [132:133] - Status Ausgaenge ComfortPlatine
      m_dtaStream.readRawData( buffer, 2);                                                    // [134:135]
      m_dtaStream >> value; values[39] = calcLinearData( value, 0.002619, 0.0, 100);          // [136:137] - AO1
      m_dtaStream >> value; values[40] = calcLinearData( value, 0.002619, 0.0, 100);          // [138:139] - AO2
      m_dtaStream >> value; values[41] = calcBitDataInv(value, 4);                            // [140:141] - Status Eingaenge ComfortPlatine
      m_dtaStream.readRawData( buffer, 2);                                                    // [142:143]
      m_dtaStream >> value; values[57] = calcLUTData( value, LUT[3]);                         // [144:145] - TSS
      m_dtaStream >> value; values[58] = calcLUTData( value, LUT[3]);                         // [146:147] - TSK
      m_dtaStream >> value; values[59] = calcLUTData( value, LUT[4]);                         // [148:149] - TFB2
      m_dtaStream >> value; values[60] = calcLUTData( value, LUT[4]);                         // [150:151] - TFB3
      m_dtaStream >> value; values[61] = calcLUTData( value, LUT[4]);                         // [152:153] - TEE
      m_dtaStream.readRawData( buffer, 4);                                                    // [154:157]
      m_dtaStream >> value; values[42] = calcLinearData( value, 0.003631, 0.0, 100);          // [158:159] - AI1
      m_dtaStream >> value; values[62] = calcLinearData( value, 0.1, 0.0, 10);                // [160:161] - TMK2soll
      m_dtaStream.readRawData( buffer, 2);                                                    // [162:163]
      m_dtaStream >> value; values[63] = calcLinearData( value, 0.1, 0.0, 10);                // [164:165] - TMK3soll
      m_dtaStream.readRawData( buffer, 2);                                                    // [166:167]

      //
      // berechnete Felder
      //

      // Durchfluss Heizkreis
      // Werte fuer Durchflussmesser Grundfoss VFS 5-100
      // der Sensor hat "nur" 0.5l/min Aufloesung
      // alle Werte unter 0,5V sind als Durchfluss=0.0l/min zu werten
      const quint8 posAI1 = 42;
      if(values[posAI1]<0.5) values[43] = 0.0;
      else
         values[43] = calcLinearData( 1, values[posAI1] * 95.0/3.0, -65.0/6.0, 2);

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
      values[46] = qRound( values[posDF]*values[posSpHz]/60.0 * 4.18*0.9956 * 100)/100.0;

      //
      // Berechnung Waermemenge
      //
      const quint8 posVD1 = 7;
      const quint8 posQth = 46;
      if( lastVD1==0.0 && values[posVD1]==1) heatEnergy = 0.0;
      if(ts-lastTS > MISSING_DATA_GAP) // Luecke gefunden
         heatEnergy = 0.0;
      else
         heatEnergy += values[posQth] * (ts-lastTS) / 3600.0;
      values[64] = heatEnergy;
      lastTS = ts;
      lastVD1 = values[posVD1];

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

   if(info.delta==20)
   {
      qDebug() << value << idx << x1 << y1 << x2 << y2;
   }

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
      return 0.0;
   else
      return 1.0;
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
const DtaLUTInfo DtaFile::LUT[5] = {
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
          -220, -237, -255, -273, -279, -279},
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
          -372, -384, -396, -408, -411, -411},
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
          -240, -300, -378, -411, -411},
      0,  // offset
      10, // delta
      10  // precision
   },

	// LUT for TSS, TSK
	{
		{1550, 1550, 1550, 1550, 1550, 1550, 1550, 1537, 1468, 1409, 1357, 
		    1311, 1268, 1229, 1193, 1160, 1130, 1100, 1074, 1048, 1024, 1000, 
			 978, 956, 936, 916, 896, 879, 861, 843, 827, 811, 795, 780, 765, 
			 750, 737, 723, 709, 695, 681, 668, 655, 646, 630, 617, 605, 593, 
			 581, 570, 558, 547, 535, 524, 513, 502, 490, 479, 467, 456, 444, 
			 433, 421, 410, 398, 387, 376, 365, 354, 343, 331, 319, 308, 296, 
			 283, 270, 257, 244, 231, 218, 204, 191, 177, 162, 148, 133, 117, 
			 101, 84, 67, 48, 29, 9, -12, -35, -60, -87, -117, -152, -189, 
			 -235, -292, -369, -411, -411},
		0,  // offset
      40, // delta
		10, // precision
	},

	// LUT for TFB2, TFB3, TEE
	{
		{1550, 1550, 1550, 1438, 1305, 1205, 1128, 1063, 1007, 959, 916, 878, 
			 843, 811, 783, 756, 732, 708, 685, 664, 647, 625, 607, 590, 574, 
			 558, 543, 529, 515, 501, 487, 474, 461, 448, 436, 424, 412, 401, 
			 390, 379, 368, 358, 348, 338, 328, 318, 308, 299, 289, 279, 269, 
			 260, 250, 241, 232, 223, 214, 205, 196, 187, 178, 170, 161, 152, 
			 144, 135, 127, 119, 110, 101, 93, 84, 75, 66, 57, 48, 39, 30, 21, 
			 12, 2, -7, -16, -25, -36, -47, -57, -66, -77, -89, -101, -111, 
			 -123, -135, -149, -161, -174, -189, -204, -219, -235, -254, -271, 
			 -279, -279},
		0,  // offset
      40, // delta
		10, // precision
	}
};

/*---------------------------------------------------------------------------
* DAT Version 2.61 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA2(DataMap *data)
{
   quint16 dsLenght = DTA2_DATASET_LENGTH1;
   if( m_dtaSubVersion == 2) dsLenght = DTA2_DATASET_LENGTH2;

   qint32 ds[dsLenght-1]; // Werte des Datensatzes
   quint32 lastTS = 0;
   qreal heatEnergy = 0.0;
   qreal lastVD1 = 0.0;

   m_dsCount = 0;
   while(1)
   {
      // Zeitstempel einlesen
      quint32 ts; // Zeitstempel
      m_dtaStream >> ts; // [0  :3  ] - Datum

      // Ende der Datei?
      if( ts == 0) break;

      // jedes Feld lesen
      for( int j=0; j<dsLenght-1; j++)
      {
         quint8 type;
         m_dtaStream >> type;

         switch(type)
         {
         case 0:
            // positive byte
            quint8 tmp8u;
            m_dtaStream >> tmp8u;
            ds[j] = tmp8u;
            break;
         case 1:
            // positive short
            qint16 tmp16;
            m_dtaStream >> tmp16;
            ds[j] = tmp16;
            break;
         case 4:
            // negative byte
            quint8 tmp8;
            m_dtaStream >> tmp8;
            ds[j] = -tmp8;
            break;
         case 5:
            // negative short
            quint16 tmp16u;
            m_dtaStream >> tmp16u;
            ds[j] = -tmp16u;
            break;
         default:
            qWarning() << "type" << type << "not supported";
            ds[j] = 0;
         }
      }

      // Werte zuordnen
      m_dsCount++;
      DataFieldValues values(m_fieldCount); // Werte-Array
      for( int j=0; j<m_fieldCount; j++) values[j] = 0.0; // initial values

      // Datensatz konvertieren
      for( int j=0; j<=12; j++) values[0+j ]=calcBitData(ds[12],j);       // Status Ausgaenge
      if( m_dtaSubVersion >= 2)
         for( int j=0; j<=4;  j++) values[13+j]=calcBitDataInv(ds[13],j); // Status Eingaenge
      else
         for( int j=0; j<=4;  j++) values[13+j]=calcBitData(ds[13],j); // Status Eingaenge
      values[17] = values[17]==1.0 ? 0.0 : 1.0; // EVU invertieren
      values[19] = ds[ 5]/10.0; // TBW
      values[20] = ds[ 7]/10.0; // TA
      values[21] = ds[ 8]/10.0; // TRLext
      values[22] = ds[ 1]/10.0; // TRL
      values[23] = ds[ 0]/10.0; // TVL
      values[24] = ds[ 4]/10.0; // THG
      values[25] = ds[ 3]/10.0; // TWQaus
      values[26] = ds[ 2]/10.0; // TWQein
      values[27] = ds[ 9]/10.0; // TRLsoll
      values[43] = ds[23]/60.0; // DF (Umrechnung l/h in l/min)

      // Felder, welche nur in Unterversion1 vorhanden sind
      if( m_dtaSubVersion == 1) {
        values[71] = ds[36]/10.0; // UEHZ
        values[72] = ds[37]/10.0; // UEHZsoll
        values[73] = ds[28]/10.0; // Asg.VDi
        values[74] = ds[29]/10.0; // Asg.VDa
        values[75] = ds[30]/10.0; // VDHz
      }

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
      values[46] = qRound( values[posDF]*values[posSpHz]/60.0 * 4.18*0.9956 * 100)/100.0;

      //
      // Berechnung Waermemenge
      //
      const quint8 posVD1 = 7;
      const quint8 posQth = 46;
      if( lastVD1==0.0 && values[posVD1]==1) heatEnergy = 0.0;
      if(ts-lastTS > MISSING_DATA_GAP) // Luecke gefunden
         heatEnergy = 0.0;
      else
         heatEnergy += values[posQth] * (ts-lastTS) / 3600.0;
      values[64] = heatEnergy;
      lastTS = ts;
      lastVD1 = values[posVD1];

      // Datensatz einfuegen
      data->insert( ts, values);
   }
}

/*---------------------------------------------------------------------------
* DAT Version 2.63 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA3(DataMap *data)
{
   quint16 value;
   qint16 valueS;

   quint32 lastTS = 0;
   qreal heatEnergy = 0.0;
   qreal lastVD1 = 0.0;

   // Puffer zum Dummy-Lesen (ist schneller als skipRawData)
   char buffer[50];

   // jeden Datensatz lesen
   for( int i=0; i<m_dsCount; i++)
   {
      DataFieldValues values(m_fieldCount); // Werte-Array
      for( int j=0; j<m_fieldCount; j++) values[j] = 0.0; // initiale Werte
      quint32 ts; // Zeitstempel

		// Felder einlesen
      m_dtaStream >> ts;                                                                      // [0 :3 ] Datum und Uhrzeit in Sekunden von 1.1.1970 (Unixzeit)
      m_dtaStream >> valueS; values[23] = valueS/10.0;                                        // [4 :5 ] TVL 
      m_dtaStream >> valueS; values[22] = valueS/10.0;                                        // [6 :7 ] TRL
      m_dtaStream >> valueS; values[26] = valueS/10.0;                                        // [8 :9 ] TWQein 
      m_dtaStream >> valueS; values[25] = valueS/10.0;                                        // [10:11] TWQaus
      m_dtaStream >> valueS; values[24] = valueS/10.0;                                        // [12:13] THG
      m_dtaStream >> valueS; values[19] = valueS/10.0;                                        // [14:15] TBW 
      m_dtaStream.readRawData( buffer, 2);                                                    // [16:17] unbekannt 
      m_dtaStream >> valueS; values[20] = valueS/10.0;                                        // [18:19] TA
      m_dtaStream.readRawData( buffer, 2);                                                    // [20:21] unbekannt 
      m_dtaStream >> valueS; values[27] = valueS/10.0;                                        // [22:23] TRLsoll 
      m_dtaStream.readRawData( buffer, 2);                                                    // [24:25] unbekannt 
      m_dtaStream >> value; for( int j=0; j<=4; j++) values[13+j]=calcBitData(value,j);       // [26:27] Eingaenge
      values[17] = values[17]==1.0 ? 0.0 : 1.0; // EVU invertieren
      m_dtaStream >> value; for( int j=0; j<=12; j++) values[0+j]=calcBitData(value,j);       // [28:29] Ausgaenge 
      m_dtaStream.readRawData( buffer, 20);                                                   // [30:49] unbekannt 
      m_dtaStream >> valueS; values[43] = valueS/60.0;                                        // [50:51] DF                    
      m_dtaStream >> valueS; values[39] = valueS/1000.0;                                      // [52:53] AO1

      if (m_dtaSubVersion > 1) {
         m_dtaStream >> valueS; values[40] = valueS/1000.0;                                   // [54:55] AO2
         m_dtaStream.readRawData( buffer, 2);                                                 // [56:57] unbekannt
         m_dtaStream >> valueS; values[73] = valueS/10.0;                                     // [58:59] Ansaug Verdichter
         m_dtaStream >> valueS; values[74] = valueS/10.0;                                     // [60:61] Ansaug Verdampfer
         m_dtaStream >> valueS; values[75] = valueS/10.0;                                     // [62:63] VD Heizung
         m_dtaStream.readRawData( buffer, 10);                                                // [64:73] unbekannt T(VD*TA)
         m_dtaStream >> valueS; values[71] = valueS/10.0;                                     // [74:75] Ueberhitzung
         m_dtaStream >> valueS; values[72] = valueS/10.0;                                     // [76:77] Ueberhiztung Sollwert
         m_dtaStream.readRawData( buffer, 2);                                                 // [78:79] unbekannt
      }

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
      values[46] = qRound( values[posDF]*values[posSpHz]/60.0 * 4.18*0.9956 * 100)/100.0;

      //
      // Berechnung Waermemenge
      //
      const quint8 posVD1 = 7;
      const quint8 posQth = 46;
      if( lastVD1==0.0 && values[posVD1]==1) heatEnergy = 0.0;
      if(ts-lastTS > MISSING_DATA_GAP) // Luecke gefunden
         heatEnergy = 0.0;
      else
         heatEnergy += values[posQth] * (ts-lastTS) / 3600.0;
      values[64] = heatEnergy;
      lastTS = ts;
      lastVD1 = values[posVD1];

      // Datensatz in Map einfuegen
      data->insert( ts, values);
   }
}
