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
#include <QtGlobal>
#include <QtDebug>

#include <qmath.h>
#include <QStringList>
#include <QFileInfo>
#include <QSettings>

#include "dtafile/dtafile.h"
#include "dtagui/config.h"

/*---------------------------------------------------------------------------
* Construktor
*---------------------------------------------------------------------------*/
DtaFile::DtaFile(QString fileName, QObject *parent) :
    DataFile(fileName,parent)
{
    this->m_dtaFile = NULL;
    this->m_dtaVersion = 0;
    this->m_dtaSubVersion = 0;

    // INI-Datei lesen
    QSettings cfg(
             QSettings::IniFormat,
             QSettings::UserScope,
             ORG_NAME,
             APP_NAME,
             this);
    m_ZUPasVD1 = cfg.value( "dtafile/ZUPasVD1", false).toBool();
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
   QFileInfo fi(m_fileName);
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
   QList<quint32> validHeaders;
   validHeaders << DTA8208 << DTA8209 << DTA9000 << DTA9001 << DTA9003;
   if( !validHeaders.contains(header[0]))
   {
      errorMsg = QString(tr("FEHLER %2: DTA-Version %1 wird z.Z. noch nicht unterstuetzt!\nBei Interesse bitte die DTA-Datei (incl. CSV-Datei) an opendta@gmx.de schicken."))
                        .arg(header[0])
                        .arg(fi.fileName());
      qWarning() << errorMsg;
      return false;
   }

   // Version der DTA-Datei festhalten
   m_dtaVersion = header[0];

   // Groesse der Datei ueberpruefen
   if( (m_dtaVersion==DTA8209) || (m_dtaVersion==DTA8208)) {
      // unterschiedliche Groesse fuer 0x2010 und 0x2011
      quint32 dsSize = DTA0_DATASET_LENGTH;
      if( header[0] == DTA8209)
         dsSize = DTA1_DATASET_LENGTH;

      if( (m_dtaFile->size() - DTA_HEADER_LENGTH) % dsSize != 0) {
         errorMsg = QString(tr("FEHLER %1: Unerwartete Dateigroesse!"))
                            .arg(fi.fileName());
         qWarning() << errorMsg;
         return false;
      }
      m_dsCount = (m_dtaFile->size() - DTA_HEADER_LENGTH) / dsSize;
      // Unterversion
      if(m_dtaVersion==DTA8208)
          m_dtaSubVersion = 1;
      else
          m_dtaSubVersion = 2;
   }

   // Unter-Version ueberpruefen
   if( m_dtaVersion==DTA9000) {
      if( header[1] < DTA9000_SUBVERSION) m_dtaSubVersion = 1;
      else m_dtaSubVersion = 2;
   }
   if( m_dtaVersion==DTA9001)
       m_dtaSubVersion = header[1]%4;

   // Anzahl der Datensaetze lesen
   if( (m_dtaVersion==DTA9001) || (m_dtaVersion==DTA9003))
      m_dtaStream >> m_dsCount;

   // Feldinformation lesen
   if( m_dtaVersion == DTA9003)
       readDTA9003FieldsHeader();

   // DTA-Version als String
   if ((m_dtaVersion==DTA9000) || (m_dtaVersion==DTA9001))
      m_dtaVersionStr = QString("DTA %1.%2").arg(header[0]).arg(header[1]);
   else
      m_dtaVersionStr = QString("DTA %1").arg(header[0]);
   if (m_ZUPasVD1) m_dtaVersionStr = QString("%1 (ZUPasVD1)").arg(m_dtaVersionStr);
   if (m_dtaVersion==DTA9003)
   {
       if(m_dtaSubVersion & 0x1)
           m_dtaVersionStr = QString("%1 Comfort").arg(m_dtaVersionStr);
       if(m_dtaSubVersion & 0x2)
           m_dtaVersionStr = QString("%1 Compact").arg(m_dtaVersionStr);
   }

   return true;
}

