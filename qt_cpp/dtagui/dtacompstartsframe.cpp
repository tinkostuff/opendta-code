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

#include <QtGui>
#include <QThread>

#include <QtGlobal>
#include <QtDebug>

#include "dtacompstartsframe.h"

/*---------------------------------------------------------------------------
* Definitionen
*---------------------------------------------------------------------------*/
#define UNDEFINED_VALUE -1024

/*---------------------------------------------------------------------------
* DtaStatsRun - Daten eines VD1-Starts speichern
*---------------------------------------------------------------------------*/
enum DtaStatsRunFields { lengthField, pauseField, TVLField, TRLField, SpHZField, TWQeinField, TWQausField, SpWQField, TAField};
class DtaStatsRun : public QObject
{
public:
   DtaStatsRun(const DtaStatsRun& r) : QObject()
   {
      this->start = r.start;
      this->length = r.length;
      this->mode = r.mode;
      this->pause = r.pause;
      this->TVL = r.TVL;
      this->TRL = r.TRL;
      this->SpHz = r.SpHz;
      this->TWQein = r.TWQein;
      this->TWQaus = r.TWQaus;
      this->SpWQ = r.SpWQ;
      this->TA = r.TA;
   }
   DtaStatsRun() : QObject()
   {
      this->start = 0;
      this->length = -1;
      this->mode = -1;
      this->pause = -1;
      this->TVL = -100.0;
      this->TRL = -100.0;
      this->SpHz = -100.0;
      this->TWQein = -100.0;
      this->TWQaus = -100.0;
      this->SpWQ = -100.0;
      this->TA = -100.0;
   }
   DtaStatsRun operator=(const DtaStatsRun& r)
   {
      DtaStatsRun res;
      res.start = r.start;
      res.length = r.length;
      res.mode = r.mode;
      res.pause = r.pause;
      res.TVL = r.TVL;
      res.TRL = r.TRL;
      res.SpHz = r.SpHz;
      res.TWQein = r.TWQein;
      res.TWQaus = r.TWQaus;
      res.SpWQ = r.SpWQ;
      res.TA = r.TA;
      return res;
   }
   static QList<DtaStatsRunFields> statFields()
   {
      QList<DtaStatsRunFields> res;
      res << lengthField << pauseField << TVLField << TRLField << SpHZField
            << TWQeinField << TWQausField << SpWQField << TAField;
      return res;
   }
   qreal value(DtaStatsRunFields field)
   {
      switch(field)
      {
      case lengthField: {return this->length; break;}
      case pauseField: {return this->pause; break;}
      case TVLField: {return this->TVL; break;}
      case TRLField: {return this->TRL; break;}
      case SpHZField: {return this->SpHz; break;}
      case TWQeinField: {return this->TWQein; break;}
      case TWQausField: {return this->TWQaus; break;}
      case SpWQField: {return this->SpWQ; break;}
      case TAField: {return this->TA; break;}
      }
      return -1.0;
   }
   quint32 start;
   qint32 length;
   qint32 mode;
   qint32 pause;
   qreal TVL;
   qreal TRL;
   qreal SpHz;
   qreal TWQein;
   qreal TWQaus;
   qreal SpWQ;
   qreal TA;
};

