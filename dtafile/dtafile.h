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
* - Klasse zum Lesen und Decodieren von DTA-Dateien
* - Alpha-InnoTec Waermepumpen Steuerung 'Luxtronik 2' speichert die Daten
*   (Schaltzustaende und Temperaturen) der letzten 48h in DTA-Dateien
* - baugleiche Waermepumpen: Siemens Novelan, ?
*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
*
* Aufbau der DTA-Dateien (von Firmware Version 1.x):
*  - die ersten 8 Byte sind ein Datei-Kopf
*      [0:3]: 0x2011 fuer Version 1.x
*      [4:7]: unbekannte Funktion
*  - dann folgen 2880 Datensaetze (48 Stunden, ein Datensatz pro Minute)
*  - jeder Datensatz ist 168 Byte lang
*  - die Byte-Order ist little-endian
*  - die Datensaetze sind nur bedingt sortiert, sie werden in einem
*    Round-Robin-Verfahren gespeichert (die Schreibposition rueckt mit jedem
*    Datensatz weiter und springt am Ende der Datei wieder an den Anfang)
*
* Aufbau eines Datensatzes
*  -   [0:3]   Datum und Uhrzeit in Sekunden von 1.1.1970 (Unixzeit)
*  -   [8:9]   StatusA = Status der Ausgaenge
*        bit 0:  HUP  = Heizungsumwaelzpumpe
*        bit 1:  ZUP  = Zusatzumwaelzpumpe
*        bit 2:  BUP  = Brauswarmwasserumwaelzpumpe oder Drei-Wege-Ventil auf Brauchwassererwaermung
*        bit 3:  ZW2  = Zusaetzlicher Waermeerzeuger 2 / Sammelstoerung
*        bit 4:  MA1  = Mischer 1 auf
*        bit 5:  MZ1  = Mischer 1 zu
*        bit 6:  ZIP  = Zirkulationspumpe
*        bit 7:  VD1  = Verdichter 1
*        bit 8:  VD2  = Verdichter 2
*        bit 9:  VENT = Ventilation des WP Gehaeses / 2. Stufe des Ventilators
*        bit 10: AV   = Abtauventil (Kreislaufumkehr)
*        bit 11: VBS  = Ventilator, Brunnen- oder Soleumwaelzpumpe
*        bit 12: ZW1  = Zusaetzlicher Waermeerzeuger 1
*  -   [44:45] StatusE = Status der Eingaenge (die Bits sind invertiert zur Funktion)
*        bit 0:  HD_  = Hochdruckpressostat
*        bit 1:  ND_  = Niederdruckpressostat
*        bit 2:  MOT_ = Motorschutz
*        bit 3:  ASD_ = Abtau/Soledruck/Durchfluss
*        bit 4:  EVU_ = EVU Sperre
*  -   [52:53] TFB1    = Temperatur Fussbodenheizung 1
*  -   [54:55] TBW     = Temperatur Brauch-Warm-Wasser
*  -   [56:57] TA      = Aussentemperatur
*  -   [58:59] TRL     = Temperatur Heizung Ruecklauf extern
*  -   [60:61] TRL     = Temperatur Heizung Ruecklauf
*  -   [62:63] TVL     = Temperatur Heizung Vorlauf
*  -   [64:65] THG     = Temperatur Heissgas
*  -   [66:67] TWQaus  = Temperatur Waermequelle Austritt
*  -   [70:71] TWQein  = Temperatur Waermequelle Eintritt
*  -   [80:81] TRLsoll = Solltemperatur Heizung Ruecklauf
*  -   [82:83] TRLsoll_highbytes = zwei Extra-Byte fuer TRLsoll (werden nicht ausgelesen)
*  -   [84:85] TMK1soll= Solltemperatur Mischer Kreis 1
*  -   [86:87] TMK1soll_highbytes = zwei Extra-Byte fuer TMK1soll (werden nicht ausgelesen)
*  - [128:129] ComfortPlatine: Indikator, ob und welche Erweiterung eingebaut ist
*  - [132:133] StatusA_CP = Status der Ausgaenge der ComfortPlatine
*        bit 6:  AI1DIV = Spannungsteiler an AI1: wann AI1DIV dann AI1 = AI1/2
*        bit 7:  SUP = Schwimmbadumwaelzpumpe
*        bit 8:  FUP2 = Mischkreispumpe 2 / Kuehlsignal 2
*        bit 9:  MA2 = Mischer 2 auf
*       bit 10:  MZ2 = Mischer 2 zu
*       bit 11:  MA3 = Mischer 3 auf
*       bit 11:  MZ3 = Mischer 3 zu
*       bit 12:  FUP3 = Mischkreispumpe 3 / Kuehlsignal 3
*       bit 14:  ZW3 = Zusaetzlicher Waermeerzeuger 3
*       bit 15:  SLP = Solarladepumpe
*  - [136:137] AO1 = ComfortPlatine: Analoger Ausgang 1
*  - [138:139] AO2 = ComfortPlatine: Analoger Ausgang 2
*  - [140:141] StatusE_CP = Status der Eingaenge der ComfortPlatine
*        bit 4:  SWT_ = Schwimmbadthermostat
*  - [144:145] TSS  = Temperatur Solar Speicher
*  - [146:147] TSK  = Temperatur Solar Kollektor
*  - [148:149] TFB2 = Temperatur Fussbodenheizung 2
*  - [150:151] TFB3 = Temperatur Fussbodenheizung 3
*  - [152:153] TEE  = Temperatur Externe Energiequelle
*  - [158:159] AI1 = ComfortPlatine: Analoger Eingang 1
*  - [160:161] TMK2soll = Solltemperatur Mischer Kreis 2
*  - [162:163] TMK2soll_highbytes = zwei Extra-Byte fuer TMK2soll (werden nicht ausgelesen)
*  - [164:165] TMK3soll = Solltemperatur Mischer Kreis 3
*  - [166:167] TMK3soll_highbytes = zwei Extra-Byte fuer TMK3soll (werden nicht ausgelesen)
*
* Umrechnung der Werte:
*  Fuer die oben genannten Werte sind in den Datensaetzen natuerliche
*  Zahlen gespeichert. Diese muessen noch in die eigentlichen
*  Temperatur- und Spannungswerte umgerechnet werden. Dies erfolgt entweder
*  ueber eine lineare Berechnung (TRLsoll, TMK1soll, AO1, AO2 und AI1) oder
*  mit Hilfe einer Wertetabelle (restliche Temperaturen).
*
* invertierte Bit-Werte:
*  Feldnamen, die mit eine "_" enden (alle Eingaenge) beinhalten invertierte
*  Werte.
*
* Felder, die mit dieser Klasse berechnet werden:
*  - DF: Druchfluss Heizkreis aus AI1
*      Durchflussmesser: Grundfos VFS 5-100
*      Wertepaar: 1V=20l/min, 3,5V=100l/min
*  - SpHz: Spreizung Heizkreis (TVL-TRL)
*  - SpWQ: Spreizung Waermequelle (TWQein-TWQaus)
*  - Qh: thermische Leistung (Durchfluss * Dichte * Waermekapazitaet * SpHz)
*
*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
*
* Aufbau der DTA-Dateien (von Firmware Version 2.x):
*  - die ersten 8 Byte sind ein Datei-Kopf
*      [0:3]: 0x2011 fuer Version 1.x
*      [4:7]: unbekannte Funktion
*  - dann folgen 2880 Datensaetze (48 Stunden, ein Datensatz pro Minute)
*  - die Byte-Order ist little-endian
*  - die Datensaetze sind sortiert
*  - ein Datensatz besteht aus 4 Bytes fuer Datum/Uhrzeit und 38 Datenfeldern
*
* Aufbau eines Datenfeldes
*  - [0] Feldtyp:
*      - 0: positive Zahl, 1 Byte lang
*      - 1: positive Zahl, 2 Byte lang
*      - 4: negative Zahl, 1 Byte lang
*      - 5: negative Zahl, 2 Byte lang
*  - [1:Laenge] Wert des Feldes
*
* Zuordnung der Feldpositionen
*   [0 ] TVL
*   [1 ] TRL
*   [2 ] TWQein
*   [3 ] TWQaus?
*   [4 ] THG
*   [5 ] TBW
*   [6 ] ?
*   [7 ] TA
*   [8 ] TRLext
*   [9 ] TRLsoll
*   [10] ?
*   [11] ?
*   [12] StatusA = Status der Ausgaenge
*        bit 0:  HUP  = Heizungsumwaelzpumpe
*        bit 1:  ZUP  = Zusatzumwaelzpumpe
*        bit 2:  BUP  = Brauswarmwasserumwaelzpumpe oder Drei-Wege-Ventil auf Brauchwassererwaermung
*        bit 3:  ZW2  = Zusaetzlicher Waermeerzeuger 2 / Sammelstoerung
*        bit 4:  MA1  = Mischer 1 auf
*        bit 5:  MZ1  = Mischer 1 zu
*        bit 6:  ZIP  = Zirkulationspumpe
*        bit 7:  VD1  = Verdichter 1
*        bit 8:  VD2  = Verdichter 2
*        bit 9:  VENT = Ventilation des WP Gehaeses / 2. Stufe des Ventilators
*        bit 10: AV   = Abtauventil (Kreislaufumkehr)
*        bit 11: VBS  = Ventilator, Brunnen- oder Soleumwaelzpumpe
*        bit 12: ZW1  = Zusaetzlicher Waermeerzeuger 1
*   [13] StatusE = Status der Eingaenge (die Bits sind invertiert zur Funktion)
*        bit 0:  HD_  = Hochdruckpressostat
*        bit 1:  ND_  = Niederdruckpressostat
*        bit 2:  MOT_ = Motorschutz
*        bit 3:  ASD_ = Abtau/Soledruck/Durchfluss
*        bit 4:  EVU_ = EVU Sperre
*   [14] ?
*   [15] ?
*   [16] ?
*   [17] ?
*   [18] ?
*   [19] ?
*   [20] ?
*   [21] ?
*   [22] ?
*   [23] DF
*   [24] ?
*   [25] ?
*   [26] ?
*   [27] Ansaug VD
*   [28] Ansaug Verdampfer
*   [29] VD-Heizung
*   [30] ?
*   [31] ?
*   [32] ?
*   [33] ?
*   [34] ?
*   [35] Ueberhitzung
*   [36] Ueberhitzung soll
*   [37] ?
*
* Umrechnung der Werte:
*  Fuer die oben genannten Werte sind in den Datensaetzen in der Regel
*  die Messwerte x10 gespeichert.
*
* invertierte Bit-Werte:
*  Feldnamen, die mit eine "_" enden (alle Eingaenge) beinhalten invertierte
*  Werte.
*
* Felder, die mit dieser Klasse berechnet werden:
*  - SpHz: Spreizung Heizkreis (TVL-TRL)
*  - SpWQ: Spreizung Waermequelle (TWQein-TWQaus)
*  - Qh: thermische Leistung (Durchfluss * Dichte * Waermekapazitaet * SpHz)
*
*---------------------------------------------------------------------------*/