/*---------------------------------------------------------------------------
* alle Datensaetze der Datei lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDatasets(DataMap *data)
{
   // DTA in Abhaengigkeit von Version lesen
   if( (m_dtaVersion==DTA8209) || (m_dtaVersion==DTA8208)) readDTA8209(data);
   else if( m_dtaVersion == DTA9000) readDTA9000(data);
   else if( m_dtaVersion == DTA9001) readDTA9001(data);
   else if( m_dtaVersion == DTA9003) readDTA9003(data);
   else {
      qWarning() << QString(tr("FEHLER: unbekannt DTA Version (%1)!").arg(m_dtaVersion));
   }
}

/*---------------------------------------------------------------------------
* DAT Version 8208 und 8209 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA8209(DataMap *data)
{
   quint16 value;

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

      // unbekannte Daten bei DTA-Version 0x2010
      if( m_dtaSubVersion == 1 )
         m_dtaStream.readRawData( buffer, DTA0_DATASET_LENGTH-DTA1_DATASET_LENGTH);           // [168:187]

      // VD1 mit ZUP ersetzen
      // es gibt DTA-Dateien, bei denen VD1 nicht gesetzt ist
      if(m_ZUPasVD1)
      {
          const quint8 posZUP = 1;
          const quint8 posVD1 = 7;
          values[posVD1] = values[posZUP];
      }

      // berechnete Felder
      calcFields(ts, &values);

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
* DAT Version 9000 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA9000(DataMap *data)
{
   quint16 dsLenght = DTA2_DATASET_LENGTH1;
   if( m_dtaSubVersion == 2) dsLenght = DTA2_DATASET_LENGTH2;

   qint32 ds[dsLenght-1]; // Werte des Datensatzes

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
      values[42] = ds[23]/1000.0; // AI

      // Felder, welche nur in Unterversion1 vorhanden sind
      if( m_dtaSubVersion == 1) {
        values[71] = ds[36]/10.0; // UEHZ
        values[72] = ds[37]/10.0; // UEHZsoll
        values[73] = ds[28]/10.0; // Asg.VDi
        values[74] = ds[29]/10.0; // Asg.VDa
        values[75] = ds[30]/10.0; // VDHz
      }

      // VD1 mit ZUP ersetzen
      // es gibt DTA-Dateien, bei denen VD1 nicht gesetzt ist
      if(m_ZUPasVD1)
      {
          const quint8 posZUP = 1;
          const quint8 posVD1 = 7;
          values[posVD1] = values[posZUP];
      }

      // berechnete Felder
      calcFields(ts,&values);

      // Datensatz einfuegen
      data->insert( ts, values);
   }
}

/*---------------------------------------------------------------------------
* DAT Version 9001 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA9001(DataMap *data)
{
   quint16 value;
   qint16 valueS;

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
      m_dtaStream >> valueS; values[18] = valueS/10.0;                                        // [16:17] TFB1
      m_dtaStream >> valueS; values[20] = valueS/10.0;                                        // [18:19] TA
      m_dtaStream >> valueS; values[21] = valueS/10.0;                                        // [20:21] TRLext
      m_dtaStream >> valueS; values[27] = valueS/10.0;                                        // [22:23] TRLsoll
      m_dtaStream >> valueS; values[28] = valueS/10.0;                                        // [24:25] TKM1soll
      m_dtaStream >> value; for( int j=0; j<=4; j++) values[13+j]=calcBitData(value,j);       // [26:27] Eingaenge
      values[17] = values[17]==1.0 ? 0.0 : 1.0; // EVU invertieren
      m_dtaStream >> value; for( int j=0; j<=12; j++) values[0+j]=calcBitData(value,j);       // [28:29] Ausgaenge
      m_dtaStream.readRawData( buffer, 2);                                                    // [30:31] unbekannt
      m_dtaStream >> valueS; values[57] = valueS/10.0;                                        // [32:33] TSS
      m_dtaStream >> valueS; values[58] = valueS/10.0;                                        // [34:35] TSK
      m_dtaStream >> valueS; values[59] = valueS/10.0;                                        // [36:37] TFB2
      m_dtaStream >> valueS; values[60] = valueS/10.0;                                        // [38:39] TFB3
      m_dtaStream >> valueS; values[61] = valueS/10.0;                                        // [40:41] TEE
      m_dtaStream.readRawData( buffer, 4);                                                    // [42:45] unbekannt
      m_dtaStream >> valueS; values[62] = valueS/10.0;                                        // [46:47] TMK2soll
      m_dtaStream >> valueS; values[63] = valueS/10.0;                                        // [48:49] TMK3soll
      m_dtaStream >> valueS; values[42] = valueS/1000.0;                                      // [50:51] AI
      m_dtaStream >> valueS; values[39] = valueS/1000.0;                                      // [52:53] AO1

      if (m_dtaSubVersion > 0) {
         m_dtaStream >> valueS; values[40] = valueS/1000.0;                                   // [54:55] AO2
         m_dtaStream.readRawData( buffer, 2);                                                 // [56:57] unbekannt
         m_dtaStream >> valueS; values[73] = valueS/10.0;                                     // [58:59] Ansaug Verdichter
         m_dtaStream >> valueS; values[74] = valueS/10.0;                                     // [60:61] Ansaug Verdampfer
         m_dtaStream >> valueS; values[75] = valueS/10.0;                                     // [62:63] VD Heizung
         m_dtaStream.readRawData( buffer, 8);                                                 // [64:71] unbekannt

         if ((m_dtaSubVersion == 1) || (m_dtaSubVersion == 3)) {
            m_dtaStream.readRawData( buffer, 2);                                              // [72:73] unbekannt
            m_dtaStream >> valueS; values[71] = valueS/10.0;                                  // [74:75] Ueberhitzung
            m_dtaStream >> valueS; values[72] = valueS/10.0;                                  // [76:77] Ueberhiztung Sollwert
            m_dtaStream.readRawData( buffer, 2);                                              // [78:79] unbekannt
         }

         if (m_dtaSubVersion == 3) {
            m_dtaStream.readRawData( buffer, 18);                                             // [80:97] unbekannt
         }
      }


      // VD1 mit ZUP ersetzen
      // es gibt DTA-Dateien, bei denen VD1 nicht gesetzt ist
      if(m_ZUPasVD1)
      {
          const quint8 posZUP = 1;
          const quint8 posVD1 = 7;
          values[posVD1] = values[posZUP];
      }

      // berechnete Felder
      calcFields(ts,&values);

      // Datensatz in Map einfuegen
      data->insert( ts, values);
   }
}

/*---------------------------------------------------------------------------
* DAT Version 9003 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA9003(DataMap *data)
{
    // Position von IOs merken
    QList<int> ioFields;
    for(int i=0; i<m_dta9003Fields.size(); i++)
        if(m_dta9003Fields.at(i)=="IO") ioFields << i;

    qint16 valueS;

    // jeden Datensatz lesen
    for( int i=0; i<m_dsCount; i++)
    {
       QVarLengthArray<qint16> temp(m_fieldCount); // Werte-Array
       for( int j=0; j<m_fieldCount; j++) temp[j] = 0; // initiale Werte
       quint32 ts; // Zeitstempel

         // Felder einlesen
       m_dtaStream >> ts; // [0 :3 ] Datum und Uhrzeit in Sekunden von 1.1.1970 (Unixzeit)
       for(int j=0; j<m_dta9003Fields.size(); j++)
       {
           m_dtaStream >> valueS;
           temp[j] = valueS;
       }

       DataFieldValues values(m_fieldCount); // Werte-Array
       for( int j=0; j<m_fieldCount; j++) values[j] = 0.0; // initiale Werte

       // Werte zuordnen
       dta9003SetField( "TVL",               &temp, &values, 23);
       dta9003SetField( "TRL",               &temp, &values, 22);
       dta9003SetField( "TRL_soll",          &temp, &values, 27);
       dta9003SetField( "TRL_ext",           &temp, &values, 21);
       dta9003SetField( "TA",                &temp, &values, 20);
       dta9003SetField( "THG",               &temp, &values, 24);
       dta9003SetField( "TWE",               &temp, &values, 26);
       dta9003SetField( "TWA",               &temp, &values, 25);
       dta9003SetField( "TBW",               &temp, &values, 19);
       dta9003SetField( "TFB1",              &temp, &values, 18);
       dta9003SetField( "MK1-Soll",          &temp, &values, 28);
       dta9003SetField( "Ansaug VD",         &temp, &values, 73);
       dta9003SetField( "Ansaug Verdampfer", &temp, &values, 74);
       dta9003SetField( "Ueberhitzung",      &temp, &values, 71);
       dta9003SetField( "Ueberhitzung Soll", &temp, &values, 72);
       dta9003SetField( "Durchfluss",        &temp, &values, 43); values[43] = values[43]/6.0; // Korrektur und Umrechnung in l/min
       dta9003SetField( "Mitteltemperatur",  &temp, &values, 56);
       // Werte ohne Zuordnung
       // ("Text_VL_Soll", "Text_BW_oben", "Multi1", "Multi2", Temp VDH", "Druck HD", "Druck ND", "Verfluessigungstemp.", "Verdampfungstemp.", "Schrittmotor", "PWM VBO", "PWM HUP", "Text_Freq_VD", "Spr. HUP/ZUP", "Spr. HUP/ZUP Soll", "Spr. VBO", "Spr. VBO Soll")

       // IOs zuordnen
       QList<qint16> ioValues;
       foreach( int j, ioFields) ioValues << temp.at(j);
       // Ausgaenge
       dta9003SetIOField( "HUPout",     &ioValues, &values, 0);
       dta9003SetIOField( "ZUPout",     &ioValues, &values, 1);
       dta9003SetIOField( "BUPout",     &ioValues, &values, 2);
       dta9003SetIOField( "ZW2SSTout",  &ioValues, &values, 3);
       dta9003SetIOField( "MA1out",     &ioValues, &values, 4);
       dta9003SetIOField( "MZ1out",     &ioValues, &values, 5);
       dta9003SetIOField( "ZIPout",     &ioValues, &values, 6);
       dta9003SetIOField( "Verdichter", &ioValues, &values, 7);
       dta9003SetIOField( "AVout",      &ioValues, &values, 10);
       dta9003SetIOField( "VBOout",     &ioValues, &values, 11);
       dta9003SetIOField( "ZW1out",     &ioValues, &values, 12);
       // Ausgaenge ohne Zuordnung: "VDHZ", "OUT 7", "OUT 8", "OUT 9", "FP1out"

       // Eingaenge
       dta9003SetIOField( "HDin",   &ioValues, &values, 13, true);
       dta9003SetIOField( "MOT VD", &ioValues, &values, 14, true); // Feldname: ND
       dta9003SetIOField( "MOTin",  &ioValues, &values, 15, true);
       dta9003SetIOField( "ASDin",  &ioValues, &values, 16, true);
       dta9003SetIOField( "EVU 1",  &ioValues, &values, 17, true);
       // Eingaenge ohne Zuordnung: "EVU 2", "IN 7"

       // berechnete Felder
       calcFields(ts,&values);

       // Datensatz in Map einfuegen
       data->insert( ts, values);
    }
}

/*---------------------------------------------------------------------------
* DTA9003 Werte dem Datensatz zuordnen
*---------------------------------------------------------------------------*/
inline void DtaFile::dta9003SetField(const QString &key,
                                     QVarLengthArray<qint16> *source,
                                     DataFieldValues *target, const int &index)
{
    int srcIndex = m_dta9003Fields.indexOf(key);
    if(srcIndex >= 0)
        target->replace(index, source->at(srcIndex)/10.0);
}

