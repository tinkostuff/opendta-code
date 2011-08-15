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
#include <QStringList>
#include <QHash>

#include "datafile.h"

/*---------------------------------------------------------------------------
* Construktor
*---------------------------------------------------------------------------*/
DataFile::DataFile(QString fileName, QObject *parent) :
    QObject(parent)
{
   this->m_fileName = fileName;
}

/*---------------------------------------------------------------------------
* Datei oeffnen
*---------------------------------------------------------------------------*/
bool DataFile::open()
{
   return false;
}

/*---------------------------------------------------------------------------
* alle Datensaetze der Datei lesen
*---------------------------------------------------------------------------*/
void DataFile::readDatasets(DataMap *data)
{
   Q_UNUSED(data)
}

/*---------------------------------------------------------------------------
* Berechnung der Feldwerte aus Array-Werten
*---------------------------------------------------------------------------*/
qreal DataFile::fieldValueReal( const DataFieldValues &values, const QString &name)
{
   quint16 idx = fieldIndex(name);
   return values[idx];
}
qint32 DataFile::fieldValueInt( const DataFieldValues &values, const QString &name)
{
   return qRound(fieldValueReal(values,name));
}

/*---------------------------------------------------------------------------
* Feld Kategorien
*---------------------------------------------------------------------------*/
QStringList DataFile::fieldCategories()
{
   QStringList res;
   res << tr("Temperaturen");
   res << tr("Ausg\344nge");
   res << tr("Eing\344nge");
   res << tr("Analoge Signale");
   res << tr("Berechnete Werte");
   res << tr("Web-Interface");
   res << tr("Sonstiges");
   return res;
}

/*---------------------------------------------------------------------------
* Feldinformationen
*---------------------------------------------------------------------------*/
const DataFieldInfo* const DataFile::fieldInfo(const quint16 &index)
{
   if(index<m_fieldCount) return &m_fieldInfoArray[index];
   return &m_defaultFieldInfo;
}

const DataFieldInfo* const DataFile::fieldInfo(const QString &name)
{
   quint16 idx = fieldIndex(name);
   if(idx<m_fieldCount) return &m_fieldInfoArray[idx];
   return &m_defaultFieldInfo;
}

const DataFieldInfo* const DataFile::defaultFieldInfo()
{
   return &m_defaultFieldInfo;
}

/*---------------------------------------------------------------------------
*----------------------------------------------------------------------------
* Initialisierung statischer Variablen
*---------------------------------------------------------------------------
*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
* statisches Array mit Feldnamen
*---------------------------------------------------------------------------*/
const QString DataFile::m_fieldNamesArray[DATA_DS_FIELD_COUNT] = {
   // Status Ausgaenge
   "HUP",  // Heizungsumwaelzpumpe
   "ZUP",  // Zusatzumwaelzpumpe
   "BUP",  // Brauswarmwasserumwaelzpumpe oder Drei-Wege-Ventil auf Brauchwassererwaermung
   "ZW2",  // Zusaetzlicher Waermeerzeuger 2 / Sammelstoerung
   "MA1",  // Mischer 1 auf
   "MZ1",  // Mischer 1 zu
   "ZIP",  // Zirkulationspumpe
   "VD1",  // Verdichter 1
   "VD2",  // Verdichter 2
   "VENT", // Ventilation des WP Gehaeses / 2. Stufe des Ventilators
   "AV",   // Abtauventil (Kreislaufumkehr)
   "VBS",  // Ventilator, Brunnen- oder Soleumwaelzpumpe
   "ZW1",  // Zusaetzlicher Waermeerzeuger 1

   // Status Eingaenge
   "HD",  // Hochdruckpressostat
   "ND",  // Niederdruckpressostat
   "MOT", // Motorschutz
   "ASD", // Abtau/Soledruck/Durchfluss
   "EVU", // EVU Sperre

   // Temperaturen
   "TFB1",     // TFB1
   "TBW",      // TBW
   "TA",       // TA
   "TRLext",   // TRLext
   "TRL",      // TRL
   "TVL",      // TVL
   "THG",      // THG
   "TWQaus",   // TWQaus
   "TWQein",   // TQWein
   "TRLsoll",  // TRLsoll
   "TMK1soll", // TMK1soll

   // Status Ausgaenge ComfortPlatine
   "AI1DIV", // wenn AI1DIV dann AI1 = AI1/2
   "SUP",    // Schwimmbadumwaelzpumpe
   "FUP2",   // Mischkreispumpe 2 / Kuehlsignal 2
   "MA2",    // Mischer 2 auf
   "MZ2",    // Mischer 2 zu
   "MA3",    // Mischer 3 auf
   "MZ3",    // Mischer 3 zu
   "FUP3",   // Mischkreispumpe 3 / Kuehlsignal 3
   "ZW3",    // Zusaetzlicher Waermeerzeuger 3
   "SLP",    // Solarladepumpe

   "AO1",    // AO1
   "AO2",    // AO2

   // Status Eingaenge ComfortPlatine
   "SWT",  // Schwimmbadthermostat

   "AI1",   // AI1

   // berechnete Werte
   "DF",    // Durchfluss Heizkreis
   "SpHz",  // Spreizung Heizkreis
   "SpWQ",  // Spreizung Waermequelle
   "Qh",    // thermische Leistung

   // Werte vom Web-Interface
   "StdVD1", // Betriebsstunden VD1
   "StdWP",  // Betriebsstunden Waermepumpe
   "StdHz",  // Betriebsstunden Heizung
   "StdBW",  // Betriebsstunden Brauchwasser
   "StdKue", // Betriebsstunden Kuehlung
   "ImpVD1", // Impulse VD1
   "WMZ",    // Waermemengenzaehler gesamt
   "WMZHz",  // Waermemengenzaehler Heizung
   "WMZBW",  // Waermemengenzaehler Brauchwasser
   "TAm",    // Mitteltemperatur (aussen)

   // Werte der ComfortPlatine
   "TSS",      // Temperatur Solar Speicher
   "TSK",      // Temperatur Solar Kollektor
   "TFB2",     // Temperatur Fussbodenheizung 2
   "TFB3",     // Temperatur Fussbodenheizung 3
   "TEE",      // Temperatur Externe Energiequelle
   "TMK2soll", // Soll-Temperatur Mischkreis 2
   "TMK3soll", // Soll-Temperatur Mischkreis 3

   "WMCalc",   // berechnete Waermemenge
};