#ifndef DTAFILE_H
#define DTAFILE_H

#include <QObject>
#include <QFile>
#include <QDataStream>

#include "dtafile/datafile.h"

#define DTA_HEADER_LENGTH 8      // bytes
#define DTA1_DATASET_LENGTH 168  // bytes
#define DTA2_DATASET_LENGTH1 39  // fields
#define DTA2_DATASET_LENGTH2 26  // fields

// Header-Werte fuer unterschiedliche Datei-Versionen
#define DTA1_HEADER_VALUE 0x2011
#define DTA2_HEADER_VALUE 0x2328
#define DTA2_HEADER_VALUE_SUBVERSION 676

// Struktur mit Informationen einer Wertetabelle
typedef struct
{
   qint16 data[105];  // Y-Datenpunkte
   qint16 offset;     // Verschiebung des Anfangs der Datenpunkte
   qint16 delta;      // X-Abstand zwischen den Datenpunkten
   quint8 precision;  // Praezision der Datenpunkte
} DtaLUTInfo;

/*---------------------------------------------------------------------------
* DtaFile
*---------------------------------------------------------------------------*/
class DtaFile : public DataFile
{
    Q_OBJECT
public:
    explicit DtaFile(QString fileName, QObject *parent = 0);
    ~DtaFile();

    virtual bool open(); // DTA-Datei oeffnen
    virtual void readDatasets(DataMap *data); // alle Datensaete lesen und in Map speichern

private:
    QFile *m_dtaFile;
    QDataStream m_dtaStream;
    quint16 m_dsCount;
    quint8 m_dtaVersion;
    quint8 m_dtaSubVersion;

    // Wertetabellen zur Umrechnung
    static const DtaLUTInfo LUT[5];
    static inline qreal calcLinearData( const quint16 &value, const qreal &m, const qreal &n, const qint8 &precision);
    static qreal calcLUTData( const quint16 &value, const DtaLUTInfo &info);
    static inline qreal calcBitData( const quint16 &value, const quint8 &pos);
    static inline qreal calcBitDataInv( const quint16 &value, const quint8 &pos);

    virtual void readDTA1(DataMap *data); // DAT Version 1.x lesen
    virtual void readDTA2(DataMap *data); // DAT Version 2.x lesen
};

#endif // DTAFILE_H
