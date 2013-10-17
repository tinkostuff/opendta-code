/*
*---------------------------------------------------------------------------
* Copyright (C) 2013  opendta@gmx.de
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
*  Hauptfenster
*---------------------------------------------------------------------------*/
#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QLocale>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dtagui/dtaplotframe.h"
#include "dtagui/dtastatsframe.h"
#include "dtagui/dtacompstartsframe.h"

/*---------------------------------------------------------------------------
* Constructor
*---------------------------------------------------------------------------*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
   helpDialog = NULL;
   ui->setupUi(this);
   ui->tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);

   // Fenster maximieren
   setWindowState(windowState() ^ Qt::WindowMaximized);

   // Zaehler initialsieren
   tabDiagramCount = 1;
   tabStatisticsCount = 1;
   tabCompStartsCount = 1;

   // initiale Tabs anlegen
   this->on_actionNeuKompStarts_triggered();
   this->on_actionNeuStatistik_triggered();
   this->on_actionNeuDiagramm_triggered();

   // Dops akzeptieren: DTA- und Sitzungsdateien
   setAcceptDrops(true);

   lastOpenPathDTA = ".";

   this->setWindowTitle(tr("DtaGui - %1").arg(VERSION_STRING));
}

/*---------------------------------------------------------------------------
* Destructor
*---------------------------------------------------------------------------*/
MainWindow::~MainWindow()
{
   if(helpDialog) delete helpDialog;
   delete ui;
}

/*---------------------------------------------------------------------------
* Data-Dateien oeffnen und laden
*---------------------------------------------------------------------------*/
void MainWindow::readDataFiles(QStringList files, bool DTA)
{
   setCursor(Qt::WaitCursor);
   for( int i=0; i<files.size(); i++)
   {
      QString fileName = files.at(i);
      statusBar()->showMessage(QString(tr("Lese Datei: %1")).arg(fileName));
      QCoreApplication::processEvents();

      // Datei einlesen
      DataFile *dataFile;
      if (DTA) dataFile = new DtaFile(fileName);
      else dataFile = new DumpFile(fileName);
      if( !dataFile->open())
      {
         QMessageBox::warning(
               this,
               tr("Fehler beim \326ffnen der Daten-Datei"),
               dataFile->errorMsg);
      }
      else
      {
         dataFile->readDatasets(&data);

         // Dateiversionen merken
         if(fileVersions.indexOf(dataFile->version()) == -1)
            fileVersions << dataFile->version();
      }
      delete dataFile;
   } // for files

   // letzten Pfad merken
   QFileInfo fi(files.last());
   lastOpenPathDTA = fi.absolutePath();

   // StatusBar aktualisieren
   if(data.isEmpty())
      statusBar()->showMessage( tr("Datens\344tze: 0"));
   else
   {
      statusBar()->showMessage(
         QString(tr("Datens\344tze: %1, Start: %2, Ende: %3, Datei-Version: %4"))
           .arg(data.size())
           .arg(QDateTime::fromTime_t(data.keys().first()).toString("yyyy-MM-dd hh:mm"))
           .arg(QDateTime::fromTime_t(data.keys().last()).toString("yyyy-MM-dd hh:mm"))
           .arg(fileVersions.join(", ")));
   }

   setCursor(Qt::ArrowCursor);
   emit dataChanged();
}

