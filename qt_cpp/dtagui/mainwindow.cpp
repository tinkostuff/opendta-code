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

   // Daten-Array bekannt machen
   ui->framePlots->setData(&data);
   ui->frameStats->setData(&data);

   // Dops akzeptieren: DTA- und Sitzungsdateien
   setAcceptDrops(true);

   // Standardsitzung laden
   loadDefaultSession = true;

   this->setWindowTitle(tr("DtaGui - Version $Rev$"));
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
   }
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

   if(loadDefaultSession)
   {
      // Default Diagramme bauen
      if( QFile::exists("default.session"))
         ui->framePlots->loadSession("default.session");
      else
         ui->framePlots->loadSession(":/sessions/default.session");
      loadDefaultSession = false;
   }
   else
   {
      ui->framePlots->update();
   }
   ui->frameStats->dataUpdated();
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
            else if( fName.endsWith(".session", Qt::CaseInsensitive))
               sessionFile = fName;
         }
      }

      // Sitzungsdatei laden
      if(sessionFile!="")
      {
         ui->framePlots->loadSession(sessionFile);
         loadDefaultSession = false;
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
      "<p>Version: <b>$Rev$</b> $Date$</p>"
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
      "<p>Copyright (C) 2011 opendta@gmx.de<p>"
   ));
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
* Diagramme hinzufuegen und loeschen
*---------------------------------------------------------------------------*/
void MainWindow::on_actionDiagHinzufuegen_triggered()
{
   ui->framePlots->addPlot();
   loadDefaultSession = false;
}
void MainWindow::on_actionDiagAlleLoeschen_triggered()
{
   ui->framePlots->clear();
   loadDefaultSession = true;
}

/*---------------------------------------------------------------------------
* Sitzungen laden und speichern
*---------------------------------------------------------------------------*/
void MainWindow::on_actionSitzungSpeichern_triggered()
{
   QString fileName = QFileDialog::getSaveFileName( this,
                                                    tr("Sitzung speichern"),
                                                    tr("."),
                                                    tr("Sitzungsdateien (*.session);;Alle Dateien (*.*)"));
   if( fileName != "")
      ui->framePlots->saveSession(fileName);
}

void MainWindow::on_actionSitzungOeffnen_triggered()
{
   QString fileName = QFileDialog::getOpenFileName( this,
                                                    tr("Sitzung \344ffnen"),
                                                    tr("."),
                                                    tr("Sitzungsdateien (*.session);;Alle Dateien (*.*)"));
   if( fileName != "")
   {
      ui->framePlots->loadSession(fileName);
      loadDefaultSession = false;
   }
}

/*---------------------------------------------------------------------------
* Drucken
*---------------------------------------------------------------------------*/
void MainWindow::on_actionDrucken_triggered()
{
   QPrinter *printer = new QPrinter;
   QPrintDialog printDialog(printer, this);
   if (printDialog.exec() == QDialog::Accepted) {
      ui->framePlots->printAll(printer);
   }
}
void MainWindow::on_actionStatistikDrucken_triggered()
{
   QPrinter *printer = new QPrinter;
   QPrintDialog printDialog(printer, this);
   if (printDialog.exec() == QDialog::Accepted) {
      ui->frameStats->print(printer);
   }
}

/*---------------------------------------------------------------------------
* Dialog zum Oeffnen von DTA-Dateien
*---------------------------------------------------------------------------*/
void MainWindow::on_actionOeffnen_triggered()
{
   QStringList files = QFileDialog::getOpenFileNames(
         this,
         tr("Eine oder mehrere Dateien ausw\344hlen"),
         ".",
         tr("DTA-Dateien (*.dta);;Alle Dateien (*.*)"));
   if(!files.isEmpty()) readDtaFiles(files);
}

/*---------------------------------------------------------------------------
* Datenarray leeren
*---------------------------------------------------------------------------*/
void MainWindow::on_actionNeu_triggered()
{
   // reset Daten
   this->data.clear();
   statusBar()->showMessage( tr("Datens\344tze: 0"));
   ui->framePlots->update();
   ui->frameStats->dataUpdated();
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
