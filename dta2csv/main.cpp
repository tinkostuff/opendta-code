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
* DTA-Dateien in CSV-Format umwandeln
* Befehl: dta2csv <Liste-mit-DTA-Dateien>
*---------------------------------------------------------------------------*/

#include <iostream>
using namespace std;

#include <QtCore/QCoreApplication>
#include <QtGlobal>
#include <QDebug>
#include <QLocale>
#include <QStringList>
#include <QDateTime>

#define VERSION_STRING "$Rev$"

#include "dtafile/dtafile.h"

int main(int argc, char *argv[])
{
   // Separator wechseln, wenn "," schon Decimal-Separator ist
   QChar separator = ',';
   if( QLocale::system().decimalPoint() == ',') separator = ';';

   // usage
   if(argc==1)
   {
      cout << QObject::tr("dta2csv <Liste-von-DTA-Dateien>").toStdString() << endl;
      cout << QObject::tr("  Copyright (C) 2011  opendta@gmx.de, http://opendta.sf.net/").toStdString() << endl;
      cout << QObject::tr("  Version: %1").arg(VERSION_STRING).toStdString() << endl;
      cout << QObject::tr("  GNU General Public License Version 3").toStdString() << endl;
      cout << QObject::tr("  powered by Qt framework").toStdString() << endl;
      return 0;
   }

   // jede Datei einzeln bearbeiten
   for( int i=1; i<argc; i++)
   {
      DataMap data;
      QString fileName = argv[i];
      cout << QObject::tr("konvertiere Datei: ").toStdString() << fileName.toStdString() << endl;

      // Datei oeffnen
      DtaFile *dta = new DtaFile(fileName);
      if( !dta->open())
      {
         qWarning() << QString(QObject::tr("FEHLER: beim \326ffnen der DTA-Datei '%1'!")).arg(fileName);
         delete dta;
         continue;
      }

      // Datei einlesen
      dta->readDatasets(&data);

      if( !data.isEmpty())
      {
         // Ausgabedatei oeffnen
         QFile fOut(fileName+".csv");
         if (!fOut.open(QIODevice::WriteOnly | QIODevice::Text))
         {
            qWarning() << QString(QObject::tr("FEHLER: beim \326ffnen der CSV-Datei '%1'!")).arg(fileName+".csv");
            delete dta;
            continue;
         }
         QTextStream out(&fOut);

         // Kopfzeile
         out << QObject::tr("Datum") << separator;
         for( int i=0; i<DataFile::fieldCount(); i++)
            out << DataFile::fieldInfo(i)->prettyName << separator;
         out << endl;

         // Daten schreiben
         DataMap::const_iterator iterator = data.constBegin();
         do
         {
            QDateTime timestamp = QDateTime::fromTime_t(iterator.key());
            DataFieldValues values = iterator.value();

            // Datum
            out << timestamp.toString("yyyy-MM-dd hh:mm:ss") << separator;
            // Felder
            for( int i=0; i<values.size(); i++)
               out << QLocale::system().toString(values[i]) << separator;
            out << endl;

            // naechster Datensatz
            iterator++;
         } while( iterator != data.constEnd());

         // Ausgabedatei schliessen
         fOut.close();

      } // if !data.isEmpty

      delete dta;
   } // foreach file

} // main
