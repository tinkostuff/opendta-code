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
* DtaStatsFrame
*  - ein bisschen Statistik mit den Daten
*  - die Funktion 'dataUpdated()' startet einen Thread, der die Hauptarbeit
*    verrichtet
*  - 'threadFinished()' sammelt die Ergebnisse ein und stellt sie dar
*---------------------------------------------------------------------------*/
#include <QtGui>
#include <QThread>

#include <QtGlobal>
#include <QtDebug>

#include "dtastatsframe.h"

/*---------------------------------------------------------------------------
* DtaStatsThread
*  - Berechnung der Statistik
*---------------------------------------------------------------------------*/
class DtaStatsThread : public QThread
{
public:
   // Constructor
   DtaStatsThread() {this->data=NULL;}
   // Zeiger auf Daten uebergeben
   void setData(DtaDataMap *data) {this->data=data;}
   // Berechnungen
   void run()
   {
      if(data==NULL || data->size()==0) return;

      // Anfang und Ende
      dataStart = data->keys().first();
      dataEnd = data->keys().last();
      datasets = data->size();

      quint32 lastTS = 0;
      bool first = true;

      // durch alle Datensaetze gehen
      DtaDataMapIterator iterator(*data);
      while(iterator.hasNext())
      {
         // naechsten Datensatz lesen
         iterator.next();

         quint32 ts = iterator.key();
         DtaFieldValues dat = iterator.value();

         // Initialisierung
         if(first)
         {
            missingSum = 0;
            missingList.clear();
            analogFields.clear();
            digitalFields.clear();
            staticAnalogFields.clear();
            staticDigitalFields.clear();

            // digitales oder analoges Feld
            for( int i=0; i<dat.size(); i++)
            {
               QString k = DtaFile::fieldName(i);
               qreal v = dat[i];
               const DtaFieldInfo *info = DtaFile::fieldInfo(i);

               if(info->analog)
               {
                  // analoge Felder: min, max, Durchschnitt, Median, Standardabweichung
                  analogFields << k;
                  anaMinValues[k] = v;
                  anaMaxValues[k] = v;
                  anaSumValues[k] = 0.0;
                  anaAvgValues[k] = 0.0;
                  anaMedianValues[k] = 0.0;
                  anaStdValues[k] = 0.0;
                  analogValues[k].clear();
                  analogValues[k] << v;
               }
               else
               {
                  // digitale Felder: Anzahl von Impulsen, Zeit (in)aktive, Durschnittliche Laufzeit
                  digitalFields << k;
                  digActivityValues[k] = 0;
                  digFirstActivity[k] = true;
                  digLastValues[k] = qRound(v);
                  digRuntimeValues[k] = 0;
                  digTotalRuntimeValues[k] = 0;
                  digPulseLists.insert(k, QList<QList<qint32> >());
                  digPulseLists[k].append(QList<qint32>());
                  digPulseLists[k].append(QList<qint32>());
               }
            }
         }
         else // !first
         {
            // Luecken suchen
            bool missingFound = false;
            if(ts-lastTS > 62) // 62 Sekunden fuer ein bisschen Spielraum
            {
               missingList << ts;
               missingSum += ts - lastTS - 60;
               missingFound = true;
            }

            // analoge Signale bearbeiten
            for( int i=0; i<analogFields.size(); i++)
            {
               QString field = analogFields.at(i);
               qreal value = DtaFile::fieldValueReal(dat,field);
               if( value < anaMinValues[field]) anaMinValues[field]=value;
               if( value > anaMaxValues[field]) anaMaxValues[field]=value;
               anaSumValues[field] += value;
               analogValues[field] << value;
            }

            // digitale Signale bearbeiten
            for( int i=0; i<digitalFields.size(); i++)
            {
               QString field = digitalFields.at(i);
               qint32 value = DtaFile::fieldValueInt(dat,field);
               if(!missingFound)
               {
                  qint32 last = digLastValues[field];
                  digRuntimeValues[field] += ts - lastTS;
                  if( (last==0) && (value==1))
                     digActivityValues[field] += 1;
                  if(last != value)
                  {
                     if(!digFirstActivity[field])
                     {
                        digPulseLists[field][last] << digRuntimeValues[field];
                        digTotalRuntimeValues[field] += digRuntimeValues[field];
                     }
                     digRuntimeValues[field] = 0;
                     digFirstActivity[field] = false;
                  }
               }
               else
               {
                  digRuntimeValues[field] = 0;
                  digFirstActivity[field] = true;
               }
               digLastValues[field] = value;
            }
         } // !first

         // letzten Zeitstempel merken
         lastTS = ts;
         first = false;
      } // while *data

      // Nachbearbeitung analoge Signale
      for( int i=0; i<analogFields.size(); i++)
      {
         QString field = analogFields.at(i);
         if( anaMinValues[field] == anaMaxValues[field])
         {
            staticAnalogFields << field;
            continue;
         }

         // Durchschnitt
         anaAvgValues[field] = anaSumValues[field]/qreal(datasets);
         // Median
         qSort(analogValues[field]);
         quint32 n = analogValues[field].size();
         if( n == 0)
            anaMedianValues[field] = 0.0;
         else if( n%2 == 1)
            anaMedianValues[field] = analogValues[field].at(n/2);
         else
            anaMedianValues[field] = (analogValues[field].at(n/2-1) + analogValues[field].at(n/2))/2.0;
      }

      // Nachbearbeitung digitalen Signale
      for( int i=0; i<digitalFields.size(); i++)
      {
         QString field = digitalFields.at(i);
         if( digActivityValues[field] == 0)
         {
            staticDigitalFields << field;
            continue;
         }

         digMinValues.insert(field, QList<qint32>());
         digMaxValues.insert(field, QList<qint32>());
         digSumValues.insert(field, QList<qint32>());
         digAvgValues.insert(field, QList<qreal>());

         for( int lh=0; lh<=1; lh++)
         {
            QList<qint32> pulseList = digPulseLists[field][lh];
            if(pulseList.size()==0) pulseList.append(0);

            // min, max und Summe
            for( int j=0; j<pulseList.size(); j++)
            {
               qint32 value = pulseList.at(j);
               if(j==0)
               {

                  digMinValues[field] << value;
                  digMaxValues[field] << value;
                  digSumValues[field] << value;
                  digAvgValues[field] << 0.0;
               }
               else
               {
                  if( value < digMinValues[field][lh]) digMinValues[field][lh] = value;
                  if( value > digMaxValues[field][lh]) digMaxValues[field][lh] = value;
                  digSumValues[field][lh] += value;
               }
            } // Eintraege in pulseList

            // Durchschnitt
            digAvgValues[field][lh] = qreal(digSumValues[field][lh]) / qreal(pulseList.size());
         }

         digOnValues[field]  = 100.0 * qreal(digSumValues[field][1]) / qreal(digTotalRuntimeValues[field]);
         digOffValues[field] = 100.0 * qreal(digSumValues[field][0]) / qreal(digTotalRuntimeValues[field]);

      }

      // statische Signal aus Liste entfernen
      for( int i=0; i<staticAnalogFields.size(); i++)
         analogFields.removeOne(staticAnalogFields.at(i));
      for( int i=0; i<staticDigitalFields.size(); i++)
         digitalFields.removeOne(staticDigitalFields.at(i));


      // Standardabweichung
      // zweiter Druchlauf durch die Daten noetig
      if(datasets > 1)
      {
         DtaDataMapIterator iterator(*data);
         while(iterator.hasNext())
         {
            // naechsten Datensatz lesen
            iterator.next();

            DtaFieldValues dat = iterator.value();
            for( int i=0; i<analogFields.size(); i++)
            {
               QString field = analogFields.at(i);
               qreal value = DtaFile::fieldValueReal(dat,field);
               anaStdValues[field] += qPow(value - anaAvgValues[field],2);
            }
         }
         for( int i=0; i<analogFields.size(); i++)
         {
            QString field = analogFields.at(i);
            anaStdValues[field] = qSqrt( anaStdValues[field] / qreal(datasets-1));
         }
      } // if datasets > 1

      // Felder sortieren
      analogFields.sort();
      staticAnalogFields.sort();
      digitalFields.sort();
      staticDigitalFields.sort();
   }