/*---------------------------------------------------------------------------
* DtaCompStartsThread - Thread zum Berechnen der Kompressor Starts
*---------------------------------------------------------------------------*/
class DtaCompStartsThread : public QThread
{
public:
   DtaCompStartsThread() {this->data=NULL;}
   // Zeiger auf Daten uebergeben
   void setData(DtaDataMap *data) {this->data=data;}
   // Zeitspanne setzen
   void setDateTimeRange( quint32 start, quint32 end) { this->tsStart=start; this->tsEnd=end;}
   // Daten analysieren
   void run()
   {
      if(data==NULL || data->size()==0) return;

      // Anfang und Ende
      dataStart = 0;
      dataEnd = 0;
      datasets = 0;

      quint32 lastTS = 0;
      bool first = true;
      runList.clear();

      // moegliche Modi setzen
      modeList.clear();
      modeList << tr("HZ");
      modeList << tr("BW");
      modeList << tr("HZ+BW");
      modeList << tr("von EVU unterbrochen");

      DtaDataMap::iterator iteratorEnd = data->upperBound(tsEnd);
      DtaDataMap::iterator iterator = data->lowerBound(tsStart);
      do
      {
         quint32 ts = iterator.key();
         DtaFieldValues dat = iterator.value();

         // Zeitspanne testen
         if( (tsStart!=0) && (ts < tsStart)) continue;
         if( (tsEnd!=0) && (ts > tsEnd)) break;

         // Ende der Statistik merken
         dataEnd = ts;
         datasets++;

         qint32 vd1 = DtaFile::fieldValueInt( dat, "VD1");
         qint32 evu = DtaFile::fieldValueInt( dat, "EVU_");

         // Initialisierung
         if(first)
         {
            firstRun.insert( 0, true);
            firstRun.insert( 1, true);
            dataStart = ts;

            stateVD1 = stateOff;
            if( vd1 == 1) stateVD1 = stateInvalid;

            // EVU
            lastEVU = 0;
            stateEVU = stateOff;
            if( evu == 1) stateEVU = stateOn;

            // Luecken merken
            missingList.clear();
            missingSum = 0;
         }
         else // !first
         {
            // Luecken suchen
            bool missingFound = false;
            if((ts - lastTS) > 62) // 62 Sekunden fuer ein bisschen Spielraum
            {
               missingList << ts;
               missingSum += ts - lastTS - 60;
               missingFound = true;

               // runFirst zuruecksetzen
               firstRun[0] = true;
               firstRun[1] = true;

               // Lauf ungueltig machen
               if( vd1 == 1) stateVD1 = stateInvalid;
            }

            //
            // EVU
            //
            if( (evu==1) && (stateEVU==stateOff))
               stateEVU = stateOn;
            else if( (evu==0) && (stateEVU==stateOn))
            {
               lastEVU = ts;
               stateEVU = stateOff;
            }

            if(vd1 == 1)
            {
               // Start
               if( stateVD1 == stateOff)
               {
                  cmpRun.start = ts;
                  cmpRun.mode = DtaFile::fieldValueInt( dat, "BUP");
                  sumTA = DtaFile::fieldValueReal( dat, "TA");
                  countTA = 1;
                  stateVD1 = stateOn;
               }
               // im Kompressor Lauf
               else if( stateVD1 == stateOn)
               {
                  sumTA += DtaFile::fieldValueReal( dat, "TA");
                  countTA += 1;
                  cmpRun.TRL = DtaFile::fieldValueReal( dat, "TRL");
                  cmpRun.TVL = DtaFile::fieldValueReal( dat, "TVL");
                  cmpRun.SpHz = DtaFile::fieldValueReal( dat, "SpHz");
                  cmpRun.TWQein = DtaFile::fieldValueReal( dat, "TWQein");
                  cmpRun.TWQaus = DtaFile::fieldValueReal( dat, "TWQaus");
                  cmpRun.SpWQ = DtaFile::fieldValueReal( dat, "SpWQ");
                  // hat sich der Modus geaendert?
                  if( cmpRun.mode != DtaFile::fieldValueInt( dat, "BUP")) cmpRun.mode = 2;
               }
               else
               {
                  // Modus speichern, auch wenn wir in einem ungueltigen Lauf sind
                  cmpRun.mode = DtaFile::fieldValueInt( dat, "BUP");
               }
            } // VD1 == 1

            else if(vd1 == 0)
            {
               // ungueltiger Lauf
               if( stateVD1 == stateInvalid)
               {
                  // aber Ende speichern
                  lastRun[cmpRun.mode] = ts;
                  stateVD1 = stateOff;
               }

               // Ende eines Laufes
               else if( stateVD1 == stateOn)
               {
                  // Mittelwert der Aussentemperatur
                  cmpRun.TA = sumTA / countTA;

                  // Laenge
                  cmpRun.length = ts - cmpRun.start;

                  // Ruhezeit vor dem Lauf
                  if( (cmpRun.mode<=1)  && !firstRun[cmpRun.mode])
                     // Start direkt nach EVU Freigabe? Schwelle = 8min (Netzeinschaltverzoegerung)
                     if( cmpRun.start-lastEVU > 60*8)
                        cmpRun.pause = cmpRun.start - lastRun[cmpRun.mode];
                     else
                        cmpRun.pause = UNDEFINED_VALUE;
                  else
                     cmpRun.pause = UNDEFINED_VALUE;

                  // firstRun setzen
                  if( cmpRun.mode <= 1)
                     firstRun[cmpRun.mode] = false;
                  else
                  {
                     firstRun[0] = true;
                     firstRun[1] = true;
                  }

                  // Ende des Laufes merken
                  if( cmpRun.mode <= 1)
                     lastRun[cmpRun.mode] = ts;
                  else
                  {
                     // HZ + BW
                     lastRun[0] = ts;
                     lastRun[1] = ts;
                  }

                  // Lauf von EVU unterbrochen?
                  if( evu == 1)
                  {
                     if( cmpRun.mode <= 1)
                        firstRun[cmpRun.mode] = true;
                     else
                     {
                        firstRun[0] = true;
                        firstRun[1] = true;
                     }
                     cmpRun.mode = 3;
                  }

                  runList << cmpRun;
                  stateVD1 = stateOff;
               }

            } // VD1 == 0

         } // if/else first

         // letzten Zeitstempel merken
         lastTS = ts;
         first = false;

         // naechster Datensatz
         iterator++;
      } while( iterator != iteratorEnd);

      //
      // Statistik der Laeufe
      //
      QList<DtaStatsRunFields> fields = DtaStatsRun::statFields();
      firstRun[0] = true;
      firstRun[1] = true;
      starts[0] = 0;
      starts[1] = 0;
      for( int i=0; i<runList.size(); i++)
      {
         DtaStatsRun r = runList.at(i);
         if( r.mode > 1) continue;
         starts[r.mode] += 1;

         for( int j=0; j<fields.size(); j++)
         {
            DtaStatsRunFields field = fields.at(j);
            qreal v = r.value(field);
            if(firstRun[r.mode])
            {
               runData[field][r.mode] = QList<qreal>();
               min[field][r.mode] = v;
               max[field][r.mode] = v;
               if(v != UNDEFINED_VALUE)
               {
                  sum[field][r.mode] = v;
                  count[field][r.mode] = 1;
                  runData[field][r.mode] << v;
               }
               else
               {
                  sum[field][r.mode] = 0;
                  count[field][r.mode] = 0;
               }
            } // if first
            else
            {
               if( v != UNDEFINED_VALUE) // gueltiger Wert?
               {
                  if( (min[field][r.mode] == UNDEFINED_VALUE) || (v < min[field][r.mode]))
                     min[field][r.mode] = v;
                  if( (max[field][r.mode] == UNDEFINED_VALUE) || (v > max[field][r.mode]))
                     max[field][r.mode] = v;
                  sum[field][r.mode] += v;
                  count[field][r.mode] += 1;
                  runData[field][r.mode] << v;
               }
            } // else first
         } // for fields

         firstRun[r.mode] = false;
      } // for runList

      //
      // Statistik Nachbearbeitung
      //
      for( int j=0; j<fields.size(); j++)
      {
         DtaStatsRunFields field = fields.at(j);
         for( int bw=0; bw<=1; bw++)
         {
            // fehlende Daten auffuellen
            if(!min[field].contains(bw)) min[field][bw] = 0.0;
            if(!max[field].contains(bw)) max[field][bw] = 0.0;
            if(!sum[field].contains(bw)) sum[field][bw] = 0.0;
            if(!count[field].contains(bw)) count[field][bw] = 1;
            if(!runData[field].contains(bw)) runData[field][bw] = QList<qreal>();

            // Durchschnitt
            avg[field][bw] = sum[field][bw] / count[field][bw];

            // Median
            qSort(runData[field][bw]);
            quint32 n = runData[field][bw].size();
            if( n == 0)
               median[field][bw] = 0.0;
            else if( n%2 == 1)
               median[field][bw] = runData[field][bw].at(n/2);
            else
               median[field][bw] = (runData[field][bw].at(n/2-1) + runData[field][bw].at(n/2))/2.0;

            // Standardabweichung
            stddev[field][bw] = 0.0;
            if(n>1)
            {
               for( int k=0; k<runData[field][bw].size(); k++)
               {
                  qreal v = runData[field][bw].at(k);
                  stddev[field][bw] += qPow(v - avg[field][bw],2);
               }
               stddev[field][bw] = qSqrt( stddev[field][bw] / qreal(n-1));
            }
         }
      } // for fields

   } // run()