/*---------------------------------------------------------------------------
* DTA- und Sitzungsdateien per Drag&Drop in die Anwendung bringen
*---------------------------------------------------------------------------*/
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
   // text/uri-list akzeptieren
   if (event->mimeData()->hasFormat("text/uri-list"))
      event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
   QList<QUrl> urlList;
   QString fName;
   QFileInfo info;
   QStringList dtaFiles;
   QString sessionFile = "";

   // Dateinamen extrahieren
   if( event->mimeData()->hasUrls())
   {
      urlList = event->mimeData()->urls();
      for( int i=0; i<urlList.size(); i++)
      {
         fName = urlList.at(i).toLocalFile(); // convert first QUrl to local path
         info.setFile(fName); // information about file
         if(info.isFile())
         {
            if( fName.endsWith(".dta",Qt::CaseInsensitive))
               dtaFiles << fName;
         }
      }

      // DTA-Dateien laden
      if(!dtaFiles.isEmpty()) readDataFiles(dtaFiles, true);
   }

   if( (sessionFile!="") && (!dtaFiles.isEmpty()))
      event->acceptProposedAction();
}

/*---------------------------------------------------------------------------
*---------------------------------------------------------------------------
* Actions
*---------------------------------------------------------------------------
*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
* ueber diese Anwendung
*---------------------------------------------------------------------------*/
void MainWindow::on_actionUeber_triggered()
{
   QMessageBox::about( this,
                       tr("\334ber DtaGui"),
                       tr(
      "<h3>&Uuml;ber DtaGui</h3>"
      "<p>Author: opendta@gmx.de, <a href=\"http://opendta.sf.net\">http://opendta.sf.net</a></p>"
      "<p>Version: <b>%1</p>"
      ""
      "<h4>Beschreibung</h4>"
      "<p>Mit DtaGui k&ouml;nnen DTA-Dateien visualisiert und statistisch "
      "analysiert werden.</p>"
      "<p>DTA-Dateien werden von der W&auml;rmepumpensteuerung <i>Luxtronik 2</i> "
      "der Firma <i>AlphaInnotec</i> (C) erzeugt. Sie enhalten den Betriebszustand "
      "(Temperaturen, Eingangs- und Ausgangssignale) der W&auml;rmepumpe der letzten 48h.</p>"
      "<p>Baugleiche Steuerung sind: Siemens-Novelan WPR-NET, ?</p>"
      ""
      "<h4>Lizenz</h4>"
      "<p>Dieses Programm ist freie Software. Sie k&ouml;nnen es unter den Bedingungen "
      "der GNU General Public License, wie von der Free Software Foundation "
      "ver&ouml;ffentlicht, weitergeben und/oder modifizieren, entweder gem&auml;&szlig; "
      "Version 3 der Lizenz oder (nach Ihrer Option) jeder sp&auml;teren Version.</p>"
      "<p>Die Ver&ouml;ffentlichung dieses Programms erfolgt in der Hoffnung, dass es "
      "Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, sogar ohne die "
      "implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT F&Uuml;R EINEN "
      "BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License.</p>"
      "Sie sollten ein Exemplar der GNU General Public License zusammen mit "
      "diesem Programm erhalten haben. Falls nicht, siehe "
      "<a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>.</p>"
      ""
      "<p>Copyright (C) 2013 opendta@gmx.de<p>").arg(VERSION_STRING)
   );
}
void MainWindow::on_actionUeberQt_triggered()
{
   QMessageBox::aboutQt( this, tr("\334ber Qt"));
}
void MainWindow::on_actionUeberQwt_triggered()
{
   QMessageBox::about( this,
                       tr("\334ber Qwt"),
                       tr(
      "<h3>Qwt</h3>"
      "<h4>Deutsch:</h4>"
      "<p>Dieses Programm basiert in Teilen auf der Arbeit des Qwt Projektes "
      "(<a href=\"http://qwt.sf.net\">http://qwt.sf.net</a>).</p>"
      "<p>Ich m&ouml;chte mich an dieser Stelle recht herzlich f&uuml;r die "
      "geleistet Arbeit bedanken!</p>"
      ""
      "<h4>English:</h4>"
      "<p>Program is based in part on the work of the Qwt project "
      "(<a href=\"http://qwt.sf.net\">http://qwt.sf.net</a>).</p>"
      "<p>I would like to thank the Qwt team for their work!</p>"
      ));
}

