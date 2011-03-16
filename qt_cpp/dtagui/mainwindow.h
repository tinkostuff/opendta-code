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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "dtafile/dtafile.h"
#include "dtafile/dumpfile.h"

#define VERSION_STRING "$Rev$"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
   void dataChanged();

private:
    Ui::MainWindow *ui; // GUI
    QDialog *helpDialog;
    DataMap data; // alle Daten

    // Pfad zur zuletzt geoeffneten Datei
    QString lastOpenPathDTA;

    // Zaehler fuer die einzelnen Tab-Arten
    quint16 tabDiagramCount;
    quint16 tabStatisticsCount;
    quint16 tabCompStartsCount;

    void readDtaFiles(QStringList files); // DTA-Dateien lesen
    void readDumpFiles(QStringList files); // DUMP-Dateien lesen

    // Dateien per Drag&Dop in der Anwendung laden
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private slots:
    void on_actionHilfe_triggered();
    void on_actionUeberQwt_triggered();
    void on_actionUeberQt_triggered();
    void on_actionOeffnen_triggered();
    void on_actionUeber_triggered();
    void on_actionZuruecksetzen_triggered();
    void on_actionNeuDiagramm_triggered();
    void on_tabWidget_tabCloseRequested(int index);
    void on_actionNeuStatistik_triggered();
    void on_actionNeuKompStarts_triggered();
    void on_actionDUMPOeffnen_triggered();
    void on_tabWidget_customContextMenuRequested(const QPoint &pos);
};

#endif // MAINWINDOW_H
