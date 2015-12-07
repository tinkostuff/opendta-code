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

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFrame>
#include <QLabel>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>

#include <QHeaderView>

#include "dtacompstartsframe.h"
#include "statistics/dtacompstartsstatistics.h"

/*---------------------------------------------------------------------------
* DtaCompStartsThread - Thread zum Berechnen der Kompressor Starts
*---------------------------------------------------------------------------*/
class DtaCompStartsThread : public QThread
{
public:
   DtaCompStartsThread() {data=NULL; tsStart = 0; tsEnd = 0; stats=new DtaCompStartsStatistics();}
   DtaCompStartsThread(DataMap *data, quint32 tsStart, quint32 tsEnd, QObject *parent=0) : QThread(parent)
   {
      this->data=data;
      this->tsStart = tsStart;
      this->tsEnd = tsEnd;
      stats = new DtaCompStartsStatistics();
   }
   ~DtaCompStartsThread() { if(stats!=NULL) delete stats;}
   // Zeiger auf Daten uebergeben
   void setData(DataMap *data) {this->data=data;}
   // Zeitspanne setzen
   void setDateTimeRange( quint32 start, quint32 end) { this->tsStart=start; this->tsEnd=end;}
   // Daten analysieren
   void run()
   {
      if(data==NULL || data->size()==0) return;

      DataMap::const_iterator iteratorEnd = data->upperBound(tsEnd);
      DataMap::const_iterator iteratorStart = data->lowerBound(tsStart);
      stats->calcStatistics( iteratorStart, iteratorEnd);
   } // run()

   DtaCompStartsStatistics *stats;
private:
   DataMap *data;
   quint32 tsStart;
   quint32 tsEnd;
};

/*---------------------------------------------------------------------------
* DtaCompStatsFrame - Daten darstellen
*---------------------------------------------------------------------------*/
DtaCompStartsFrame::DtaCompStartsFrame(DataMap *data, QWidget *parent) :
    QFrame(parent)
{
   this->data = data;
   this->thread = NULL;
   this->m_runs.clear();

   //
   // GUI
   //

   // Hauptlayout
   QVBoxLayout *layoutMain = new QVBoxLayout(this);

   // GroupBox mit Schaltern
   QGroupBox *gbButtons = new QGroupBox();
   QHBoxLayout *layoutGbButtons = new QHBoxLayout(gbButtons);
   layoutGbButtons->setSpacing(5);

   // Schalter
   QPushButton *btnPrint = new QPushButton( QIcon(":/images/images/print.png"), tr("&Drucken..."));
   connect( btnPrint, SIGNAL(clicked()), this, SLOT(print()));
   layoutGbButtons->addWidget(btnPrint);

   QPushButton *btnRefresh = new QPushButton( QIcon(":/images/images/refresh.png"), tr("&Aktualisieren"));
   connect( btnRefresh, SIGNAL(clicked()), this, SLOT(dataUpdated()));
   layoutGbButtons->addWidget(btnRefresh);

   // Separator
   QFrame *frameLine;
   frameLine = new QFrame();
   frameLine->setFrameShape(QFrame::VLine);
   frameLine->setFrameShadow(QFrame::Sunken);
   layoutGbButtons->addWidget(frameLine);

   // Zeitspanne festlegen
   layoutGbButtons->addWidget(new QLabel(tr("<b>Zeitspanne:</b>")));
   QDateTime dtStart = QDateTime::fromTime_t(0);
   QDateTime dtEnd = QDateTime::fromTime_t(0);

   layoutGbButtons->addWidget(new QLabel(tr("Begin:")));
   dteStart = new QDateTimeEdit(dtStart);
   dteStart->setDateTimeRange( dtStart, dtEnd);
   dteStart->setCalendarPopup(true);
   dteStart->setDisplayFormat(tr("yyyy-MM-dd hh:mm:ss"));
   layoutGbButtons->addWidget(dteStart);

   layoutGbButtons->addWidget(new QLabel(tr("Ende:")));
   dteEnd = new QDateTimeEdit(dtEnd);
   dteEnd->setDateTimeRange( dtStart, dtEnd);
   dteEnd->setCalendarPopup(true);
   dteEnd->setDisplayFormat(tr("yyyy-MM-dd hh:mm:ss"));
   layoutGbButtons->addWidget(dteEnd);

   QPushButton *btnZoomFit = new QPushButton( QIcon(":/images/images/zoom-fit.png"), tr("gesamter Bereich"));
   connect( btnZoomFit, SIGNAL(clicked()), this, SLOT(setCompleteTimeRange()));
   layoutGbButtons->addWidget(btnZoomFit);

   this->updateTimeRangeEdit();

   // Spacer
   QSpacerItem *spacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
   layoutGbButtons->addItem(spacer);

   layoutMain->addWidget(gbButtons);

   QLabel *label = new QLabel(tr("Modus:"), this);
   cbModus = new QComboBox(this);
   cbModus->setMinimumWidth(200);

   frameLine = new QFrame();
   frameLine->setFrameShape(QFrame::VLine);
   frameLine->setFrameShadow(QFrame::Sunken);

   QPushButton *btnCSV = new QPushButton( QIcon(":/images/images/csv.png"), tr("Daten als CSV speichern ..."));
   connect( btnCSV, SIGNAL(clicked()), this, SLOT(saveAsCSV()));

   QHBoxLayout *layout1 = new QHBoxLayout();
   layout1->addWidget(label);
   layout1->addWidget(cbModus);
   layout1->addWidget(frameLine);
   layout1->addWidget(btnCSV);
   layout1->addStretch();

   table = new QTableWidget(this);
   table->setStyleSheet(
         "QHeaderView::section {"
            "padding-bottom: 3px;"
            "padding-top: 3px;"
            "padding-left: 5px;"
            "padding-right: 5px;"
            "margin: 0px;"
         "}"
         "QTableView::item {"
           "padding-bottom: 3px;"
           "padding-top: 3px;"
           "padding-left: 5px;"
           "padding-right: 5px;"
        "}"
         );
   table->setEditTriggers(QAbstractItemView::NoEditTriggers);

   QVBoxLayout *layout2 = new QVBoxLayout();
   layout2->addLayout(layout1);
   layout2->addWidget(table);

   QGroupBox *groupBox = new QGroupBox(this);
   groupBox->setLayout(layout2);

   textEdit = new QTextEdit(this);
   textEdit->setReadOnly(true);

   layoutMain->addWidget(textEdit);
   layoutMain->addWidget(groupBox);

   // Signale verbinden
   connect( cbModus, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRunTable(int)));
   connect( table->horizontalHeader(), SIGNAL(sectionClicked(int)), table, SLOT(sortByColumn(int)));
}

