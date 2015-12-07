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
/*---------------------------------------------------------------------------
* DtaStatsFrame
*  - ein bisschen Statistik mit den Daten
*  - die Funktion 'dataUpdated()' startet einen Thread, der die Hauptarbeit
*    verrichtet
*  - 'threadFinished()' sammelt die Ergebnisse ein und stellt sie dar
*---------------------------------------------------------------------------*/
#ifndef DTASTATSFRAME_H
#define DTASTATSFRAME_H

#include <QFrame>
#include "dtafile/dtafile.h"

QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)

class DtaStatsThread;

class DtaStatsFrame : public QFrame
{
   Q_OBJECT
public:
   explicit DtaStatsFrame(DataMap *data, QWidget *parent = 0);
   ~DtaStatsFrame();

signals:

public slots:
   void dataUpdated(); // Daten wurden aktualisiert

private slots:
   void threadFinished();  // Thread wurde normal beendet
   void print(); // Ergebnisse ausdrucken
   void setCompleteTimeRange();

private:
   QTextEdit *textEdit;
   QDateTimeEdit *dteStart;
   QDateTimeEdit *dteEnd;
   DtaStatsThread *thread; // Thread
   DataMap *data; // Zeiger auf die Daten

   void updateTimeRangeEdit(); // Zeitspanne der Eingabefelder aktualisieren
};

#endif // DTASTATSFRAME_H
