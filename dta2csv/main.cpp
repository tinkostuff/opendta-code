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
* DTA-Dateien in CSV-Format umwandeln
* Befehl: dta2csv <Liste-mit-DTA-Dateien>
*---------------------------------------------------------------------------*/

#include <iostream>
using namespace std;

#include <QtGlobal>
#include <QDebug>
#include <QLocale>
#include <QStringList>
#include <QDateTime>

#define VERSION_STRING "$Rev$"

#include "dtafile/dtafile.h"

/*---------------------------------------------------------------------------
* Daten in TextStream ausgeben
*---------------------------------------------------------------------------*/
void outputData( QIODevice *file, const DataMap *data)
{
   if (data->isEmpty()) return;

   QTextStream out(file);

   // Separator wechseln, wenn "," schon Decimal-Separator ist
   QChar separator = ',';
   if( QLocale::system().decimalPoint() == ',') separator = ';';

   // Kopfzeile
   out << QObject::tr("Datum/Uhrzeit") << separator << QObject::tr("Zeitstempel");
   for( int i=0; i<DataFile::fieldCount(); i++)
      out << separator << DataFile::fieldInfo(i).prettyName;
   out << endl;

   // Daten schreiben
   DataMap::const_iterator iterator = data->constBegin();
   do
   {
      QDateTime timestamp = QDateTime::fromTime_t(iterator.key());
      DataFieldValues values = iterator.value();

      // Datum
      out << timestamp.toString("yyyy-MM-dd hh:mm:ss")
          << separator
          << iterator.key();

      // Felder
      for( int i=0; i<values.size(); i++)
         out << separator << QLocale::system().toString(values[i]);
      out << "\n"; // nicht endl, da endl alles aus dem Puffer in die Datei schreibt

      // naechster Datensatz
      iterator++;
   } while( iterator != data->constEnd());
}

/*---------------------------------------------------------------------------
* MAIN
*---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
   QStringList args;
   for( int i=1; i<argc; i++) args << argv[i];

   // usage
   if( (argc==1) || ((argc==2) && (args.at(0)=="--stdout")))
   {
      cerr << QObject::tr("dta2csv [--stdout] <Liste-von-DTA-Dateien>").toStdString() << endl;
      cerr << QObject::tr("  Copyright (C) 2014  opendta@gmx.de, http://opendta.sf.net/").toStdString() << endl;
      cerr << QObject::tr("  Version: %1").arg(VERSION_STRING).toStdString() << endl;
      cerr << QObject::tr("  GNU General Public License Version 3").toStdString() << endl;
      cerr << QObject::tr("  powered by Qt framework").toStdString() << endl;
      return 0;
   }

   // globale variablen
   bool output2stdout = false;

   // Argumente untersuchen
   if (args.at(0) == "--stdout")
   {
      cerr << "Ausgabe wird nach stdout umgeleitet!" << endl;
      output2stdout = true;
      args.takeFirst();
   }

   // jede Datei einzeln bearbeiten
   DataMap data;
   for( int i=0; i<args.size(); i++)
   {
      QString fileName = args.at(i);
      cerr << QObject::tr("konvertiere Datei: ").toStdString() << fileName.toStdString() << endl;

      // Datei oeffnen
      DtaFile *dta = new DtaFile(fileName);
      if( !dta->open())
      {
         // Fehlertext wird beireits in DtaFile geschrieben
         delete dta;
         continue;
      }

      // Datei einlesen
      dta->readDatasets(&data);

      if( !output2stdout && !data.isEmpty())
      {
         // Ausgabedatei oeffnen
         QFile fOut(fileName+".csv");
         if (!fOut.open(QIODevice::WriteOnly | QIODevice::Text))
         {
            qWarning() << QString(QObject::tr("FEHLER: beim \326ffnen der CSV-Datei '%1'!")).arg(fileName+".csv");
            delete dta;
            continue;
         }

         // Daten schreiben
         outputData( &fOut, &data);

         // Ausgabedatei schliessen
         fOut.close();

         // Daten loeschen
         data.clear();

      } // if !output2stdout && !data.isEmpty

      delete dta;
   } // foreach file

   // Ausgabe nach stdout
   if (output2stdout)
   {
      cerr << "schreibe Ergebnis" << endl;
      QFile fOut;
      //fOut.open( stdout, QIODevice::WriteOnly | QIODevice::Text);
      fOut.open( stdout, QIODevice::WriteOnly);
      outputData( &fOut, &data);
      fOut.close();
   }

} // main
