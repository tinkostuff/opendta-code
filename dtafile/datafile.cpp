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
#include <QStringList>
#include <QHash>

#include "datafile.h"

/*---------------------------------------------------------------------------
* DataFieldInfo
*---------------------------------------------------------------------------*/
DataFieldInfo::DataFieldInfo(
   QString prettyName,
   QString toolTip,
   QString category,
   bool analog,
   qreal scale,
   qreal offset,
#ifdef QT_GUI_LIB
   QColor color,
#endif
   QObject *parent) :
   QObject(parent)
{
   this->prettyName = prettyName;
   this->toolTip = toolTip;
   this->category = category;
   this->analog = analog;
   this->scale = scale;
   this->offset = offset;
#ifdef QT_GUI_LIB
   this->color = color;
#endif
}
DataFieldInfo::DataFieldInfo(const DataFieldInfo &info) : QObject()
{
   this->prettyName = info.prettyName;
   this->toolTip = info.toolTip;
   this->category = info.category;
   this->analog = info.analog;
   this->scale = info.scale;
   this->offset = info.offset;
#ifdef QT_GUI_LIB
   this->color = info.color;
#endif
}
DataFieldInfo DataFieldInfo::operator =(const DataFieldInfo &info)
{
   return DataFieldInfo(
            info.prettyName,
            info.toolTip,
            info.category,
            info.analog,
            info.scale,
            info.offset,
#ifdef QT_GUI_LIB
            info.color,
#endif
            0
            );
}

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
* Dateiversion
*---------------------------------------------------------------------------*/
QString DataFile::version()
{
   return QString("");
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
   res << tr("Ausgänge");
   res << tr("Eingänge");
   res << tr("Analoge Signale");
   res << tr("Berechnete Werte");
   res << tr("Web-Interface");
   res << tr("Elektro-Zähler");
   res << tr("Sonstiges");
   return res;
}

/*---------------------------------------------------------------------------
* Feldinformationen
*---------------------------------------------------------------------------*/
const DataFieldInfo DataFile::fieldInfo(const quint16 &index)
{
   if(index<m_fieldCount) return fieldInfos().at(index);
   return defaultFieldInfo();
}

const DataFieldInfo DataFile::fieldInfo(const QString &name)
{
   quint16 idx = fieldIndex(name);
   if(idx<m_fieldCount) return fieldInfos().at(idx);
   return defaultFieldInfo();
}

const DataFieldInfo DataFile::defaultFieldInfo()
{
   return DataFieldInfo(
      tr("XX"),
      tr("unbekanntes Feld"),
      tr("Sonstiges"),
      false,
      1.0,
      0.0,
#ifdef QT_GUI_LIB
      Qt::white,
#endif
      0
   );
}

