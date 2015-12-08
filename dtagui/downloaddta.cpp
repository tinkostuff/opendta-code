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
#include <QMessageBox>
#include <QFileDialog>

#include <QDebug>

#include "downloaddta.h"
#include "ui_downloaddta.h"

#include "config.h"

/*---------------------------------------------------------------------------
* Constructor
*---------------------------------------------------------------------------*/
DownloadDTA::DownloadDTA(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DownloadDTA)
{
   ui->setupUi(this);

   m_fileName = "";
   m_reply = NULL;

   // Konfiguration laden
   QSettings cfg(
            QSettings::IniFormat,
            QSettings::UserScope,
            ORG_NAME,
            APP_NAME,
            this);

   QString url = cfg.value( "download_dta/url", "http://192.168.2.20/proclog").toString();
   QString downloadDir = cfg.value( "download_dta/dir", QDir::temp().absolutePath()).toString();
   bool renameFile = cfg.value("download_dta/rename", true).toBool();

   // Dialog initialisieren
   ui->editURL->setText(url);
   ui->editDir->setText(downloadDir);
   ui->lblStatus->setText(tr("Eingabe erforderlich"));
   ui->progressBar->setValue(0);
   ui->cbRename->setChecked(renameFile);
}

/*---------------------------------------------------------------------------
* Destructor
*---------------------------------------------------------------------------*/
DownloadDTA::~DownloadDTA()
{
    delete ui;
}

/*---------------------------------------------------------------------------
* Abbruch
*---------------------------------------------------------------------------*/
void DownloadDTA::reject()
{
   if( m_reply != NULL)
   {
      // Download abbrechen
      m_reply->abort();
      m_reply->deleteLater();
      m_reply = NULL;

      // Datei schliessen
      m_file.close();
   }
   m_fileName = "";

   QDialog::reject();
}

/*---------------------------------------------------------------------------
* Operation ausfuehren
*---------------------------------------------------------------------------*/
void DownloadDTA::on_buttonBox_accepted()
{
   QUrl url(ui->editURL->text());
   QString downloadDir = ui->editDir->text();
   bool renameFile = ui->cbRename->isChecked();

   // Verzeichnis ueberpruefen
   if(!QDir(downloadDir).exists())
   {
      QMessageBox::critical(
               this,
               tr("Verzeichnis exisitiert nicht!"),
               tr("Das Verzeichnis '%1' existiert nicht!").arg(downloadDir));
      return;
   }

   // Ausgabedatei oeffnen
   QString fileName = "";
   if(renameFile)
      fileName = QString("%1_proclog.dta").arg(QDateTime::currentDateTime().toString("yyMMdd_hhmm"));
   else
      fileName = QFileInfo(url.path()).fileName();
   if( fileName == "")
      fileName = QString("%1_tmp.dta").arg(QDateTime::currentDateTime().toString("yyMMdd_hhmm"));
   m_fileName = downloadDir + "/" + fileName;

   // Existiert die Datei schon?
   if( QFileInfo(m_fileName).exists())
   {
      int res = QMessageBox::question(
               this,
               tr("Datei Überschreiben?"),
               tr("Datei '%1' existiert bereits!<br>Soll sie Überschrieben werden?").arg(m_fileName),
               QMessageBox::Yes,
               QMessageBox::No);
      if(res != QMessageBox::Yes) return;
   }

   // Datei oeffnen
   m_file.setFileName(m_fileName);
   if( !m_file.open(QIODevice::WriteOnly))
   {
      QMessageBox::critical(
               this,
               tr("Fehler"),
               tr("Fehler beim öffnen der Datei '%1'!").arg(m_fileName));
      return;
   }

   // Download starten
   connect( &m_nwam, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
   QNetworkRequest request(url);
   m_reply = m_nwam.get(request);

   // Daten speichern
   connect( m_reply, SIGNAL(readyRead()), this, SLOT(readDownload()));

   // Progressbar aktualisieren
   connect( m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateProgress(qint64,qint64)));

   ui->lblStatus->setText(tr("Download gestartet"));

   // OK deaktivieren
   ui->buttonBox->buttons()[0]->setEnabled(false);

   // Sanduhr anzeigen
   QApplication::setOverrideCursor(Qt::WaitCursor);

   // Progressbar initialisieren
   ui->progressBar->setValue(0);
   ui->progressBar->setMaximum(0);
}

/*---------------------------------------------------------------------------
* Daten in Datei schreiben
*---------------------------------------------------------------------------*/
void DownloadDTA::readDownload()
{
   // Daten in Datei schreiben
   m_file.write(m_reply->readAll());
}

/*---------------------------------------------------------------------------
* Download fertig
*---------------------------------------------------------------------------*/
void DownloadDTA::fileDownloaded(QNetworkReply* reply)
{
   // OK aktivieren
   ui->buttonBox->buttons()[0]->setEnabled(true);

   // normaler Cursor
   QApplication::restoreOverrideCursor();

   // Download Fehler?
   if(reply->error() != QNetworkReply::NoError)
   {
      ui->lblStatus->setText(tr("Download Fehler"));

      // Progressbar zuruecksetzen
      ui->progressBar->setValue(0);
      ui->progressBar->setMaximum(1);

      // Aufraeumen
      m_file.close();
      m_reply->deleteLater();
      m_reply = NULL;

      QMessageBox::critical(
               this,
               tr("Download Fehler"),
               reply->errorString());
      return;
   }

   // restlichen Daten speichern und Datei schliessen
   ui->lblStatus->setText(tr("Datei speichern"));
   readDownload();
   m_file.close();
   m_reply->deleteLater();
   m_reply = NULL;

   // Einstellungen speichern
   ui->lblStatus->setText(tr("Einstellungen speichern"));
   QSettings cfg(
            QSettings::IniFormat,
            QSettings::UserScope,
            ORG_NAME,
            APP_NAME,
            this);
   cfg.setValue( "download_dta/url", ui->editURL->text());
   cfg.setValue( "download_dta/dir", ui->editDir->text());
   cfg.setValue( "download_dta/rename", ui->cbRename->isChecked());

   ui->lblStatus->setText(tr("Ende"));

   // Dialog beenden
   QDialog::accept();
}

/*---------------------------------------------------------------------------
* Fortschritt anzeigen
*---------------------------------------------------------------------------*/
void DownloadDTA::updateProgress(qint64 bytesReceived, qint64 bytesTotal)
{
   ui->progressBar->setMaximum(bytesTotal);
   ui->progressBar->setValue(bytesReceived);
}

/*---------------------------------------------------------------------------
* Verzeichnis auswaehlen
*---------------------------------------------------------------------------*/
void DownloadDTA::on_btnDir_clicked()
{
   QString dir = QFileDialog::getExistingDirectory(
            this,
            tr("Verzeichnis auswählen"),
            ui->editDir->text());
   if( dir != "") ui->editDir->setText(dir);
}