/*---------------------------------------------------------------------------
* Dialog zum Oeffnen von DTA-Dateien
*---------------------------------------------------------------------------*/
void MainWindow::on_actionOeffnen_triggered()
{
   QStringList files = QFileDialog::getOpenFileNames(
         this,
         tr("Eine oder mehrere Dateien ausw\344hlen"),
         lastOpenPathDTA,
         tr("DTA-Dateien (*.dta);;Alle Dateien (*.*)"));
   if(!files.isEmpty()) readDataFiles(files, true);
}
void MainWindow::on_actionDUMPOeffnen_triggered()
{
   QStringList files = QFileDialog::getOpenFileNames(
         this,
         tr("Eine oder mehrere Dateien ausw\344hlen"),
         lastOpenPathDTA,
         tr("DUMP-Dateien (*.dump.bz2 *.dumpe.bz2);;Alle Dateien (*.*)"));
   if(!files.isEmpty()) readDataFiles(files, false);
}

/*---------------------------------------------------------------------------
* Datenarray leeren
*---------------------------------------------------------------------------*/
void MainWindow::on_actionZuruecksetzen_triggered()
{
   // reset Daten
   this->data.clear();
   this->fileVersions.clear();
   statusBar()->showMessage( tr("Datens\344tze: 0"));
   emit dataChanged();
}

/*---------------------------------------------------------------------------
* Hilfe
*---------------------------------------------------------------------------*/
void MainWindow::on_actionHilfe_triggered()
{
   if(!helpDialog)
   {
      // Hilfe laden
      QString helpText;
      QFile *f = new QFile(":/help/doc/hilfe.html");
      if(f->open(QFile::ReadOnly))
      {
         QTextStream strm(f);
         helpText = strm.readAll();
         f->close();
         delete f;
      }
      else
      {
         QMessageBox::warning( this,
                               tr("Hilfe"),
                               tr("Hilfe konnte nicht geladen werden!"));
         delete f;
         return;
      }

      // Hilfefenster erstellen
      helpDialog = new QDialog( this, Qt::Dialog);
      helpDialog->setWindowTitle(tr("DtaGui Hilfe"));
      helpDialog->resize(800,600);

      QTextBrowser *text = new QTextBrowser(this);
      text->setHtml(helpText);

      QVBoxLayout *layout = new QVBoxLayout();
      layout->addWidget(text);
      helpDialog->setLayout(layout);
   }

   // Hilfe anzeigen
   helpDialog->show();
   helpDialog->raise();
   helpDialog->activateWindow();
}

/*---------------------------------------------------------------------------
* Tab schliessen
*---------------------------------------------------------------------------*/
void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
   delete ui->tabWidget->widget(index);
   //ui->tabWidget->removeTab(index);
}

/*---------------------------------------------------------------------------
* neues Diagramm einfuegen
*---------------------------------------------------------------------------*/
void MainWindow::on_actionNeuDiagramm_triggered()
{
   DtaPlotFrame *f = new DtaPlotFrame(&data, ui->tabWidget);
   connect( this, SIGNAL(dataChanged()), f, SLOT(dataUpdated()));
   int idx = ui->tabWidget->addTab( f, QIcon(), tr("Diagramm %1").arg(tabDiagramCount++));
   ui->tabWidget->setCurrentIndex(idx);
}

/*---------------------------------------------------------------------------
* neue Statistikseite einfuegen
*---------------------------------------------------------------------------*/
void MainWindow::on_actionNeuStatistik_triggered()
{
   DtaStatsFrame *f = new DtaStatsFrame(&data,ui->tabWidget);
   connect( this, SIGNAL(dataChanged()), f, SLOT(dataUpdated()));
   int idx = ui->tabWidget->addTab( f, QIcon(), tr("Statistik %1").arg(tabStatisticsCount++));
   ui->tabWidget->setCurrentIndex(idx);
}

