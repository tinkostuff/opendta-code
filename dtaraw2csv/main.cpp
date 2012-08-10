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
* DTA-Dateien in CSV-Format umwandeln (Rohwerte)
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
#include <QObject>
#include <QFile>
#include <QDataStream>

#define VERSION_STRING "$Rev$"

// Parameter der DTA-Datei
#define DTA_HEADER_LENGTH 8      // bytes
#define DTA1_DATASET_LENGTH 168  // bytes
#define DTA2_DATASET_LENGTH 39   // fields

// Header-Werte fuer unterschiedliche Datei-Versionen
#define DTA1_HEADER_VALUE 0x2011
#define DTA2_HEADER_VALUE 0x2328


int main(int argc, char *argv[])
{
   // Separator wechseln, wenn "," schon Decimal-Separator ist
   QChar separator = ',';
   if( QLocale::system().decimalPoint() == ',') separator = ';';

   // usage
   if(argc==1)
   {
      cout << QObject::tr("dtaraw2csv <Liste-von-DTA-Dateien>").toStdString() << endl;
      cout << QObject::tr("  Copyright (C) 2011  opendta@gmx.de, http://opendta.sf.net/").toStdString() << endl;
      cout << QObject::tr("  Version: %1").arg(VERSION_STRING).toStdString() << endl;
      cout << QObject::tr("  GNU General Public License Version 3").toStdString() << endl;
      cout << QObject::tr("  powered by Qt framework").toStdString() << endl;
      return 0;
   }

   // jede Datei einzeln bearbeiten
   for( int i=1; i<argc; i++)
   {
      QString fileName = argv[i];
      cout << QObject::tr("konvertiere Datei: ").toStdString() << fileName.toStdString() << endl;

		// gibt es die Daten
		if( !QFile::exists(fileName))
		{
			cerr << QObject::tr("  FEHLER: Datei '%1' nicht gefunden!").arg(fileName).toStdString() << endl;
			continue;
		}

		// Datei oeffnen
		QFile *dtaFile = new QFile(fileName);
		dtaFile->open(QIODevice::ReadOnly);

		// Stream erstellen
		QDataStream dtaStream;
		dtaStream.setDevice(dtaFile);
		dtaStream.setByteOrder(QDataStream::LittleEndian);

		// Dateikopf lesen
		quint32 header[2];
		dtaStream >> header[0];
		dtaStream >> header[1];

		// Kopf pruefen
		if( (header[0] != DTA1_HEADER_VALUE) && (header[0] != DTA2_HEADER_VALUE)) {
			cerr << QObject::tr("  FEHLER: DTA-Version %1 wird z.Z. noch nicht unterstuetzt! Bei Interesse bitte die DTA-Datei an opendta@gmx.de schicken.").arg(header[0]).toStdString() << endl;
			delete dtaFile;
			continue;
		}

		// Ausgabedatei oeffnen
		QFile fOut(fileName+".raw.csv");
		if (!fOut.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			cerr << QObject::tr("FEHLER: beim \326ffnen der CSV-Datei '%1'!").arg(fileName+".raw.csv").toStdString() << endl;
			delete dtaFile;
			continue;
		}
		QTextStream out(&fOut);

		// Kopf schreiben
		out << QObject::tr("Dateikopf:") << separator
			<< header[0] << separator
			<< header[1] << separator
			<< endl 
			<< endl;

		if( header[0] == DTA1_HEADER_VALUE) {
			//---------------------------------------
			// DTA Version 1.x
			//---------------------------------------

			// Kopfzeile schreiben
			out << QObject::tr("Datum");
			for( int i=4; i<DTA1_DATASET_LENGTH; i+=2)
				out << separator << "[" << i << ":" << i+1 << "]";
			out << endl;

			quint16 dsCount = 0;
			while(1)
			{
				quint32 ts;
				quint16 value;
				dtaStream >> ts;

				// Ende der Datei?
				if( ts == 0) break;
				
            // Datum
			 	QDateTime timestamp = QDateTime::fromTime_t(ts);
            out << timestamp.toString("yyyy-MM-dd hh:mm:ss");

				for( int i=4; i<DTA1_DATASET_LENGTH; i+=2)
				{
					dtaStream >> value;
					out << separator << value;
				} // for i

				out << endl;
				dsCount++;

			} // while 1
			//cout << "  " << dsCount << QObject::tr(" Datensaetze gelesen").toStdString() << endl;
			
		} else if( header[0] == DTA2_HEADER_VALUE) {
			//---------------------------------------
			// DTA Version 2.x
			//---------------------------------------

			// Kopfzeile schreiben
			out << QObject::tr("Datum");
			for( int i=0; i<DTA2_DATASET_LENGTH-1; i++)
				out << separator << i;
			out << endl;

			// Datensaetze lesen
			quint16 dsCount = 0;
			while(1)
			{
				quint32 ts;
				qint32 value;
				dtaStream >> ts;

				// Ende der Datei?
				if( ts == 0) break;
				
            // Datum
			 	QDateTime timestamp = QDateTime::fromTime_t(ts);
            out << timestamp.toString("yyyy-MM-dd hh:mm:ss");

				for( int i=0; i<DTA2_DATASET_LENGTH-1; i++)
				{
					quint8 type;
					dtaStream >> type;
					switch(type)
					{
					case 0:
						// positive byte
						quint8 tmp8u;
						dtaStream >> tmp8u;
						value = tmp8u;
						break;
					case 1:
						// positive short
						qint16 tmp16;
						dtaStream >> tmp16;
						value = tmp16;
						break;
					case 4:
						// negative byte
						quint8 tmp8;
						dtaStream >> tmp8;
						value = -tmp8;
						break;
					case 5:
						// negative short
						quint16 tmp16u;
						dtaStream >> tmp16u;
						value = -tmp16u;
						break;
					default:
						cerr << QObject::tr("  FEHLER: Feldtype %1 nicht unterstuetzt! (Datensatz: %2, Feld: %3)").arg(type).arg(dsCount+1).arg(i).toStdString() << endl;
						value = 0;
					}

					out << separator << value;
				} // for i

				out << endl;
				dsCount++;

			} // while 1
			//cout << "  " << dsCount << QObject::tr(" Datensaetze gelesen").toStdString() << endl;
		}

		// Ausgabedatei schliessen
		fOut.close();

		// Datei schliessen
		delete dtaFile;

/*
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
*/
   } // foreach file

} // main