/*---------------------------------------------------------------------------
* DTA9003 IO Werte dem Datensatz zuordnen
*---------------------------------------------------------------------------*/
void DtaFile::dta9003SetIOField(const QString &key,
                                QList<qint16> *source,
                                DataFieldValues *target, const int &index,
                                bool invert)
{
    for( int i=0; i<m_dta9003IOs.size(); i++)
    {
        QStringList ioNames = m_dta9003IOs.at(i);
        int pos = ioNames.indexOf(key);
        if(pos >= 0)
        {
            qint16 value = source->at(i);
            qreal res = qreal((value >> pos) & 1);
            if(invert) res = res==0.0 ? 1.0 : 0.0;
            target->replace(index,res);
            return;
        }
    }
}


/*---------------------------------------------------------------------------
* Felder im Datensatz berechnen
*---------------------------------------------------------------------------*/
void DtaFile::calcFields(const quint32 &ts, DataFieldValues *values)
{
    static quint32 lastTS = 0;
    static qreal lastVD1 = 0.0;
    static qreal heatEnergy = 0.0;

    // Durchfluss Heizkreis
    // Werte fuer Durchflussmesser Grundfoss VFS 5-100
    // der Sensor hat "nur" 0.5l/min Aufloesung
    // alle Werte unter 0,5V sind als Durchfluss=0.0l/min zu werten
    const quint8 posAI1 = 42;
    const quint8 posDF = 43;
    if( values->at(posDF) == 0.0) // DTA9003 setzt Durchfluss direkt
    {
        if( (values->at(posAI1)<0.5) || (values->at(posAI1)>5.0))
            values->replace(posDF, 0.0);
        else
           values->replace(posDF, calcLinearData( 1, values->at(posAI1) * 95.0/3.0, -65.0/6.0, 2));
    }

    // Spreizung Heizkreis
    const quint8 posHUP = 0;
    const quint8 posTRL = 22;
    const quint8 posTVL = 23;
    if(values->at(posHUP))
        values->replace(44, values->at(posTVL) - values->at(posTRL));
    else
        values->replace(44,0.0);

    // Spreizung Waermequelle
    const quint8 posVBS = 11;
    const quint8 posTWQein = 26;
    const quint8 posTWQaus = 25;
    if(values->at(posVBS))
        values->replace(45,values->at(posTWQein) - values->at(posTWQaus));
    else values->replace(45,0.0);

    // thermische Leistung
    // Qth = Durchfluss->at(l/min) * Spreizung->at(K) / 60 * c->at(kJ/kg) * Dichte->at(kg/l)
    //   c(Wasser) = 4.18kJ/kg bei 30 Grad C
    //   Dichte = 1.0044^-1 kg/l bei 30 Grad C
    const quint8 posSpHz = 44;
    values->replace(46,qRound( values->at(posDF)*values->at(posSpHz)/60.0 * 4.18*0.9956 * 100)/100.0);

    //
    // Berechnung Waermemenge
    //
    const quint8 posVD1 = 7;
    const quint8 posQth = 46;
    if( lastVD1==0.0 && values->at(posVD1)==1) heatEnergy = 0.0;
    if(ts-lastTS > MISSING_DATA_GAP) // Luecke gefunden
       heatEnergy = 0.0;
    else
       heatEnergy += values->at(posQth) * (ts-lastTS) / 3600.0;
    values->replace(64,heatEnergy);
    lastTS = ts;
    lastVD1 = values->at(posVD1);
}