/*---------------------------------------------------------------------------
* Destructor
*---------------------------------------------------------------------------*/
DtaCompStartsFrame::~DtaCompStartsFrame()
{
   if(this->thread)
   {
      delete this->thread;
      this->thread = NULL;
   }
}

/*---------------------------------------------------------------------------
* Daten wurden aktualisiert
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::dataUpdated()
{
   // Anzeige loeschen
   textEdit->clear();
   this->cbModus->clear();
   this->table->setRowCount(0);
   this->table->setColumnCount(0);

   if(data->isEmpty()) textEdit->insertPlainText("");
   else textEdit->insertPlainText(tr("Bitte warten! Daten werden ausgewertet."));

   // Zeitspanne der Eingabefelder aktualisieren
   this->updateTimeRangeEdit();

   // Thread starten
   if(!data->isEmpty())
   {
      this->thread = new DtaCompStartsThread();
      this->thread->setData(data);
      this->thread->setDateTimeRange( dteStart->dateTime().toTime_t(),
                                      dteEnd->dateTime().toTime_t());
      connect( thread, SIGNAL(finished()), this, SLOT(threadFinished()));
      this->thread->start();
   }
}

/*---------------------------------------------------------------------------
* Thread wurde beendete - Daten darstellen
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::threadFinished()
{
   //
   // textEdit
   //

   textEdit->clear();
   if(data==NULL || data->size()==0)
   {
      textEdit->insertPlainText(tr("Fehler beim Auswerten der Daten!"));
      delete this->thread;
      this->thread = NULL;
      return;
   }

   DtaCompStartsStatistics *stats = thread->stats;

   // HTML in einem Zug einfuegen -> sonst wird der Text nicht korrekt angezeigt
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
                        .arg(tr("Datensätze"))
                        .arg(stats->datasets());
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Daten Start"))
                        .arg(QDateTime::fromTime_t(stats->timeStart()).toString("yyyy-MM-dd hh:mm:ss"));
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Daten Ende"))
                        .arg(QDateTime::fromTime_t(stats->timeEnd()).toString("yyyy-MM-dd hh:mm:ss"));
   quint32 delta = stats->timeRange();
   QString s = QString(tr("%1 Tage %2 Stunden %3 Minuten %4 Sekunden"))
                        .arg(delta/86400)
                        .arg((delta%86400)/3600)
                        .arg((delta%3600)/60)
                        .arg(delta%60);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Zeitraum"))
                        .arg(s);
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Anzahl Aufzeichnungslücken"))
                        .arg(stats->missingCount());
   s = QString(tr("%1 Tage %2 Stunden %3 Minuten"))
                        .arg(stats->missingSum()/86400)
                        .arg((stats->missingSum()%86400)/3600)
                        .arg((stats->missingSum()%3600)/60);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Summe Aufzeichnungslücken"))
                        .arg(s);
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Komporessor Starts"))
                        .arg(stats->runCount());
   html << "</table>";

   for( int i=0; i<DtaCompStart::modeCount(); ++i)
   {
      DtaCompStart::CompStartModes mode = (DtaCompStart::CompStartModes)i;

      html << QString(tr("<h1>Statistik für Modus %1 (%2 Starts)</h1>")).arg(DtaCompStart::modeString(mode)).arg(stats->runCount(mode));

      html << "<table cellpadding=\"2\" cellspacing=\"0\" border=\"1\">";
      html << QString("<tr><th align=left>%1</th><th align=center>%2</th><th align=center>%3</th><th align=center>%4</th><th align=center>%5</th><th align=center>%6</th></tr>")
                         .arg(tr("Signalname"))
                         .arg(tr("Minimum"))
                         .arg(tr("Maximum"))
                         .arg(tr("Mittelwert"))
                         .arg(tr("Median"))
                         .arg(tr("Standardabweichung"));

      quint32 n = 0;
      for( int j=0; j<DtaCompStart::fieldCount(); j++)
      {
         DtaCompStart::CompStartFields field = (DtaCompStart::CompStartFields)j;
         if( (field==DtaCompStart::fStart) || (field==DtaCompStart::fMode))
            continue;

         // Feldname
         QString fieldName = DtaCompStart::fieldName(field);
         QString lineColor = n%2 ? "#FFFFFF" : "#E5E5E5";
         html << QString("<tr bgcolor=\"%7\"><td align=left>%1</td><td align=center>%2</td><td align=center>%3</td><td align=center>%4</td><td align=center>%5</td><td align=center>%6</td></tr>")
               .arg(fieldName)
               .arg(stats->statMinStr(mode,field))
               .arg(stats->statMaxStr(mode,field))
               .arg(stats->statAvgStr(mode,field))
               .arg(stats->statMedianStr(mode,field))
               .arg(stats->statStdevStr(mode,field))
               .arg(lineColor);
         n++;
      } // for field
      html << "</table>";
   } // for mode

   // Fuss
   html << "    <p>DtaGui - http://opendta.sourceforge.net/ - opendta@gmx.de</p>";
   html << "  </body>";
   html << "</html>";
   textEdit->setHtml(html.join("\n"));

   //
   // Tabelle
   //
   cbModus->clear();
   cbModus->addItem( tr("alle"), -1);
   for( int i=0; i<DtaCompStart::modeStringList().size(); i++)
      cbModus->addItem( DtaCompStart::modeStringList().at(i), i);

   QStringList header;
   header << tr("Start");
   header << tr("Modus");
   header << tr("Länge\n[h:min]");
   header << tr("Pause\n[h:min]");
   header << tr("TVL\n[°C]");
   header << tr("TRL\n[°C]");
   header << tr("SpHZ\n[K]");
   header << tr("TWQein\n[°C]");
   header << tr("TWQaus\n[°C]");
   header << tr("SpWQ\n[K]");
   header << tr("TA\n[°C]");
   header << tr("WM\n[kWh]");
   header << tr("E1\n[kWh]");
   header << tr("E2\n[kWh]");
   header << tr("AZ1");
   header << tr("AZ2");

   table->setColumnCount(header.size());
   table->setRowCount(stats->runs().size());
   table->setHorizontalHeaderLabels(header);

   QTableWidgetItem *item;
   for( int i=0; i<stats->runs().size(); i++)
   {
      const DtaCompStart *run = &(stats->runs().at(i));

      // Start
      item = new QTableWidgetItem(QDateTime::fromTime_t(run->start()).toString("yyyy-MM-dd hh:mm"));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 0, item);

      // Modus
      item = new QTableWidgetItem(run->modeString());
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 1, item);

      // Laenge
      item = new QTableWidgetItem(QString("%1:%2")
                                  .arg(run->length()/3600)
                                  .arg(QString("%1").arg((run->length()%3600)/60).rightJustified(2,'0')));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 2, item);

      // Pause
      if(run->pause() > 0) s = QString("%1:%2")
                                 .arg(run->pause()/3600)
                                 .arg(QString("%1").arg((run->pause()%3600)/60).rightJustified(2,'0'));
      else s = "";
      item = new QTableWidgetItem(s);
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 3, item);

      // TVL
      item = new QTableWidgetItem(QString("%1").arg(run->TVL(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 4, item);

      // TRL
      item = new QTableWidgetItem(QString("%1").arg(run->TRL(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 5, item);

      // SpHZ
      item = new QTableWidgetItem(QString("%1").arg(run->SpHz(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 6, item);

      // TWQein
      item = new QTableWidgetItem(QString("%1").arg(run->TWQein(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 7, item);

      // TWQaus
      item = new QTableWidgetItem(QString("%1").arg(run->TWQaus(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 8, item);

      // SpWQ
      item = new QTableWidgetItem(QString("%1").arg(run->SpWQ(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 9, item);

      // TA
      item = new QTableWidgetItem(QString("%1").arg(run->TA(),0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 10, item);

      // WM
      item = new QTableWidgetItem(QString("%1").arg(run->WM(),0,'f',2));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 11, item);

      // E1
      item = new QTableWidgetItem(QString("%1").arg(run->E1(),0,'f',2));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 12, item);

      // E2
      item = new QTableWidgetItem(QString("%1").arg(run->E2(),0,'f',2));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 13, item);

      // AZ1
      item = new QTableWidgetItem(QString("%1").arg(run->AZ1(),0,'f',2));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 14, item);

      // AZ2
      item = new QTableWidgetItem(QString("%1").arg(run->AZ2(),0,'f',2));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 15, item);
   }

   table->resizeColumnsToContents();
   table->resizeRowsToContents();

   // Daten kopieren
   m_runs.clear();
   m_runs.append(thread->stats->runs());

   delete this->thread;
   this->thread = NULL;
}

/*---------------------------------------------------------------------------
* Tabelle aktualisieren
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::updateRunTable(int index)
{
   if(index < 0) return;

   QString mode = cbModus->itemText(index);

   // nicht relevante Zeilen verstecken
   for( int i=0; i<table->rowCount(); i++)
   {
      if(index == 0)
         table->setRowHidden(i, false);
      else
      {
         QString rowMode = table->item(i, 1)->text();
         table->setRowHidden(i, !(mode == rowMode));
      }
   }
}

/*---------------------------------------------------------------------------
* Eingabefelder fuer Zeitspanne aktualisieren
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::updateTimeRangeEdit()
{
   // sollen wir die Werte an den neuen Datenbereich anpassen
   bool setMinMax = true;
   if( dteStart->dateTime() != dteStart->minimumDateTime()) setMinMax = false;
   if( dteEnd->dateTime() != dteEnd->maximumDateTime()) setMinMax = false;

   // Start und Ende der Daten ermitteln
   QDateTime dtStart = QDateTime::fromTime_t(0);
   QDateTime dtEnd = QDateTime::fromTime_t(0);
   if( (data!=NULL) && !data->isEmpty())
   {
      DataMap::const_iterator iStart = data->constBegin();
      DataMap::const_iterator iEnd = data->constEnd();
      iEnd--;
      dtStart.setTime_t(iStart.key());
      dtEnd.setTime_t(iEnd.key());
   }

   // Zeitbereich setzen
   dteStart->setDateTimeRange( dtStart, dtEnd);
   dteEnd->setDateTimeRange( dtStart, dtEnd);

   // Werte veraendern?
   if(setMinMax)
   {
      dteStart->setDateTime(dtStart);
      dteEnd->setDateTime(dtEnd);
   }

}

/*---------------------------------------------------------------------------
* Werte auf komplette Zeitspanne setzen
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::setCompleteTimeRange()
{
   // Start und Ende der Daten ermitteln
   QDateTime dtStart = QDateTime::fromTime_t(0);
   QDateTime dtEnd = QDateTime::fromTime_t(0);
   if( (data!=NULL) && !data->isEmpty())
   {
      DataMap::const_iterator iStart = data->constBegin();
      DataMap::const_iterator iEnd = data->constEnd();
      iEnd--;
      dtStart.setTime_t(iStart.key());
      dtEnd.setTime_t(iEnd.key());
   }
   dteStart->setDateTime(dtStart);
   dteEnd->setDateTime(dtEnd);
}

/*---------------------------------------------------------------------------
* Drucken
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::print()
{
   QPrinter *printer = new QPrinter;
   QPrintDialog printDialog(printer, this);
   if (printDialog.exec() == QDialog::Accepted)
      textEdit->print(printer);
}

/*---------------------------------------------------------------------------
* als CSV speichern
*---------------------------------------------------------------------------*/
void DtaCompStartsFrame::saveAsCSV()
{
   // Daten vorhanden?
   if(m_runs.isEmpty())
   {
      QMessageBox::warning( this,
                            tr("keine Daten"),
                            tr("Z.Z. sind keine Daten vorhanden!<br>Deshalb kann auch nichts gespeichert werden!"));
      return;
   }

   // Dateinamen abfragen
   QString fileName = QFileDialog::getSaveFileName( this,
                                                    tr("Daten speichern"),
                                                    ".",
                                                    tr("CSV-Dateien (*.csv);;Alle Dateien (*.*)"));
   if( fileName == "") return;

   // Separator wechseln, wenn "," schon Decimal-Separator ist
   QChar separator = ',';
   if( QLocale::system().decimalPoint() == ',') separator = ';';

   // Ausgabedatei oeffnen
   QFile fOut(fileName);
   if (!fOut.open(QIODevice::WriteOnly | QIODevice::Text))
   {
      QMessageBox::critical( this,
                             tr("Fehler"),
                             tr("FEHLER: beim öffnen der CSV-Datei '%1'!").arg(fileName));
      return;
   }
   QTextStream out(&fOut);
   out.setCodec("UTF-8");
   out.setGenerateByteOrderMark(true);

   setCursor(Qt::WaitCursor);
   QCoreApplication::processEvents();

   // Kopfzeile
   out << tr("Start") << separator
       << tr("Modus") << separator
       << tr("Länge [h:min]") << separator
       << tr("Pause [h:min]") << separator
       << tr("TVL [°C]") << separator
       << tr("TRL [°C]") << separator
       << tr("SpHZ [K]") << separator
       << tr("TWQein [°C]") << separator
       << tr("TWQaus [°C]") << separator
       << tr("SpWQ [K]") << separator
       << tr("TA [°C]") << separator
       << tr("WM [kWh]") << separator
       << tr("E1 [kWh]") << separator
       << tr("E2 [kWh]") << separator
       << tr("AZ1") << separator
       << tr("AZ2") << separator
		 << endl;

   QListIterator<DtaCompStart> i(m_runs);
   while(i.hasNext())
   {
      DtaCompStart run = i.next();
      out << QDateTime::fromTime_t(run.start()).toString("yyyy-MM-dd hh:mm") << separator;
      out << run.modeString() << separator;
      out << QString("%1:%2").arg(run.length()/3600).arg(QString("%1").arg((run.length()%3600)/60).rightJustified(2,'0')) << separator;
      if(run.pause() > 0) out << QString("%1:%2").arg(run.pause()/3600).arg(QString("%1").arg((run.pause()%3600)/60).rightJustified(2,'0'));
      out << separator;
      out << QString("%1").arg(run.TVL(),0,'f',1) << separator;
      out << QString("%1").arg(run.TRL(),0,'f',1) << separator;
      out << QString("%1").arg(run.SpHz(),0,'f',1) << separator;
      out << QString("%1").arg(run.TWQein(),0,'f',1) << separator;
      out << QString("%1").arg(run.TWQaus(),0,'f',1) << separator;
      out << QString("%1").arg(run.SpWQ(),0,'f',1) << separator;
      out << QString("%1").arg(run.TA(),0,'f',1) << separator;
      out << QString("%1").arg(run.WM(),0,'f',2) << separator;
      out << QString("%1").arg(run.E1(),0,'f',2) << separator;
      out << QString("%1").arg(run.E2(),0,'f',2) << separator;
      out << QString("%1").arg(run.AZ1(),0,'f',2) << separator;
      out << QString("%1").arg(run.AZ2(),0,'f',2) << separator;
      out << endl;
   }

   // Ausgabedatei schliessen
   fOut.close();

   setCursor(Qt::ArrowCursor);
}
