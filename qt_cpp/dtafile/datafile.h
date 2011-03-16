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

#define DATA_DS_FIELD_COUNT 47   // fields per dataset

// Array zum Speichern der Werte eines Datensatzes
typedef QVarLengthArray<qreal> DataFieldValues;

// Map zum Speichern mehrere Datensaetze
//  Schluessel: Zeitstempel des Datensatzes
//  Wert: Werte eines Datensatzes
//  Map sortiert automatisch nach dem Schluessel
typedef QMap<quint32,DataFieldValues> DataMap;

// Struktur mit Informationen zu einem Feld
typedef struct {
   QString prettyName; // schoener Name
   QString toolTip;    // erweiterte Beschreibung
   QString category;   // Kategorie des Feldes
   bool analog;        // analoges oder digitales Feld
   qreal scale;        // Skalierung des Feldes beim Darstellen
   qreal offset;       // Verschiebungs des Feldes beim Darstellen
#ifdef QT_GUI_LIB
   QColor color;       // Standardfarbe
#endif
} DataFieldInfo;
// Hash mit Feldinformationen
typedef QHash<QString,DataFieldInfo> DataFieldInfoHash;

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

   // Feldnamen
   static QStringList fieldNames(); // Liste mit Feldnamen
   static int fieldCount() { return m_fieldCount;} // Anzahl der Felder
   static QString fieldName(const quint16 &index); // Feldname per Index
   static quint16 fieldIndex(const QString &name); // Feldindex per Name

   // Berechnung der Feldwerte aus Arraywert
   static qreal fieldValueReal( const DataFieldValues &values, const QString &name);
   static qint32 fieldValueInt( const DataFieldValues &values, const QString &name);

   // Feldinformationen
   static const DataFieldInfo* const fieldInfo(const QString &name);
   static const DataFieldInfo* const fieldInfo(const quint16 &index);
   static QStringList fieldCategories();
   static const DataFieldInfo* const defaultFieldInfo();

protected:
   QString m_fileName;
   static const quint16 m_fieldCount = DATA_DS_FIELD_COUNT;

private:

   // initialisierung statischer Variablen
   static QStringList initFieldList();
   static QHash<QString,quint16> initFieldHash();
   static DataFieldInfoHash initFieldInfoHash();

   // Feldnamen
   static const QString m_fieldNamesArray[DATA_DS_FIELD_COUNT];
   static const QHash<QString,quint16> m_fieldNamesHash;
   static const QStringList m_fieldNamesList;

   // Feldinformationen
   static const DataFieldInfo m_fieldInfoArray[DATA_DS_FIELD_COUNT];
   static const DataFieldInfo m_defaultFieldInfo;
   static const DataFieldInfoHash m_fieldInfoHash;
};

#endif // DATAFILE_H
