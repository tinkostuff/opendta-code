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
#include "statistics/dtafieldstatistics.h"

/*---------------------------------------------------------------------------
* DtaStatsThread
*  - Berechnung der Statistik
*---------------------------------------------------------------------------*/
class DtaStatsThread : public QThread
{
public:
   // Constructor
   DtaStatsThread(QObject *parent=0) : QThread(parent) { data=NULL; tsStart = 0; tsEnd = 0; stats=new DtaFieldStatistics();}
   DtaStatsThread(DtaDataMap *data, quint32 tsStart, quint32 tsEnd, QObject *parent=0) : QThread(parent)
   {
      this->data=data;
      this->tsStart = tsStart;
      this->tsEnd = tsEnd;
      stats = new DtaFieldStatistics();
   }
   ~DtaStatsThread() { if(stats!=NULL) delete stats;}
   // Zeiger auf Daten uebergeben
   void setData(DtaDataMap *data) {this->data=data;}
   // Zeitspanne setzen
   void setDateTimeRange( quint32 start, quint32 end) { this->tsStart=start; this->tsEnd=end;}
   // Berechnungen
   void run()
   {
      if(data==NULL || data->size()==0) return;

      // Iteratoren erstellen
      DtaDataMap::const_iterator iteratorEnd = data->upperBound(tsEnd);
      DtaDataMap::const_iterator iteratorStart = data->lowerBound(tsStart);
      stats->calcStatistics( iteratorStart, iteratorEnd);
   }

   DtaFieldStatistics *stats;
private:
   DtaDataMap *data;
   quint32 tsStart;
   quint32 tsEnd;
};