/*---------------------------------------------------------------------------
* statische Liste mit Feldnamen
*---------------------------------------------------------------------------*/
QStringList DataFile::initFieldList()
{
   QStringList res;
   for(int i=0; i<m_fieldCount; i++)
      res << m_fieldNamesArray[i];
   return res;
}
const QStringList DataFile::m_fieldNamesList = DataFile::initFieldList();
QStringList DataFile::fieldNames() { return DataFile::m_fieldNamesList;}
QString DataFile::fieldName(const quint16 &index)
{
   if(index<m_fieldCount) return m_fieldNamesArray[index];
   return QString();
}

/*---------------------------------------------------------------------------
* statischer Hash mit Feldnamen und Index im Array
*---------------------------------------------------------------------------*/
QHash<QString,quint16> DataFile::initFieldHash()
{
   QHash<QString,quint16> res;
   for(int i=0; i<m_fieldCount; i++)
      res.insert( m_fieldNamesArray[i], i);
   return res;
}
const QHash<QString,quint16> DataFile::m_fieldNamesHash = DataFile::initFieldHash();
quint16 DataFile::fieldIndex(const QString &name) { return m_fieldNamesHash.value( name, m_fieldCount); }

/*---------------------------------------------------------------------------
* Feld-Informationen
*---------------------------------------------------------------------------*/
const DataFieldInfo DataFile::m_fieldInfoArray[DATA_DS_FIELD_COUNT] = {
   //
   // Ausgaenge
   //
   {
      tr("HUP"),
      tr("Heizungsumw\344lzpumpe"),
      tr("Ausg\344nge"),
      false,
      0.5,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,  0),
#endif
   }, // HUP
   {
      tr("ZUP"),
      tr("Zusatzumw\344lzpumpe"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255, 65,  0),
#endif
   }, // ZUP
   {
      tr("BUP"),
      tr("Brauswarmwasserumw\344lzpumpe oder Drei-Wege-Ventil auf Brauchwassererw\344rmung"),
      tr("Ausg\344nge"),
      false,
      1.5,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,128),
#endif
   }, // BUP
   {
      tr("ZW2"),
      tr("Zus\344tzlicher W\344rmeerzeuger 2 / Sammelstoerung"),
      tr("Ausg\344nge"),
      false,
      2.1,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,255),
