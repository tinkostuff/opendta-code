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

class DtaPlotFrame : public QFrame
{
    Q_OBJECT
public:
    explicit DtaPlotFrame(QWidget *parent = 0);
    void setData(DtaDataMap *data);

    DtaPlot* addPlot(); // Diagramm hinzufuegen
    void clear(); // alle Diagramme loeschen
    void update(); // Daten wurden aktualisiert
    void saveSession(QString fileName); // Siztung speichern
    void loadSession(QString fileName); // Sitzung laden
    void printAll(QPaintDevice *paintDev); // alle Diagramme drucken

    // Kurve zu Diagramm hinzufuegen
    void addCurveToPlot( DtaPlot *plot, QString field);
    void addCurveToPlot( int index, QString field);
signals:

private slots:
    void scaleDivChanged(); // Skalierung muess geaendert werden
    void removeCurveFromPlot(); // Kurve entfernen
    void setCurveColor(); // Farbe der Kurve setzen
    void setCurveLineWidth(); // Linienstaerke der Kurve setzen
    void removePlot(); // einzelnes Diagramm entfernen
    void printPlot(); // Diagramm drucken

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
    QList<DtaPlot*> plotList; // Liste mit Diagrammen
    DtaDataMap *data; // Zeiger auf Daten
    bool inScaleSync; // true wenn Diagramme synchronisiert werden

    void insertFieldsToTree(); // Baum mit Signalen fuellen
    QPolygonF extractCurveData(QString field); // Daten einer Kurve extrahieren
    void alignPlots(); // Diagramme ausrichten
    void replotAll(); // alle Diagramme neu zeichen
};

#endif // DTAPLOTFRAME_H