/*---------------------------------------------------------------------------
* neue Kompressor-Starts-Seite einfuegen
*---------------------------------------------------------------------------*/
void MainWindow::on_actionNeuKompStarts_triggered()
{
   DtaCompStartsFrame *f = new DtaCompStartsFrame(&data,ui->tabWidget);
   connect( this, SIGNAL(dataChanged()), f, SLOT(dataUpdated()));
   int idx = ui->tabWidget->addTab( f, QIcon(), tr("Verdichter Starts %1").arg(tabCompStartsCount++));
   ui->tabWidget->setCurrentIndex(idx);
}

/*---------------------------------------------------------------------------
* Daten im CSV-Format speichern
*---------------------------------------------------------------------------*/
void MainWindow::on_actionCSVSpeichern_triggered()
{
   // Daten vorhanden?
   if(data.isEmpty())
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
                             tr("FEHLER: beim \326ffnen der CSV-Datei '%1'!").arg(fileName));
      return;
   }
   QTextStream out(&fOut);

   setCursor(Qt::WaitCursor);
   QCoreApplication::processEvents();

   // Kopfzeile
   out << tr("Datum/Uhrzeit") << separator << tr("Zeitstempel");
   for( int i=0; i<DataFile::fieldCount(); i++)
      out << separator << DataFile::fieldInfo(i).prettyName;
   out << endl;

   // Daten schreiben
   DataMap::const_iterator iterator = data.constBegin();
   do
   {
      QDateTime timestamp = QDateTime::fromTime_t(iterator.key());
      DataFieldValues values = iterator.value();

      // Datum + Zeitstempel
      out << timestamp.toString("yyyy-MM-dd hh:mm:ss")
          << separator
          << iterator.key();
      // Felder
      for( int i=0; i<values.size(); i++)
         out << separator << QLocale::system().toString(values[i]);
      out << endl;

      // naechster Datensatz
      iterator++;
   } while( iterator != data.constEnd());

   // Ausgabedatei schliessen
   fOut.close();

   setCursor(Qt::ArrowCursor);
}

/*---------------------------------------------------------------------------
* Sprache auswaehlen
*---------------------------------------------------------------------------*/
void MainWindow::on_actionSprache_triggered()
{
   // INI-Datei lesen
   QSettings cfg(
            QCoreApplication::applicationDirPath()+"/dtagui.ini",
            QSettings::IniFormat,
            this);
   QString lang = cfg.value( "dtagui/lang", "de").toString();
   // Sprach-Dateien suchen
   QDir dir(QCoreApplication::applicationDirPath());
   dir.setNameFilters(QStringList() << "dtagui_*.qm");
   dir.setFilter(QDir::Files);
   QStringList files = dir.entryList();

   // Optionen fuer die Auswahlliste erstellen
   QStringList opts;
   opts << "de";
   QStringListIterator iter(files);
   while(iter.hasNext())
   {
      QString s = iter.next();
      s.remove("dtagui_");
      s.remove(".qm");
      opts << s;
   }

   // Benutzer fragen
   bool ok;
   QString sel = QInputDialog::getItem(
            this,
            tr("Sprache ausw\344hlen"),
            tr("Bitte eine Sprache ausw\344hlen.<br>Die Datei 'dtagui_&lt;sprache&gt;.qm' muss im Programmverzeichnis verf\374gbar sein!<br><b>Hinweis:</b> Die \304nderung wird erst nach einem Programmstart wirksam!"),
            opts,
            opts.indexOf(lang)==-1 ? 0 : opts.indexOf(lang),
            true,
            &ok);

   // Aenderung speichern
   if( ok && (sel!="") && (sel!=lang))
   {
      cfg.setValue("dtagui/lang", sel);
      QMessageBox::information(
               this,
               tr("Anwendung neu starten"),
               tr("Bitte die Anwendung neu starten, um die \304nderung wirksam zu machen!")
            );
   }
}