   quint32 dataStart;
   quint32 dataEnd;
   quint32 datasets;

   QStringList modeList;
   QList<quint32> missingList;
   quint32 missingSum;
   QList<DtaStatsRun> runList;

   // Statistik
   QHash<DtaStatsRunFields, QHash<qint32,qreal> > min;
   QHash<DtaStatsRunFields, QHash<qint32,qreal> > max;
   QHash<DtaStatsRunFields, QHash<qint32,qreal> > avg;
   QHash<DtaStatsRunFields, QHash<qint32,qreal> > median;
   QHash<DtaStatsRunFields, QHash<qint32,qreal> > stddev;
   QHash<qint32, qint32> starts;

private:
   DtaDataMap *data;
   quint32 tsStart;
   quint32 tsEnd;

   enum states {stateOn, stateOff, stateInvalid};
   states stateVD1;
   states stateEVU;

   QHash<qint32, bool> firstRun;
   QHash<qint32, quint32> lastRun;
   quint32 lastEVU;
   DtaStatsRun cmpRun;
   qreal sumTA;
   qint32 countTA;

   // Statistik
   QHash<DtaStatsRunFields, QHash<qint32,qreal> > sum;
   QHash<DtaStatsRunFields, QHash<qint32,qint32> > count;
   QHash<DtaStatsRunFields, QHash<qint32, QList<qreal> > > runData;
};

