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
* Hauptfenster
*---------------------------------------------------------------------------*/
#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QPainter>
#include <QSettings>

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
* DTA-Dateien oeffnen und laden
*---------------------------------------------------------------------------*/
void MainWindow::readDtaFiles(QStringList files)
{
   setCursor(Qt::WaitCursor);
   for( int i=0; i<files.size(); i++)
   {
      QString fileName = files.at(i);
      statusBar()->showMessage(QString(tr("Lese Datei: %1")).arg(fileName));
      QCoreApplication::processEvents();

      // DTA-Datei einlesen
      DtaFile *dta = new DtaFile(fileName);
      if( !dta->open())
      {
         QMessageBox::warning(
               this,
               tr("Fehler beim \326ffnen der DTA-Datei"),
               QString(tr("Fehler beim \326ffnen der DTA-Datei '%1'!")).arg(fileName));
      }
      else
      {
         dta->readDatasets(&data);
      }
      delete dta;
   } // for files

   // letzten Pfad merken
   QFileInfo fi(files.last());
   lastOpenPathDTA = fi.absolutePath();

   setCursor(Qt::ArrowCursor);

   if(data.isEmpty())
      statusBar()->showMessage( tr("Datens\344tze: 0"));
   else
   {
      statusBar()->showMessage(
         QString(tr("Datens\344tze: %1 - Start: %2 - Ende: %3"))
           .arg(data.size())
           .arg(QDateTime::fromTime_t(data.keys().first()).toString("yyyy-MM-dd hh:mm"))
           .arg(QDateTime::fromTime_t(data.keys().last()).toString("yyyy-MM-dd hh:mm")));
   }

   emit dataChanged();
}

/*---------------------------------------------------------------------------
* DUMP-Dateien oeffnen und laden
*---------------------------------------------------------------------------*/
void MainWindow::readDumpFiles(QStringList files)
{
   setCursor(Qt::WaitCursor);
   for( int i=0; i<files.size(); i++)
   {
      QString fileName = files.at(i);
      statusBar()->showMessage(QString(tr("Lese Datei: %1")).arg(fileName));
      QCoreApplication::processEvents();

      // DTA-Datei einlesen
      DumpFile *dump = new DumpFile(fileName);
      if( !dump->open())
      {
         QMessageBox::warning(
               this,
               tr("Fehler beim \326ffnen der DUMP-Datei"),
               QString(tr("Fehler beim \326ffnen der DUMP-Datei '%1'!")).arg(fileName));
      }
      else
      {
         dump->readDatasets(&data);
      }
      delete dump;
   } // for files

   // letzten Pfad merken
   QFileInfo fi(files.last());
   lastOpenPathDTA = fi.absolutePath();

   setCursor(Qt::ArrowCursor);

   if(data.isEmpty())
      statusBar()->showMessage( tr("Datens\344tze: 0"));
   else
   {
      statusBar()->showMessage(
         QString(tr("Datens\344tze: %1 - Start: %2 - Ende: %3"))
           .arg(data.size())
           .arg(QDateTime::fromTime_t(data.keys().first()).toString("yyyy-MM-dd hh:mm"))
           .arg(QDateTime::fromTime_t(data.keys().last()).toString("yyyy-MM-dd hh:mm")));
   }

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
      if(!dtaFiles.isEmpty()) readDtaFiles(dtaFiles);
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
      "<p>Copyright (C) 2011 opendta@gmx.de<p>").arg(VERSION_STRING)
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
      "<p>I would like to thank the Qwt tean for their work!</p>"
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
   if(!files.isEmpty()) readDtaFiles(files);
}
void MainWindow::on_actionDUMPOeffnen_triggered()
{
   QStringList files = QFileDialog::getOpenFileNames(
         this,
         tr("Eine oder mehrere Dateien ausw\344hlen"),
         lastOpenPathDTA,
         tr("DUMP-Dateien (*.dump.bz2);;Alle Dateien (*.*)"));
   if(!files.isEmpty()) readDumpFiles(files);
}


/*---------------------------------------------------------------------------
* Datenarray leeren
*---------------------------------------------------------------------------*/
void MainWindow::on_actionZuruecksetzen_triggered()
{
   // reset Daten
   this->data.clear();
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

