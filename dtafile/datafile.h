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
/*---------------------------------------------------------------------------
* - allgemeine Klasse zum Lesen und Decodieren von Daten-Dateien
* - Definition der Feldnamen und Feld-Informationen
*---------------------------------------------------------------------------*/
#ifndef DATAFILE_H
#define DATAFILE_H

#include <QObject>
#include <QMap>
#include <QVarLengthArray>

#ifdef QT_GUI_LIB
#include <QColor>
#endif

#define DATA_DS_FIELD_COUNT 76   // Felder pro Datensatz

#define MISSING_DATA_GAP 605 // Zeit (in sec) ab wann eine Luecke als fehlende
                             // Daten interpretiert wird

// Array zum Speichern der Werte eines Datensatzes
typedef QVarLengthArray<qreal> DataFieldValues;

// Map zum Speichern mehrere Datensaetze
//  Schluessel: Zeitstempel des Datensatzes
//  Wert: Werte eines Datensatzes
//  Map sortiert automatisch nach dem Schluessel
typedef QMap<quint32,DataFieldValues> DataMap;

// Klasse mit Informationen zu einem Feld
class DataFieldInfo : public QObject
{
   Q_OBJECT
public:
   //explicit DataFieldInfo(QObject *parent=0) : QObject(parent) {}
   explicit DataFieldInfo(
      QString prettyName,
      QString toolTip,
      QString category,
      bool analog,
      qreal scale,
      qreal offset,
   #ifdef QT_GUI_LIB
      QColor color,
   #endif
      QObject *parent=0);
   DataFieldInfo(const DataFieldInfo &info);
   DataFieldInfo operator=(const DataFieldInfo &info);
   QString prettyName; // schoener Name
   QString toolTip;    // erweiterte Beschreibung
   QString category;   // Kategorie des Feldes
   bool analog;        // analoges oder digitales Feld
   qreal scale;        // Skalierung des Feldes beim Darstellen
   qreal offset;       // Verschiebungs des Feldes beim Darstellen
#ifdef QT_GUI_LIB
   QColor color;       // Standardfarbe
#endif
};
typedef QList<DataFieldInfo> DataFieldInfoList;

/*---------------------------------------------------------------------------
* DataFile
*---------------------------------------------------------------------------*/
class DataFile : public QObject
{
    Q_OBJECT
public:
   explicit DataFile(QString fileName, QObject *parent = 0);
   virtual bool open(); // Daten-Datei oeffnen
   virtual void readDatasets(DataMap *data); // alle Datensaete lesen und in Map speichern
   virtual QString version(); // Datei-Version

   // Feldnamen
   static QStringList fieldNames(); // Liste mit Feldnamen
   static int fieldCount() { return m_fieldCount;} // Anzahl der Felder
   static QString fieldName(const quint16 &index); // Feldname per Index
   static quint16 fieldIndex(const QString &name); // Feldindex per Name

   // Berechnung der Feldwerte aus Arraywert
   static qreal fieldValueReal( const DataFieldValues &values, const QString &name);
   static qint32 fieldValueInt( const DataFieldValues &values, const QString &name);

   // Feldinformationen
   static const DataFieldInfoList fieldInfos();
   static const DataFieldInfo fieldInfo(const QString &name);
   static const DataFieldInfo fieldInfo(const quint16 &index);
   static const DataFieldInfo defaultFieldInfo();
   static QStringList fieldCategories();

   // String mit Fehlermeldung
   QString errorMsg;

protected:
   QString m_fileName;
   static const quint16 m_fieldCount;

private:
};

#endif // DATAFILE_H