/*---------------------------------------------------------------------------
* DtaCompStatsFrame - Daten darstellen
*---------------------------------------------------------------------------*/
DtaCompStartsFrame::DtaCompStartsFrame(DtaDataMap *data, QWidget *parent) :
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

   QLabel *label = new QLabel(tr("Modus:"), this);
   cbModus = new QComboBox(this);
   cbModus->setMinimumWidth(200);
   QHBoxLayout *layout1 = new QHBoxLayout();
   layout1->addWidget(label);
   layout1->addWidget(cbModus);
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
   textEdit->insertPlainText(tr("Bitte warten! Daten werden ausgewertet."));

   // Zeitspanne der Eingabefelder aktualisieren
   this->updateTimeRangeEdit();

   // Thread starten
   this->thread = new DtaCompStartsThread();
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
void DtaCompStartsFrame::threadTerminated()
{
   textEdit->clear();
   textEdit->insertPlainText(tr("Fehler beim Auswerten der Daten!"));
   delete this->thread;
   this->thread = NULL;
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
      delete this->thread;
      this->thread = NULL;
      return;
   }

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
                        .arg(tr("Datens\344tze"))
                        .arg(thread->datasets);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Daten Start"))
                        .arg(QDateTime::fromTime_t(thread->dataStart).toString("yyyy-MM-dd hh:mm:ss"));
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Daten Ende"))
                        .arg(QDateTime::fromTime_t(thread->dataEnd).toString("yyyy-MM-dd hh:mm:ss"));
   qint32 delta = thread->dataEnd - thread->dataStart;
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
                        .arg(thread->missingList.size());
   s = QString(tr("%1 Tage %2 Stunden %3 Minuten"))
                        .arg(thread->missingSum/86400)
                        .arg((thread->missingSum%86400)/3600)
                        .arg((thread->missingSum%3600)/60);
   html << QString("<tr bgcolor=\"#E5E5E5\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Summe Aufzeichnungsl\374cken"))
                        .arg(s);
   html << QString("<tr bgcolor=\"#FFFFFF\"><td>%1</td><td>%2</td></tr>")
                        .arg(tr("Komporessor Starts"))
                        .arg(thread->runList.size());
   html << "</table>";

   // Statistik
   for( int bw=0; bw<=1; bw++)
   {
      if(bw==0)
         html << QString(tr("<h1>Statistik Heizung (%1 Starts)</h1>")).arg(thread->starts[bw]);
      else
         html << QString(tr("<h1>Statistik Warmwasser (%1 Starts)</h1>")).arg(thread->starts[bw]);

      html << "<table cellpadding=\"2\" cellspacing=\"0\" border=\"1\">";
      html << QString("<tr><th align=left>%1</th><th align=center>%2</th><th align=center>%3</th><th align=center>%4</th><th align=center>%5</th><th align=center>%6</th></tr>")
                         .arg(tr("Signalname"))
                         .arg(tr("Minimum"))
                         .arg(tr("Maximum"))
                         .arg(tr("Mittelwert"))
                         .arg(tr("Median"))
                         .arg(tr("Standardabweichung"));

      QList<DtaStatsRunFields> fields = DtaStatsRun::statFields();
      for( int j=0; j<fields.size(); j++)
      {
         DtaStatsRunFields field = fields.at(j);

         // Feldname
         QString fieldName = "";
         switch(field)
         {
         case lengthField: {fieldName=tr("Laufzeit [min]"); break;}
         case pauseField: {fieldName=tr("Pause [min]"); break;}
         case TVLField: {fieldName=tr("TVL [\260C]"); break;}
         case TRLField: {fieldName=tr("TRL [\260C]"); break;}
         case SpHZField: {fieldName=tr("Spreizung HZ [K]"); break;}
         case TWQeinField: {fieldName=tr("TWQein [\260C"); break;}
         case TWQausField: {fieldName=tr("TWQaus [\260C]"); break;}
         case SpWQField: {fieldName=tr("Spreizung WQ [\260C]"); break;}
         case TAField: {fieldName=tr("TA [\260C]"); break;}
         } // switch

         QString lineColor = j%2 ? "#FFFFFF" : "#E5E5E5";
         qreal scale = field==lengthField || field==pauseField ? 60.0 : 1;
         qint32 precision = field==lengthField || field==pauseField ? 0 : 1;
         html << QString("<tr bgcolor=\"%7\"><td align=left>%1</td><td align=center>%2</td><td align=center>%3</td><td align=center>%4</td><td align=center>%5</td><td align=center>%6</td></tr>")
               .arg(fieldName)
               .arg(thread->min[field][bw]/scale,0,'f',precision,' ')
               .arg(thread->max[field][bw]/scale,0,'f',precision,' ')
               .arg(thread->avg[field][bw]/scale,0,'f',precision,' ')
               .arg(thread->median[field][bw]/scale,0,'f',precision,' ')
               .arg(thread->stddev[field][bw]/scale,0,'f',1,' ')
               .arg(lineColor);
      }
      html << "</table>";
   }

   // Fuss
   html << "    <p>DtaGui - http://opendta.sourceforge.net/ - opendta@gmx.de</p>";
   html << "  </body>";
   html << "</html>";
   textEdit->setHtml(html.join("\n"));

   //
   // Tabelle
   //
   cbModus->clear();
   cbModus->addItem( "alle", -1);
   for( int i=0; i<thread->modeList.size(); i++)
      cbModus->addItem( thread->modeList.at(i), i);

   QStringList header;
   header << tr("Start");
   header << tr("Modus");
   header << tr("L\344nge\n[min]");
   header << tr("Pause\n[min]");
   header << tr("TVL\n[\260C]");
   header << tr("TRL\n[\260C]");
   header << tr("SpHZ\n[K]");
   header << tr("TWQein\n[\260C]");
   header << tr("TWQaus\n[\260C]");
   header << tr("SpWQ\n[K]");
   header << tr("TA\n[\260C]");

   table->setColumnCount(header.size());
   table->setRowCount(thread->runList.size());
   table->setHorizontalHeaderLabels(header);

   QTableWidgetItem *item;
   for( int i=0; i<thread->runList.size(); i++)
   {
      const DtaStatsRun *run = &(thread->runList.at(i));

      // Start
      item = new QTableWidgetItem(QDateTime::fromTime_t(run->start).toString("yyyy-MM-dd hh:mm"));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 0, item);

      // Modus
      item = new QTableWidgetItem( thread->modeList[run->mode]);
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 1, item);

      // Laenge
      item = new QTableWidgetItem(QString("%1").arg(run->length/60));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 2, item);

      // Pause
      if(run->pause > 0) s = QString("%1").arg(run->pause/60);
      else s = "";
      item = new QTableWidgetItem(s);
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 3, item);

      // TVL
      item = new QTableWidgetItem(QString("%1").arg(run->TVL,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 4, item);

      // TRL
      item = new QTableWidgetItem(QString("%1").arg(run->TRL,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 5, item);

      // SpHZ
      item = new QTableWidgetItem(QString("%1").arg(run->SpHz,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 6, item);

      // TWQein
      item = new QTableWidgetItem(QString("%1").arg(run->TWQein,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 7, item);

      // TWQaus
      item = new QTableWidgetItem(QString("%1").arg(run->TWQaus,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 8, item);

      // SpWQ
      item = new QTableWidgetItem(QString("%1").arg(run->SpWQ,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 9, item);

      // TA
      item = new QTableWidgetItem(QString("%1").arg(run->TA,0,'f',1));
      item->setTextAlignment(Qt::AlignCenter);
      table->setItem( i, 10, item);
   }

   table->resizeColumnsToContents();
   table->resizeRowsToContents();

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
void DtaCompStartsFrame::setCompleteTimeRange()
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