const DataFieldInfoList DataFile::fieldInfos()
{
   DataFieldInfoList res;

   //
   // Ausgaenge
   //
   res << DataFieldInfo(
             tr("HUP"),
             tr("Heizungsumwälzpumpe"),
             tr("Ausgänge"),
             false,
             0.5,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,  0),
          #endif
             0
             ); // HUP
   res << DataFieldInfo(
             tr("ZUP"),
             tr("Zusatzumwälzpumpe"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255, 65,  0),
          #endif
             0
             ); // ZUP
   res << DataFieldInfo(
             tr("BUP"),
             tr("Brauswarmwasserumwälzpumpe oder Drei-Wege-Ventil auf Brauchwassererwärmung"),
             tr("Ausgänge"),
             false,
             1.5,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,128),
          #endif
             0
             ); // BUP
   res << DataFieldInfo(
             tr("ZW2"),
             tr("Zusätzlicher Wärmeerzeuger 2 / Sammelstoerung"),
             tr("Ausgänge"),
             false,
             2.1,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,255),
          #endif
             0
             ); // ZW2
   res << DataFieldInfo(
             tr("MA1"),
             tr("Mischer 1 auf"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,128),
          #endif
             0
             ); // MA1
   res << DataFieldInfo(
             tr("MZ1"),
             tr("Mischer 1 zu"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(160,160,160),
          #endif
             0
             ); // MZ1
   res << DataFieldInfo(
             tr("ZIP"),
             tr("Zirkulationspumpe"),
             tr("Ausgänge"),
             false,
             0.7,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(160,  0,160),
          #endif
             0
             ); // ZIP
   res << DataFieldInfo(
             tr("VD1"),
             tr("Verdichter 1"),
             tr("Ausgänge"),
             false,
             1.2,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,128),
          #endif
             0
             ); // VD1
   res << DataFieldInfo(
             tr("VD2"),
             tr("Verdichter 2"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,192,192),
          #endif
             0
             );  // VD2
   res << DataFieldInfo(
             tr("VENT"),
             tr("Ventilation des WP Gehäses / 2. Stufe des Ventilators"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,255,  0),
          #endif
             0
             ); // VENT
   res << DataFieldInfo(
             tr("AV"),
             tr("Abtauventil (Kreislaufumkehr)"),
             tr("Ausgänge"),
             false,
             1.7,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,255),
          #endif
             0
             ); // AV
   res << DataFieldInfo(
             tr("VBS"),
             tr("Ventilator, Brunnen- oder Soleumwälzpumpe"),
             tr("Ausgänge"),
             false,
             1.1,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             ); // VBS
   res << DataFieldInfo(
             tr("ZW1"),
             tr("Zusätzlicher Wärmeerzeuger 1"),
             tr("Ausgänge"),
             false,
             2.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,  0),
          #endif
             0
             ); // ZW1

   //
   // Eingaenge
   //
   res << DataFieldInfo(
             tr("HD"),
             tr("Hochdruckpressostat"),
             tr("Eingänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,  0),
          #endif
             0
             );  // HD_
   res << DataFieldInfo(
             tr("ND"),
             tr("Niederdruckpressostat"),
             tr("Eingänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,255),
          #endif
             0
             );  // ND_
   res << DataFieldInfo(
             tr("MOT"),
             tr("Motorschutz"),
             tr("Eingänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // MOT_
   res << DataFieldInfo(
             tr("ASD"),
             tr("Abtau/Soledruck/Durchfluss"),
             tr("Eingänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,128),
          #endif
             0
             );  // ASD_
   res << DataFieldInfo(
             tr("EVU"),
             tr("EVU Sperre"),
             tr("Eingänge"),
             false,
             1.4,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,  0),
          #endif
             0
             );  // EVU_

   //
   // Temperaturen
   //
   res << DataFieldInfo(
             tr("TFB1 [°C]"),
             tr("Temperatur Fußbodenheizung 1"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,  0),
          #endif
             0
             );  // TFB1
   res << DataFieldInfo(
             tr("TBW [°C]"),
             tr("Brauchwasser-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,128),
          #endif
             0
             );  // TBW
   res << DataFieldInfo(
             tr("TA [°C]"),
             tr("Außentemperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,  0),
          #endif
             0
             );  // TA
   res << DataFieldInfo(
             tr("TRLext [°C]"),
             tr("Rücklauf-Temperatur (extern)"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,128),
          #endif
             0
             );  // TRLext
   res << DataFieldInfo(
             tr("TRL [°C]"),
             tr("Rücklauf-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,255),
          #endif
             0
             );  // TRL
   res << DataFieldInfo(
             tr("TVL [°C]"),
             tr("Vorlauf-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,  0),
          #endif
             0
             );  // TVL
   res << DataFieldInfo(
             tr("THG [°C]"),
             tr("Heißgas-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,  0),
          #endif
             0
             );  // THG
   res << DataFieldInfo(
             tr("TWQaus [°C]"),
             tr("Temperatur Wärmequelle Ausgang"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,128),
          #endif
             0
             );  // TWQaus
   res << DataFieldInfo(
             tr("TWQein [°C]"),
             tr("Temperatur Wärmequelle Eingang"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // TWQein
   res << DataFieldInfo(
             tr("TRLsoll [°C]"),
             tr("Rücklauf-Soll-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,255),
          #endif
             0
             );  // TRLsoll
   res << DataFieldInfo(
             tr("TMK1soll [°C]"),
             tr("Mischkreis1-Soll-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,128),
          #endif
             0
             );  // TMK1soll

   //
   // Ausgaenge ComfortPlatine
   //
   res << DataFieldInfo(
             tr("AI1DIV"),
             tr("wenn AI1DIV dann AI1 = AI1/2"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,255,255),
          #endif
             0
             );  // AI1DIV
   res << DataFieldInfo(
             tr("SUP"),
             tr("Schwimmbadumwälzpumpe"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,255),
          #endif
             0
             );  // SUP
   res << DataFieldInfo(
             tr("FUP2"),
             tr("Mischkreispumpe 2 / Kühlsignal 2"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,255,  0),
          #endif
             0
             );  // FUP2
   res << DataFieldInfo(
             tr("MA2"),
             tr("Mischer 2 auf"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // MA2
   res << DataFieldInfo(
             tr("MZ2"),
             tr("Mischer 2 zu"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,160,  0),
          #endif
             0
             );  // MZ2
   res << DataFieldInfo(
             tr("MA3"),
             tr("Mischer 3 auf"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,238),
          #endif
             0
             );  // MA3
   res << DataFieldInfo(
             tr("MZ3"),
             tr("Mischer 3 zu"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,160),
          #endif
             0
             );  // MZ3
   res << DataFieldInfo(
             tr("FUP3"),
             tr("Mischkreispumpe 3 / Kühlsignal 3"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,255),
          #endif
             0
             );  // FUP3
   res << DataFieldInfo(
             tr("ZW3"),
             tr("Zusätzlicher Wärmeerzeuger 3"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,192),
          #endif
             0
             );  // ZW3
   res << DataFieldInfo(
             tr("SLP"),
             tr("Solarladepumpe"),
             tr("Ausgänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,128,  0),
          #endif
             0
             );  // SLP

   //
   // ComfortPlatine
   //
   res << DataFieldInfo(
             tr("AO1 [V]"),
             tr("analoger Ausgang 1"),
             tr("Analoge Signale"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,255),
          #endif
             0
             );  // AO1
   res << DataFieldInfo(
             tr("AO2 [V]"),
             tr("analoger Ausgang 2"),
             tr("Analoge Signale"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // AO2
   res << DataFieldInfo(
             tr("SWT"),
             tr("Schwimmbadthermostat"),
             tr("Eingänge"),
             false,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,255),
          #endif
             0
             );  // SWT_

   res << DataFieldInfo(
             tr("AI1 [V]"),
             tr("analoger Eingang 1"),
             tr("Analoge Signale"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,  0),
          #endif
             0
             );  // AI1

   //
   // Berechnete Werte
   //
   res << DataFieldInfo(
             tr("DF [l/min]"),
             tr("Durchfluss Heizkreis"),
             tr("Berechnete Werte"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // DF
   res << DataFieldInfo(
             tr("SpHz [K]"),
             tr("Spreizung Heizkreis"),
             tr("Berechnete Werte"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,  0),
          #endif
             0
             );  // SpHz
   res << DataFieldInfo(
             tr("SpWQ [K]"),
             tr("Spreizung Wärmequelle"),
             tr("Berechnete Werte"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,128),
          #endif
             0
             );  // SpWQ
   res << DataFieldInfo(
             tr("Qh [kW]"),
             tr("Heizleistung"),
             tr("Berechnete Werte"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,  0),
          #endif
             0
             );  // Qh

   //
   // Werte vom Web-Interface
   //
   res << DataFieldInfo(
             tr("BS VD1 [h]"),
             tr("Betriebsstunden Verdichter 1"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,128),
          #endif
             0
             );  // StdVD1
   res << DataFieldInfo(
             tr("BS WP [h]"),
             tr("Betriebsstunden Wärmepumpe"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // StdWP
   res << DataFieldInfo(
             tr("BS Hz [h]"),
             tr("Betriebsstunden Heizung"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255, 0,  0),
          #endif
             0
             );  // StdHz
   res << DataFieldInfo(
             tr("BS BW [h]"),
             tr("Betriebsstunden Brauchwasser"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,128),
          #endif
             0
             );  // StdBw
   res << DataFieldInfo(
             tr("BS Kü [h]"),
             tr("Betriebsstunden Kühlung"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,255),
          #endif
             0
             );  // StdKue
   res << DataFieldInfo(
             tr("Imp VD1 []"),
             tr("Impule Verdichter 1"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,128),
          #endif
             0
             );  // ImpVD1
   res << DataFieldInfo(
             tr("WMZ [kWh]"),
             tr("Wärmemenge gesamt"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,  0),
          #endif
             0
             );  // WMZ
   res << DataFieldInfo(
             tr("WMZ Hz [kWh]"),
             tr("Wärmemenge Heizung"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,  0,  0),
          #endif
             0
             );  // WMZHz
   res << DataFieldInfo(
             tr("WMZ BW [kWh]"),
             tr("Wärmemenge Brauchwasser"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,  0,128),
          #endif
             0
             );  // WMZBW
   res << DataFieldInfo(
             tr("TA mittel [°C]"),
             tr("Außen-Mitteltemperatur"),
             tr("Web-Interface"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(200,200,  0),
          #endif
             0
             );  // TAm

   //
   // Temperaturen Comfortplatine
   //
   res << DataFieldInfo(
             tr("TSS [°C]"),
             tr("Temperatur Solar Speicher"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255,128,  0),
          #endif
             0
             );  // TSS
   res << DataFieldInfo(
             tr("TSK [°C]"),
             tr("Temperatur Solar Kollektor"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255, 64,  0),
          #endif
             0
             );  // TSK
   res << DataFieldInfo(
             tr("TFB2 [°C]"),
             tr("Temperatur Fußbodenheizung 2"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,  0),
          #endif
             0
             );  // TFB2
   res << DataFieldInfo(
             tr("TFB3 [°C]"),
             tr("Temperatur Fußbodenheizung 3"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,  0),
          #endif
             0
             );  // TFB3
   res << DataFieldInfo(
             tr("TEE [°C]"),
             tr("Temperatur externe Energiequelle"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,  0),
          #endif
             0
             );  // TEE
   res << DataFieldInfo(
             tr("TMK2soll [°C]"),
             tr("Mischkreis2-Soll-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,128),
          #endif
             0
             );  // TMK2soll
   res << DataFieldInfo(
             tr("TMK3soll [°C]"),
             tr("Mischkreis3-Soll-Temperatur"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,128),
          #endif
             0
             );  // TMK3soll
   res << DataFieldInfo(
             tr("WM [kWh]"),
             tr("berechnete Wärmemenge pro Verdichter-Start"),
             tr("Berechnete Werte"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(255, 64,  0),
          #endif
             0
             );  // WMCalc

   //
   // elektrische Energie
   //
   res << DataFieldInfo(
             tr("Pe1 [W]"),
             tr("elektrische Leistung Verdichter"),
             tr("Elektro-Zähler"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,128,128),
          #endif
             0
             );  // Pe1
   res << DataFieldInfo(
             tr("Pe2 [W]"),
             tr("elektrische Leistung Steuerung, Pumpen"),
             tr("Elektro-Zähler"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(128,128,  0),
          #endif
             0
             );  // Pe2
   res << DataFieldInfo(
             tr("AZ1 []"),
             tr("Arbeitszahl mit Elt1"),
             tr("Elektro-Zähler"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0, 64, 64),
          #endif
             0
             );  // AZ1
   res << DataFieldInfo(
             tr("AZ2 []"),
             tr("Arbeitszahl mit Elt1 und Elt2"),
             tr("Elektro-Zähler"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor( 64, 64,  0),
          #endif
             0
             );  // AZ2
   res << DataFieldInfo(
             tr("E1 [kWh]"),
             tr("elektrische Energie Verdichter"),
             tr("Elektro-Zähler"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,192,192),
          #endif
             0
             );  // E1
   res << DataFieldInfo(
             tr("E2 [kWh]"),
             tr("elektrische Energie Steuerung, Pumpen"),
             tr("Elektro-Zähler"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(192,192,  0),
          #endif
             0
             );  // E2

   res << DataFieldInfo(
             tr("UEHZ [K]"),
             tr("Überhitzung"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,192),
          #endif
             0
             );  // UEHZ
   res << DataFieldInfo(
             tr("UEHZsoll [K]"),
             tr("Überhitzung Sollwert"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,192),
          #endif
             0
             );  // UEHZsoll
   res << DataFieldInfo(
             tr("Asg.VDi [°C]"),
             tr("Ansaug Verdichter"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,192),
          #endif
             0
             );  // Asg.VDi
   res << DataFieldInfo(
             tr("Asg.VDa [°C]"),
             tr("Ansaug Verdampfer"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,192),
          #endif
             0
             );  // Asg.VDa
   res << DataFieldInfo(
             tr("VDHz [°C]"),
             tr("VD Heizung"),
             tr("Temperaturen"),
             true,
             1.0,
             0.0,
          #ifdef QT_GUI_LIB
             QColor(  0,  0,192),
          #endif
             0
             );  // Asg.VDi   return res;

   Q_ASSERT( m_fieldCount == res.size());
   return res;
}

/*---------------------------------------------------------------------------
*----------------------------------------------------------------------------
* Initialisierung statischer Variablen
*---------------------------------------------------------------------------
*---------------------------------------------------------------------------*/
const quint16 DataFile::m_fieldCount = DATA_DS_FIELD_COUNT;

/*---------------------------------------------------------------------------
* statische Feldnamen
*---------------------------------------------------------------------------*/
QStringList DataFile::fieldNames()
{
   QStringList res;

   // Status Ausgaenge
   res << "HUP";  // Heizungsumwaelzpumpe
   res << "ZUP";  // Zusatzumwaelzpumpe
   res << "BUP";  // Brauswarmwasserumwaelzpumpe oder Drei-Wege-Ventil auf Brauchwassererwaermung
   res << "ZW2";  // Zusaetzlicher Waermeerzeuger 2 / Sammelstoerung
   res << "MA1";  // Mischer 1 auf
   res << "MZ1";  // Mischer 1 zu
   res << "ZIP";  // Zirkulationspumpe
   res << "VD1";  // Verdichter 1
   res << "VD2";  // Verdichter 2
   res << "VENT"; // Ventilation des WP Gehaeses / 2. Stufe des Ventilators
   res << "AV";   // Abtauventil (Kreislaufumkehr)
   res << "VBS";  // Ventilator, Brunnen- oder Soleumwaelzpumpe
   res << "ZW1";  // Zusaetzlicher Waermeerzeuger 1

   // Status Eingaenge
   res << "HD";  // Hochdruckpressostat
   res << "ND";  // Niederdruckpressostat
   res << "MOT"; // Motorschutz
   res << "ASD"; // Abtau/Soledruck/Durchfluss
   res << "EVU"; // EVU Sperre

   // Temperaturen
   res << "TFB1";     // TFB1
   res << "TBW";      // TBW
   res << "TA";       // TA
   res << "TRLext";   // TRLext
   res << "TRL";      // TRL
   res << "TVL";      // TVL
   res << "THG";      // THG
   res << "TWQaus";   // TWQaus
   res << "TWQein";   // TQWein
   res << "TRLsoll";  // TRLsoll
   res << "TMK1soll"; // TMK1soll

   // Status Ausgaenge ComfortPlatine
   res << "AI1DIV"; // wenn AI1DIV dann AI1 = AI1/2
   res << "SUP";    // Schwimmbadumwaelzpumpe
   res << "FUP2";   // Mischkreispumpe 2 / Kuehlsignal 2
   res << "MA2";    // Mischer 2 auf
   res << "MZ2";    // Mischer 2 zu
   res << "MA3";    // Mischer 3 auf
   res << "MZ3";    // Mischer 3 zu
   res << "FUP3";   // Mischkreispumpe 3 / Kuehlsignal 3
   res << "ZW3";    // Zusaetzlicher Waermeerzeuger 3
   res << "SLP";    // Solarladepumpe

   res << "AO1";    // AO1
   res << "AO2";    // AO2

   // Status Eingaenge ComfortPlatine
   res << "SWT";  // Schwimmbadthermostat

   res << "AI1";   // AI1

   // berechnete Werte
   res << "DF";    // Durchfluss Heizkreis
   res << "SpHz";  // Spreizung Heizkreis
   res << "SpWQ";  // Spreizung Waermequelle
   res << "Qh";    // thermische Leistung

   // Werte vom Web-Interface
   res << "StdVD1"; // Betriebsstunden VD1
   res << "StdWP";  // Betriebsstunden Waermepumpe
   res << "StdHz";  // Betriebsstunden Heizung
   res << "StdBW";  // Betriebsstunden Brauchwasser
   res << "StdKue"; // Betriebsstunden Kuehlung
   res << "ImpVD1"; // Impulse VD1
   res << "WMZ";    // Waermemengenzaehler gesamt
   res << "WMZHz";  // Waermemengenzaehler Heizung
   res << "WMZBW";  // Waermemengenzaehler Brauchwasser
   res << "TAm";    // Mitteltemperatur (aussen)

   // Werte der ComfortPlatine
   res << "TSS";      // Temperatur Solar Speicher
   res << "TSK";      // Temperatur Solar Kollektor
   res << "TFB2";     // Temperatur Fussbodenheizung 2
   res << "TFB3";     // Temperatur Fussbodenheizung 3
   res << "TEE";      // Temperatur Externe Energiequelle
   res << "TMK2soll"; // Soll-Temperatur Mischkreis 2
   res << "TMK3soll"; // Soll-Temperatur Mischkreis 3

   res << "WMCalc";   // berechnete Waermemenge

   // elektrische Energie
   res << "Pe1";      // elektrische Leistung Verdichter
   res << "Pe2";      // elektrische Leistung Steuerung, Pumpen, ...
   res << "AZ1";      // Arbeitszahl mit Elt1
   res << "AZ2";      // Arbeitszahl mit Elt1 + Elt2
   res << "E1";       // elektrische Energie Verdichter
   res << "E2";       // elektrische Energie Steuerung, Pumpen, ....

   // Felder von DTA2 (ID >= 9000)
   res << "UEHZ";     // Ueberhitzung
   res << "UEHzsoll"; // Sollwert Ueberhitzung
   res << "Asg.VDi";  // Ansaug Verdichter
   res << "Asg.Vda";  // Ansaug Verdampfer
   res << "VDHz";     // VD Heizung

   Q_ASSERT( m_fieldCount == res.size());
   return res;
}

QString DataFile::fieldName(const quint16 &index)
{
   if(index<m_fieldCount) return fieldNames().at(index);
   return QString();
}
quint16 DataFile::fieldIndex(const QString &name)
{
   return fieldNames().indexOf(name);
}
