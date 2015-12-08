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

#ifndef DOWNLOADDTA_H
#define DOWNLOADDTA_H

#include <QtCore>
#include <QDialog>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QFile>

namespace Ui {
    class DownloadDTA;
}

class DownloadDTA : public QDialog
{
    Q_OBJECT

public:
   explicit DownloadDTA(QWidget *parent = 0);
   ~DownloadDTA();
   const QString getFileName() const {return m_fileName;}
   void reject();

signals:

private slots:
   void on_buttonBox_accepted();
   void fileDownloaded(QNetworkReply* reply);
   void readDownload();
   void updateProgress(qint64 bytesReceived, qint64 bytesTotal);

   void on_btnDir_clicked();

private:
   Ui::DownloadDTA *ui;
   QString m_fileName;
   QNetworkAccessManager m_nwam;
   QNetworkReply *m_reply;
   QFile m_file;
};

#endif // DOWNLOADDTA_H