/*---------------------------------------------------------------------------
* Feldinformation aus DTA9003 lesen
*---------------------------------------------------------------------------*/
void DtaFile::readDTA9003FieldsHeader()
{
   // Puffer zum Dummy-Lesen (ist schneller als skipRawData)
   char buffer[50];

   quint8 fieldType;
   qint16 fieldsToRead = -1;
   m_dtaSubVersion = 0;

   m_dtaStream.readRawData( buffer, 2); // unbekannte Daten
   while(true)
   {
       m_dtaStream >> fieldType;
       fieldsToRead -= 1;

       if(fieldType%8 >= 2)
       {
           // IO Feld
           QStringList ioFields = readDTA9003IOFieldsHeader(fieldType);
           m_dta9003IOs.append(ioFields);
           m_dta9003Fields << "IO";

           // Felddaten
           quint8 fieldData = 0;
           m_dtaStream >> fieldData;

           // Ende mit 0xC2 AND fieldData==0
           if(fieldType==0xC2 && fieldData==0) break;

       } else if (fieldType%2 == 0)
       {
           // Textfeld
           QString text = readString();
           if(text=="Text_Sim_Confort_Platine")
           {
               fieldsToRead = 11;
               m_dtaSubVersion += 1;
           }
           else if(text=="Text_Sim_Compact_Platine")
           {
               fieldsToRead = 9;
               m_dtaSubVersion += 2;
           }

       } else {
           // "normales Feld"
           QString fn = readString();
           m_dtaStream.readRawData( buffer, 3); // unbekannte Daten
           m_dta9003Fields << fn;
       }

       // Ende
       if(fieldsToRead==0)
           break;

   }

   //qDebug() << m_dta9003Fields;
   //qDebug() << m_dta9003IOs;
}

/*---------------------------------------------------------------------------
* Inhalt eines IO-Feldes auslesen
*---------------------------------------------------------------------------*/
QStringList DtaFile::readDTA9003IOFieldsHeader(const quint8 fieldType)
{
    QStringList res;
    char buffer[10];
    // Anzahl der Felder
    quint8 count;
    m_dtaStream >> count;
    m_dtaStream.readRawData( buffer, 1); // unbekannte Daten
    if(fieldType==0x44)
        m_dtaStream.readRawData(buffer, 3); // umbekannte Daten
    while(count>0)
    {
       m_dtaStream.readRawData( buffer, 1); // unbekannte Daten
       res << readString();
       m_dtaStream.readRawData( buffer, 2); // unbekannte Daten
       count--;
    }
    return res;
}

/*---------------------------------------------------------------------------
* String von DTA lesen (bis 0x0)
*---------------------------------------------------------------------------*/
QString DtaFile::readString()
{
    QString res;
    quint8 c;
    m_dtaStream >> c;
    while(c!=0)
    {
        res.append(char(c));
        m_dtaStream >> c;
    }
    return res;
}