#endif
   }, // ZW2
   {
      tr("MA1"),
      tr("Mischer 1 auf"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,128,128),
#endif
   }, // MA1
   {
      tr("MZ1"),
      tr("Mischer 1 zu"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(160,160,160),
#endif
   }, // MZ1
   {
      tr("ZIP"),
      tr("Zirkulationspumpe"),
      tr("Ausg\344nge"),
      false,
      0.7,
      0.0,
#ifdef QT_GUI_LIB
      QColor(160,  0,160),
#endif
   }, // ZIP
   {
      tr("VD1"),
      tr("Verdichter 1"),
      tr("Ausg\344nge"),
      false,
      1.2,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,128),
#endif
   }, // VD1
   {
      tr("VD2"),
      tr("Verdichter 2"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,192,192),
#endif
   },  // VD2
   {
      tr("VENT"),
      tr("Ventilation des WP Geh\344ses / 2. Stufe des Ventilators"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,255,  0),
#endif
   }, // VENT
   {
      tr("AV"),
      tr("Abtauventil (Kreislaufumkehr)"),
      tr("Ausg\344nge"),
      false,
      1.7,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,255),
#endif
   }, // AV
   {
      tr("VBS"),
      tr("Ventilator, Brunnen- oder Soleumw\344lzpumpe"),
      tr("Ausg\344nge"),
      false,
      1.1,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   }, // VBS
   {
      tr("ZW1"),
      tr("Zus\344tzlicher W\344rmeerzeuger 1"),
      tr("Ausg\344nge"),
      false,
      2.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,  0),
#endif
   }, // ZW1

