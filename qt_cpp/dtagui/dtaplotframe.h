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
* DtaPlotFrame
*  - Verwaltung aller Diagramme
*  - Baum mit verfuegbaren Signalen
*---------------------------------------------------------------------------*/
#ifndef DTAPLOTFRAME_H
#define DTAPLOTFRAME_H

#include <QFrame>

#include "dtaplot/dtaplot.h"
#include "dtafile/dtafile.h"

QT_FORWARD_DECLARE_CLASS(QPolygonF)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QCheckBox)

class DtaPlotFrame : public QFrame
{
    Q_OBJECT
public:
    explicit DtaPlotFrame(DtaDataMap *data, QWidget *parent = 0);

   // Kurve zu Diagramm hinzufuegen
   void addCurveToPlot( DtaPlot *plot, QString field);
   void addCurveToPlot( int index, QString field);

public slots:
    void dataUpdated(); // Daten wurden aktualisiert

private slots:
    DtaPlot* addPlot(); // Diagramm hinzufuegen
    void clear(); // alle Diagramme loeschen
    void saveSession(); // Siztung speichern
    void loadSession(); // Sitzung laden
    void printAll(); // alle Diagramme drucken

    void scaleDivChanged(); // Skalierung muess geaendert werden
    void removeCurveFromPlot(); // Kurve entfernen
    void setCurveColor(); // Farbe der Kurve setzen
    void setCurveLineWidth(); // Linienstaerke der Kurve setzen
    void removePlot(); // einzelnes Diagramm entfernen
    void printPlot(); // Diagramm drucken
    void setSymbols(int checkState); // Datenpunkte ein-/aus-blenden

    // Zoom fit
    void plotZoomFitX();
    void plotZoomFitY();
    void plotZoomFitXY();

    // Haupt- und Unter-Teilung der y-Achse festlegen
    void maxMajorYTicks();
    void maxMinorYTicks();

private:
    // UI
    QSplitter *plotSplitter; // Splitter zwischen den Diagrammen
    QTreeWidget *signalTree; // Baum fuer die Signale
    QCheckBox *cbSymbols;    // CheckBox zum Anzeigen der Datenpunkte
    QList<DtaPlot*> plotList; // Liste mit Diagrammen
    DtaDataMap *data; // Zeiger auf Daten
    bool inScaleSync; // true wenn Diagramme synchronisiert werden

    QString lastOpenPathSession;
    bool loadDefaultSession; // sollen die Standard-Diagramme geladen werden

    void insertFieldsToTree(); // Baum mit Signalen fuellen
    QPolygonF extractCurveData(QString field); // Daten einer Kurve extrahieren
    void alignPlots(); // Diagramme ausrichten
    void replotAll(); // alle Diagramme neu zeichen
    void loadSession(QString fileName); // Sitzung laden
};

#endif // DTAPLOTFRAME_H