   QStringList analogFields;
   QStringList digitalFields;
   QStringList staticAnalogFields;
   QStringList staticDigitalFields;
   quint32 dataStart;
   quint32 dataEnd;
   quint32 datasets;
   QList<quint32> missingList;
   quint32 missingSum;

   // fuer analoge Felder
   QHash<QString,qreal> anaMinValues;
   QHash<QString,qreal> anaMaxValues;
   QHash<QString,qreal> anaAvgValues;
   QHash<QString,qreal> anaMedianValues;
   QHash<QString,qreal> anaStdValues;

   // fuer digitale Felder
   QHash<QString,qint32> digActivityValues;
   QHash<QString,bool> digFirstActivity;
   QHash<QString,qint32> digLastValues;
   QHash<QString,QList<qint32> > digMinValues;
   QHash<QString,QList<qint32> > digMaxValues;
   QHash<QString,QList<qreal> > digAvgValues;
   QHash<QString,qreal> digOnValues;
   QHash<QString,qreal> digOffValues;

private:
   DtaDataMap *data;

   // analog
   QHash<QString,qreal> anaSumValues;
   QHash<QString,QList<qreal> > analogValues;

   // digital
   QHash<QString,QList<qint32> > digSumValues;
   QHash<QString,QList< QList<qint32> > > digPulseLists;
   QHash<QString,qint32> digRuntimeValues;
   QHash<QString,qint32> digTotalRuntimeValues;