// Eingaenge
   {
      tr("HD"),
      tr("Hochdruckpressostat"),
      tr("Eing\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,  0),
#endif
   },  // HD_
   {
      tr("ND"),
      tr("Niederdruckpressostat"),
      tr("Eing\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,255),
#endif
   },  // ND_
   {
      tr("MOT"),
      tr("Motorschutz"),
      tr("Eing\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // MOT_
   {
      tr("ADS"),
      tr("Abtau/Soledruck/Durchfluss"),
      tr("Eing\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,128),
#endif
   },  // ASD_
   {
      tr("EVU"),
      tr("EVU Sperre"),
      tr("Eing\344nge"),
      false,
      1.4,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,128,  0),
#endif
   },  // EVU_

// Temperaturen
   {
      tr("TFB1 [\260C]"),
      tr("Temperatur Fu\337bodenheizung 1"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,  0),
#endif
   },  // TFB1
   {
      tr("TBW [\260C]"),
      tr("Brauchwasser-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,128),
#endif
   },  // TBW
   {
      tr("TA [\260C]"),
      tr("Au\337entemperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,128,  0),
#endif
   },  // TA
   {
      tr("TRLext [\260C]"),
      tr("R\374cklauf-Temperatur (extern)"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,128),
#endif
   },  // TRLext
   {
      tr("TRL [\260C]"),
      tr("R\374cklauf-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,255),
#endif
   },  // TRL
   {
      tr("TVL [\260C]"),
      tr("Vorlauf-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,  0),
#endif
   },  // TVL
   {
      tr("THG [\260C]"),
      tr("Hei\337gas-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,  0),
#endif
   },  // THG
   {
      tr("TWQein [\260C]"),
      tr("Temperatur W\344rmequelle Eingang"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // TWQein
   {
      tr("TWQaus [\260C]"),
      tr("Temperatur W\344rmequelle Ausgang"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,128),
#endif
   },  // TWQaus
   {
      tr("TRLsoll [\260C]"),
      tr("R\374cklauf-Soll-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,255),
#endif
   },  // TRLsoll
   {
      tr("TMK1soll [\260C]"),
      tr("Mischkreis1-Soll-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,128,128),
#endif
   },  // TMK1soll

// Ausgaenge ComfortPlatine
   {
      tr("AI1DIV"),
      tr("wenn AI1DIV dann AI1 = AI1/2"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,255,255),
#endif
   },  // AI1DIV
   {
      tr("SUP"),
      tr("Schwimmbadumw\344lzpumpe"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,255),
#endif
   },  // SUP
   {
      tr("FUP2"),
      tr("Mischkreispumpe 2 / K\374hlsignal 2"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,255,  0),
#endif
   },  // FUP2
   {
      tr("MA2"),
      tr("Mischer 2 auf"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // MA2
   {
      tr("MZ2"),
      tr("Mischer 2 zu"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,160,  0),
#endif
   },  // MZ2
   {
      tr("MA3"),
      tr("Mischer 3 auf"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,238),
#endif
   },  // MA3
   {
      tr("MZ3"),
      tr("Mischer 3 zu"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,160),
#endif
   },  // MZ3
   {
      tr("FUP3"),
      tr("Mischkreispumpe 3 / K\374hlsignal 3"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,255),
#endif
   },  // FUP3
   {
      tr("ZW3"),
      tr("Zus\344tzlicher W\344rmeerzeuger 3"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,192),
#endif
   },  // ZW3
   {
      tr("SLP"),
      tr("Solarladepumpe"),
      tr("Ausg\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,128,  0),
#endif
   },  // SLP

// ComfortPlatine
   {
      tr("AO1 [V]"),
      tr("analoger Ausgang 1"),
      tr("Analoge Signale"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,255),
#endif
   },  // AO1
   {
      tr("AO2 [V]"),
      tr("analoger Ausgang 2"),
      tr("Analoge Signale"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // AO2

// Eingaenge ComfortPlatine
   {
      tr("SWT"),
      tr("Schwimmbadthermostat"),
      tr("Eing\344nge"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,255),
#endif
   },  // SWT_

   {
      tr("AI1 [V]"),
      tr("analoger Eingang 1"),
      tr("Analoge Signale"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,  0),
#endif
   },  // AI1

// Berechnete Werte
   {
      tr("DF [l/min]"),
      tr("Durchfluss Heizkreis"),
      tr("Berechnete Werte"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // DF
   {
      tr("SpHz [K]"),
      tr("Spreizung Heizkreis"),
      tr("Berechnete Werte"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,  0),
#endif
   },  // SpHz
   {
      tr("SpWQ [K]"),
      tr("Spreizung W\344rmequelle"),
      tr("Berechnete Werte"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,128),
#endif
   },  // SpWQ
   {
      tr("Qh [kW]"),
      tr("Heizleistung"),
      tr("Berechnete Werte"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,  0),
#endif
   },  // Qh


   //
   // Werte vom Web-Interface
   //
   {
      tr("BS VD1 [h]"),
      tr("Betriebsstunden Verdichter 1"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,128),
#endif
   },  // StdVD1
   {
      tr("BS WP [h]"),
      tr("Betriebsstunden W\344rmepumpe"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // StdWP
   {
      tr("BS Hz [h]"),
      tr("Betriebsstunden Heizung"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255, 0,  0),
#endif
   },  // StdHz
   {
      tr("BS BW [h]"),
      tr("Betriebsstunden Brauchwasser"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,128),
#endif
   },  // StdBw
   {
      tr("BS K\374 [h]"),
      tr("Betriebsstunden K\374hlung"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,255),
#endif
   },  // StdKue
   {
      tr("Imp VD1 []"),
      tr("Impule Verdichter 1"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,128),
#endif
   },  // ImpVD1
   {
      tr("WMZ [kWh]"),
      tr("W\344rmemenge gesamt"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,128,  0),
#endif
   },  // WMZ
   {
      tr("WMZ Hz [kWh]"),
      tr("W\344rmemenge Heizung"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,  0,  0),
#endif
   },  // WMZHz
   {
      tr("WMZ BW [kWh]"),
      tr("W\344rmemenge Brauchwasser"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,  0,128),
#endif
   },  // WMZBW
   {
      tr("TA mittel [\260C]"),
      tr("Au\337en-Mitteltemperatur"),
      tr("Web-Interface"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(200,200,  0),
#endif
   },  // TAm

   //
   // Temperaturen Comfortplatine
   //
   {
      tr("TSS [\260C]"),
      tr("Temperatur Solar Speicher"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255,128,  0),
#endif
   },  // TSS
   {
      tr("TSK [\260C]"),
      tr("Temperatur Solar Kollektor"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255, 64,  0),
#endif
   },  // TSK
   {
      tr("TFB2 [\260C]"),
      tr("Temperatur Fu\337bodenheizung 2"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,  0),
#endif
   },  // TFB2
   {
      tr("TFB3 [\260C]"),
      tr("Temperatur Fu\337bodenheizung 3"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,  0),
#endif
   },  // TFB3
   {
      tr("TEE [\260C]"),
      tr("Temperatur externe Energiequelle"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(  0,  0,  0),
#endif
   },  // TEE
   {
      tr("TMK2soll [\260C]"),
      tr("Mischkreis2-Soll-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,128,128),
#endif
   },  // TMK2soll
   {
      tr("TMK3soll [\260C]"),
      tr("Mischkreis3-Soll-Temperatur"),
      tr("Temperaturen"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(128,128,128),
#endif
   },  // TMK3soll
   {
      tr("WM [kWh]"),
      tr("berechnete W\344rmemenge pro Verdichter-Start"),
      tr("Berechnete Werte"),
      true,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      QColor(255, 64,  0),
#endif
   },  // TMK3soll
};

const DataFieldInfo DataFile::m_defaultFieldInfo = {
   tr("XX"),
   tr("unbekanntes Feld"),
   tr("Sonstiges"),
   false,
   1.0,
   0.0,
#ifdef QT_GUI_LIB
   Qt::white,
#endif
};