/*---------------------------------------------------------------------------
* DtaStatsFrame
*  - Darstellung der Ergebnisse
*---------------------------------------------------------------------------*/
DtaStatsFrame::DtaStatsFrame(DtaDataMap *data, QWidget *parent) :
    QFrame(parent)
{
   this->data = data;
   this->thread = NULL;

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

   // textEdit
   textEdit = new QTextEdit(this);
   textEdit->setReadOnly(true);
   layoutMain->addWidget(textEdit);
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

/*---------------------------------------------------------------------------
* Daten wurden aktualisiert : Thread erstellen und starten
*---------------------------------------------------------------------------*/
void DtaStatsFrame::dataUpdated()
{
   textEdit->clear();
   textEdit->insertPlainText(tr("Bitte warten! Daten werden ausgewertet."));

   // Zeitspanne der Eingabefelder aktualisieren
   this->updateTimeRangeEdit();

   // Thread starten
   this->thread = new DtaStatsThread();
   this->thread->setData(data);
   this->thread->setDateTimeRange( dteStart->dateTime().toTime_t(),
                                   dteEnd->dateTime().toTime_t());
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

   DtaFieldStatistics *stats = thread->stats;

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
                        .arg(tr("Anzahl Aufzeichnungsl\374cken"))
                        .arg(stats->missingCount());
   s = QString(tr("%1 Tage %2 Stunden %3 Minuten"))
                        .arg(stats->missingSum()/86400)
                        .arg((stats->missingSum()%86400)/3600)
                        .arg((stats->missingSum()%3600)/60);
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
   for( int i=0; i<stats->analogFields().size(); i++)
   {
      QString field = stats->analogFields().at(i);
      const DtaFieldInfo *info = DtaFile::fieldInfo(field);

      QString lineColor = i%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%7\"><td align=left>%1</td><td align=center>%2</td><td align=center>%3</td><td align=center>%4</td><td align=center>%5</td><td align=center>%6</td></tr>")
            .arg(info->prettyName)
            .arg(stats->analogMin(field),0,'f',1,' ')
            .arg(stats->analogMax(field),0,'f',1,' ')
            .arg(stats->analogAvg(field),0,'f',1,' ')
            .arg(stats->analogMedian(field),0,'f',1,' ')
            .arg(stats->analogStdev(field),0,'f',1,' ')
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
//         .arg(tr("Impulse<br>[]"))
//         .arg(tr("Aktiv<br>[%]"))
//         .arg(tr("Mittelwert Ein<br>[h:min]"))
//         .arg(tr("Minimum Ein<br>[h:min]"))
//         .arg(tr("Maximum Ein<br>[h:min]"))
//         .arg(tr("Inaktiv<br>[%]"))
//         .arg(tr("Mittelwert Aus<br>[h:min]"))
//         .arg(tr("Minimum Aus<br>[h:min]"))
//         .arg(tr("Maximum Aus<br>[h:min]"));
   for( int i=0; i<stats->digitalFields().size(); i++)
   {
      QString field = stats->digitalFields().at(i);
      const DtaFieldInfo *info = DtaFile::fieldInfo(field);
      QString lineColor = i%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%11\"><td align=left>%1</td><td align=center>%2</td><td align=center>%3</td><td align=center>%4</td><td align=center>%5</td><td align=center>%6</td><td align=center>%7</td><td align=center>%8</td><td align=center>%9</td><td align=center>%10</td></tr>")
              .arg(info->prettyName)
              .arg(stats->digitalActOn(field),6)
              .arg(QString(tr("%1&nbsp;%")  ).arg(qreal(stats->digitalTimeOn(field))/(stats->digitalTimeOn(field)+stats->digitalTimeOff(field))*100.0,5,'f',1,' '))
              .arg(QString(tr("%1:%2")).arg(stats->digitalAvgOn(field)/3600).arg(QString("%1").arg((stats->digitalAvgOn(field)%3600)/60.0,0,'f',0).rightJustified(2,'0')))
              .arg(QString(tr("%1:%2")).arg(stats->digitalMinOn(field)/3600).arg(QString("%1").arg((stats->digitalMinOn(field)%3600)/60.0,0,'f',0).rightJustified(2,'0')))
              .arg(QString(tr("%1:%2")).arg(stats->digitalMaxOn(field)/3600).arg(QString("%1").arg((stats->digitalMaxOn(field)%3600)/60.0,0,'f',0).rightJustified(2,'0')))
              .arg(QString(tr("%1&nbsp;%")  ).arg(qreal(stats->digitalTimeOff(field))/(stats->digitalTimeOn(field)+stats->digitalTimeOff(field))*100.0,5,'f',1,' '))
              .arg(QString(tr("%1:%2")).arg(stats->digitalAvgOff(field)/3600).arg(QString("%1").arg((stats->digitalAvgOff(field)%3600)/60.0,0,'f',0).rightJustified(2,'0')))
              .arg(QString(tr("%1:%2")).arg(stats->digitalMinOff(field)/3600).arg(QString("%1").arg((stats->digitalMinOff(field)%3600)/60.0,0,'f',0).rightJustified(2,'0')))
              .arg(QString(tr("%1:%2")).arg(stats->digitalMaxOff(field)/3600).arg(QString("%1").arg((stats->digitalMaxOff(field)%3600)/60.0,0,'f',0).rightJustified(2,'0')))
              .arg(lineColor);
   }
   html << "</table>";

   // statische Felder
   html << "<h1>statische Signale</h1>";
   html << "<table cellpadding=\"2\" cellspacing=\"0\" border=\"1\">";
   html << QString("<tr><th align=left>%1</th><th align=center>%2</th></tr>")
                      .arg(tr("Signalname"))
                      .arg(tr("Wert"));
   for( int i=0; i<stats->analogStaticFields().size(); i++)
   {
      QString field = stats->analogStaticFields().at(i);
      const DtaFieldInfo *info = DtaFile::fieldInfo(field);

      QString lineColor = i%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%3\"><td align=left>%1</td><td align=center>%2</td></tr>")
            .arg(info->prettyName)
            .arg(stats->analogMin(field))
            .arg(lineColor);
   }
   for( int i=0; i<stats->digitalStaticFields().size(); i++)
   {
      QString field = stats->digitalStaticFields().at(i);
      const DtaFieldInfo *info = DtaFile::fieldInfo(field);
      QString lineColor = (i+stats->analogStaticFields().size())%2 ? "#FFFFFF" : "#E5E5E5";
      html << QString("<tr bgcolor=\"%3\"><td align=left>%1</td><td align=center>%2</td></tr>")
            .arg(info->prettyName)
            .arg(stats->digitalLastValue(field))
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
void DtaStatsFrame::print()
{
   QPrinter *printer = new QPrinter;
   QPrintDialog printDialog(printer, this);
   if (printDialog.exec() == QDialog::Accepted)
      textEdit->print(printer);
}

/*---------------------------------------------------------------------------
* Eingabefelder fuer Zeitspanne aktualisieren
*---------------------------------------------------------------------------*/
void DtaStatsFrame::updateTimeRangeEdit()
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
      DtaDataMap::const_iterator iStart = data->constBegin();
      DtaDataMap::const_iterator iEnd = data->constEnd();
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
void DtaStatsFrame::setCompleteTimeRange()
{
   // Start und Ende der Daten ermitteln
   QDateTime dtStart = QDateTime::fromTime_t(0);
   QDateTime dtEnd = QDateTime::fromTime_t(0);
   if( (data!=NULL) && !data->isEmpty())
   {
      DtaDataMap::const_iterator iStart = data->constBegin();
      DtaDataMap::const_iterator iEnd = data->constEnd();
      iEnd--;
      dtStart.setTime_t(iStart.key());
      dtEnd.setTime_t(iEnd.key());
   }
   dteStart->setDateTime(dtStart);
   dteEnd->setDateTime(dtEnd);
}