   // Verdichter-Starts
   QList<qreal> runTASum;
};


/*---------------------------------------------------------------------------
* DtaStatsFrame
*  - Darstellung der Ergebnisse
*---------------------------------------------------------------------------*/
DtaStatsFrame::DtaStatsFrame(QWidget *parent) :
    QFrame(parent)
{
   this->data = NULL;
   this->thread = NULL;

   textEdit = new QTextEdit(this);
   textEdit->setReadOnly(true);

   QVBoxLayout *layout = new QVBoxLayout(this);
   layout->addWidget(textEdit);
   this->setLayout(layout);
}

// destrctor
DtaStatsFrame::~DtaStatsFrame()
{
   if(this->thread)
   {
      delete this->thread;
      this->thread = NULL;
   }
}

// Zeiger auf die Daten setzen
void DtaStatsFrame::setData(DtaDataMap *data)
{
   this->data = data;
}

/*---------------------------------------------------------------------------
* Daten wurden aktualisiert : Thread erstellen und starten
*---------------------------------------------------------------------------*/
void DtaStatsFrame::dataUpdated()
{
   textEdit->clear();
   textEdit->insertPlainText(tr("Bitte warten! Daten werden ausgewertet."));

   this->thread = new DtaStatsThread();
   this->thread->setData(data);
   connect( thread, SIGNAL(finished()), this, SLOT(threadFinished()));
   connect( thread, SIGNAL(terminated()), this, SLOT(threadTerminated()));
   this->thread->start();
}

/*---------------------------------------------------------------------------
* Thread wurde abgebrochen
*---------------------------------------------------------------------------*/
void DtaStatsFrame::threadTerminated()
{
   textEdit->clear();
   textEdit->insertPlainText(tr("Fehler beim Auswerten der Daten!"));

   delete this->thread;
   this->thread = NULL;
}

