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
#ifndef DTACOMPSTARTSFRAME_H
#define DTACOMPSTARTSFRAME_H

#include <QFrame>

#include "dtafile/dtafile.h"

QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QComboBox)
class DtaCompStartsThread;
class DtaStatsRun;

class DtaCompStartsFrame : public QFrame
{
    Q_OBJECT
public:
    explicit DtaCompStartsFrame(QWidget *parent = 0);
    ~DtaCompStartsFrame();

    void setData(DtaDataMap *data); // Zeiger auf Daten setzen

signals:

public slots:
    void dataUpdated(); // Daten wurden aktualisiert

 private slots:
    void threadFinished(); // Thread beendet
    void threadTerminated(); // thread abgebrochen
    void updateRunTable(int index); // Tabelle aktualisieren

private:
    // GUI
    QTextEdit *textEdit;
    QTableWidget *table;
    QComboBox *cbModus;

    DtaCompStartsThread *thread; // Thread zur Berechnung
    DtaDataMap *data; // Daten

};

#endif // DTACOMPSTARTSFRAME_H