/*---------------------------------------------------------------------------
* Thread wurde beendet
*  - Statistik darstellen
*---------------------------------------------------------------------------*/
void DtaStatsFrame::threadFinished()
{
   textEdit->clear();
   if(data==NULL || data->size()==0)
   {
      delete this->thread;
      this->thread = NULL;
      return;
   }

   // HTML ammeln und in einem Stueck in TextEdit einfuegen
   QStringList html;

   // HTML-Kopf
   html << "<html>";
   html << "  <head>";
   html << "    <style type=\"text/css\">";
   html << "      p{ font-family: ,sans-serif; font-size: small;}";
   html << "      td{ font-family: ,sans-serif; font-size: medium; padding-left: 7px; padding-right: 7px;}";
   html << "      th{ font-family: ,sans-serif; font-size: medium; font-weight: bold; padding-left: 7px; padding-right: 7px;}";
   html << "      h1{ font-family: ,sans-serif; font-size: large; margin-top: 20px; margin-bottom: 5px;}";
   html << "    </style>";
   html << "  </head>";
   html << "  <body>";

   // allgemeine Angaben
   html << tr("<h1>Allgemeine Informationen</h1>");
   html << "<table cellpadding=\"2\" cellspacing=\"1\" border=\"0\">";
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Datens\344tze"))
                        .arg(thread->datasets);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Daten Start"))
                        .arg(QDateTime::fromTime_t(thread->dataStart).toString("yyyy-MM-dd hh:mm"));
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Daten Ende"))
                        .arg(QDateTime::fromTime_t(thread->dataEnd).toString("yyyy-MM-dd hh:mm"));
   qint32 delta = thread->dataEnd - thread->dataStart + 120;
   QString s = QString(tr("%1 Tage %2 Stunden %3 Minuten"))
                        .arg(delta/86400)
                        .arg((delta%86400)/3600)
                        .arg((delta%3600)/60);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Zeitraum"))
                        .arg(s);
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Anzahl Aufzeichnungsl\374cken"))
                        .arg(thread->missingList.size());
   s = QString(tr("%1 Tage %2 Stunden %3 Minuten"))
                        .arg(thread->missingSum/86400)
                        .arg((thread->missingSum%86400)/3600)
                        .arg((thread->missingSum%3600)/60);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Summe Aufzeichnungsl\374cken"))
                        .arg(s);
   html << "</table>";

   // analoge Felder
   html << "<h1>analoge Signale</h1>";
   html << "<table cellpadding=\"2\" cellspacing=\"0\" border=\"1\">";
   html << QString("<tr><th align=left>%1</th><th align=center>%2</th><th align=center>%3</th><th align=center>%4</th><th align=center>%5</th><th align=center>%6</th></tr>")
                      .arg(tr("Signalname"))
                      .arg(tr("Minimum"))
                      .arg(tr("Maximum"))
                      .arg(tr("Mittelwert"))
                      .arg(tr("Median"))
                      .arg(tr("Standardabweichung"));
   for( int i=0; i<thread->analogFields.size(); i++)
   {
      QString field = thread->analogFields.at(i);

      QString fieldPretty = field;
      if( field[0] == 'A')
         fieldPretty += " [V]";
      else if( field[0] == 'T')
         fieldPretty += " [\260C]";
      else if( field == "DF")
         fieldPretty += " [l/min]";
      else if( field == "Qh")
         fieldPretty += " [kW]";
      else if( field.startsWith("Sp"))
         fieldPretty += " [K]";

      QString lineColor = i%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%7\"><td align=left>%1</td><td align=center>%2</td><td align=center>%3</td><td align=center>%4</td><td align=center>%5</td><td align=center>%6</td></tr>")
            .arg(fieldPretty)
            .arg(thread->anaMinValues[field],0,'f',1,' ')
            .arg(thread->anaMaxValues[field],0,'f',1,' ')
            .arg(thread->anaAvgValues[field],0,'f',1,' ')
            .arg(thread->anaMedianValues[field],0,'f',1,' ')
            .arg(thread->anaStdValues[field],0,'f',1,' ')
            .arg(lineColor);
   }
   html << "</table>";

   // digitale Felder
   html << "<h1>digitale Signale</h1>";
   html << "<table cellpadding=\"2\" cellspacing=\"0\" border=\"1\">";
   html << QString("<tr><th align=left>%1</th><th align=center>%2</th><th align=center>%3</th><th align=center>%4</th><th align=center>%5</th><th align=center>%6</th><th align=center>%7</th><th align=center>%8</th><th align=center>%9</th><th align=center>%10</th></tr>")
                      .arg(tr("Signalname"))
                      .arg(tr("Impulse"))
                      .arg(tr("Aktiv"))
                      .arg(tr("Mittelwert Ein"))
                      .arg(tr("Minimum Ein"))
                      .arg(tr("Maximum Ein"))
                      .arg(tr("Inaktiv"))
                      .arg(tr("Mittelwert Aus"))
                      .arg(tr("Minimum Aus"))
                      .arg(tr("Maximum Aus"));
   for( int i=0; i<thread->digitalFields.size(); i++)
   {
      QString field = thread->digitalFields.at(i);
      QString lineColor = i%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%11\"><td align=left>%1</td><td align=center>%2</td><td align=center>%3</td><td align=center>%4</td><td align=center>%5</td><td align=center>%6</td><td align=center>%7</td><td align=center>%8</td><td align=center>%9</td><td align=center>%10</td></tr>")
            .arg(field)
            .arg(thread->digActivityValues[field],6)
            .arg(QString(tr("%1&nbsp;%")  ).arg(thread->digOnValues[field],5,'f',1,' '))
            .arg(QString(tr("%1&nbsp;min")).arg(thread->digAvgValues[field][1]/60.0,6,'f',0,' '))
            .arg(QString(tr("%1&nbsp;min")).arg(thread->digMinValues[field][1]/60,7))
            .arg(QString(tr("%1&nbsp;min")).arg(thread->digMaxValues[field][1]/60,7))
            .arg(QString(tr("%1&nbsp;%")  ).arg(thread->digOffValues[field],5,'f',1,' '))
            .arg(QString(tr("%1&nbsp;min")).arg(thread->digAvgValues[field][0]/60,6,'f',0,' '))
            .arg(QString(tr("%1&nbsp;min")).arg(thread->digMinValues[field][0]/60,7))
            .arg(QString(tr("%1&nbsp;min")).arg(thread->digMaxValues[field][0]/60,7))
            .arg(lineColor);
   }
   html << "</table>";

   // statische Felder
   html << "<h1>statische Signale</h1>";
   html << "<table cellpadding=\"2\" cellspacing=\"0\" border=\"1\">";
   html << QString("<tr><th align=left>%1</th><th align=center>%2</th></tr>")
                      .arg(tr("Signalname"))
                      .arg(tr("Wert"));
   for( int i=0; i<thread->staticAnalogFields.size(); i++)
   {
      QString field = thread->staticAnalogFields.at(i);

      QString fieldPretty = field;
      if( field[0] == 'A')
         fieldPretty += " [V]";
      else if( field[0] == 'T')
         fieldPretty += " [\260C]";

      QString lineColor = i%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%3\"><td align=left>%1</td><td align=center>%2</td></tr>")
            .arg(fieldPretty)
            .arg(thread->anaMinValues[field])
            .arg(lineColor);
   }
   for( int i=0; i<thread->staticDigitalFields.size(); i++)
   {
      QString field = thread->staticDigitalFields.at(i);
      QString lineColor = (i+thread->staticAnalogFields.size())%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%3\"><td align=left>%1</td><td align=center>%2</td></tr>")
            .arg(field)
            .arg(thread->digLastValues[field])
            .arg(lineColor);
   }
   html << "</table>";

   // Fuss
   html << "    <p>DtaGui - http://opendta.sourceforge.net/ - opendta@gmx.de</p>";
   html << "  </body>";
   html << "</html>";
   textEdit->setHtml(html.join("\n"));

   delete this->thread;
   this->thread = NULL;
}

/*---------------------------------------------------------------------------
* Drucken
*---------------------------------------------------------------------------*/
void DtaStatsFrame::print(QPrinter *printer)
{
   textEdit->print(printer);
}
