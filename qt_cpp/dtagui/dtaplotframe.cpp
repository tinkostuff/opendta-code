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
#include <QtGui>

#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_legend.h>
#include <qwt_plot_printfilter.h>

#include "dtaplotframe.h"
#include "dtafile/dtafile.h"

/*---------------------------------------------------------------------------
* PlotEventHandler
*  - Drag&Drop von Signalen
*  - Popup-Menue
*---------------------------------------------------------------------------*/
class PlotEventHandler : public QObject
{
public:
   PlotEventHandler(DtaPlotFrame *plotFrame, QObject *parent=0) : QObject(parent) { this->plotFrame = plotFrame; }
protected:
   bool eventFilter(QObject *obj, QEvent *event)
   {
      Q_UNUSED(obj)

      bool filterEvent = false;

      switch(event->type())
      {
      //
      // dragEnterEvent
      //
      case QEvent::DragEnter:
         {
            // kommen die Daten vom TreeWidget
            QDragEnterEvent *e = static_cast<QDragEnterEvent *>(event);
            if( e->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist"))
               e->acceptProposedAction();
            break;
         }

      //
      // dropEvent
      //
      case QEvent::Drop:
         {
            QDropEvent *e = static_cast<QDropEvent *>(event);

            // Daten extrahieren
            QByteArray encoded = e->mimeData()->data("application/x-qabstractitemmodeldatalist");
            QDataStream stream(&encoded, QIODevice::ReadOnly);
            while (!stream.atEnd())
            {
                int row, col;
                QHash<int,  QVariant> roleDataHash;
                stream >> row >> col >> roleDataHash;

                QString field = roleDataHash.value(Qt::UserRole).toString();

                // Kurve zeichnen
                DtaPlot *plot = static_cast<DtaPlot *>(obj);
                plotFrame->addCurveToPlot( plot, field);
            }
            e->acceptProposedAction();
            break;
         }

      //
      // mousePress
      //
      case QEvent::MouseButtonPress:
         {
            QMouseEvent *e = static_cast<QMouseEvent *>(event);

            // Popup-Menue
            if( e->button()==Qt::RightButton && e->modifiers()==Qt::NoModifier)
            {
               showPopupMenu = true;
               mouseStartPos = e->globalPos();
            }
            break;
         }

      //
      // mouseMove
      //
      case QEvent::MouseMove:
         {
            QMouseEvent *e = static_cast<QMouseEvent *>(event);
            if(showPopupMenu)
            {
               // wenn sich die Maus bewegt, dann wird das Popup-Menue nicht gezeigt
               QPoint p = e->globalPos() - mouseStartPos;
               if( p.manhattanLength() > 10) showPopupMenu = false;
            }
            break;
         }

      //
      // mouseRelease
      //
      case QEvent::MouseButtonRelease:
         {
            QMouseEvent *e = static_cast<QMouseEvent *>(event);
            // Popup-Menue
            if(e->button()==Qt::RightButton && e->modifiers()==Qt::NoModifier && showPopupMenu)
            {
               DtaPlot *plot = static_cast<DtaPlot *>(obj);
               QStringList curves = plot->curveNames();
               curves.sort();

               // Menue erstellen
               popupMenu = new QMenu(plotFrame);
               QAction *act;

					// Menueeintraege fuer Diagramme
               act = new QAction( tr("&Drucken..."), plot);
               act->setIcon(QIcon(":/images/images/print.png"));
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(printPlot()));
               popupMenu->addAction(act);

               act = new QAction( tr("Diagramm &l\366schen"), plot);
               act->setIcon(QIcon(":/images/images/remove.png"));
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(removePlot()));
               popupMenu->addAction(act);

               // Zoom Menue
               QMenu *zMenu = popupMenu->addMenu("&Zoom");
               act = new QAction( tr("Volle X-Achse"), plot);
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(plotZoomFitX()));
               zMenu->addAction(act);
               act = new QAction( tr("Volle Y-Achse"), plot);
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(plotZoomFitY()));
               zMenu->addAction(act);
               act = new QAction( tr("Volle X- und Y-Achse"), plot);
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(plotZoomFitXY()));
               zMenu->addAction(act);

               // Ticks y-Achse
               QMenu *tickMenu = popupMenu->addMenu(tr("&y-Achse"));
               act = new QAction( tr("max. Haupt-Teilung"), plot);
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(maxMajorYTicks()));
               tickMenu->addAction(act);
               act = new QAction( tr("max. Unter-Teilung"), plot);
               connect( act, SIGNAL(triggered()), plotFrame, SLOT(maxMinorYTicks()));
               tickMenu->addAction(act);

               popupMenu->addSeparator();

               // Menueintraege fuer Kurven erstellen
               for( int i=0; i<curves.size(); i++)
               {
                  QMenu *sigMenu = popupMenu->addMenu(curves.at(i));
                  generateSignalMenu(sigMenu, curves.at(i), plot);
               }

               popupMenu->popup(e->globalPos());
            }
            break;
         }

      //
      // default
      //
      default:
         break;
      }

      return filterEvent;
   }
private:
   void generateSignalMenu(QMenu *parent, QString sigName, DtaPlot *plot)
   {
      QAction *act;

      act = new QAction(tr("L\366schen"),plot);
      act->setData(sigName);
      act->setIcon(QIcon(":/images/images/remove.png"));
      connect( act, SIGNAL(triggered()), plotFrame, SLOT(removeCurveFromPlot()));
      parent->addAction(act);

      act = new QAction(tr("Farbe..."),plot);
      act->setData(sigName);
      act->setIcon(QIcon(":/images/images/colorselect.png"));
      connect( act, SIGNAL(triggered()), plotFrame, SLOT(setCurveColor()));
      parent->addAction(act);

      act = new QAction(tr("Linienst\344rke..."),plot);
      act->setData(sigName);
      act->setIcon(QIcon(":/images/images/linewidth.png"));
      connect( act, SIGNAL(triggered()), plotFrame, SLOT(setCurveLineWidth()));
      parent->addAction(act);
   }
   DtaPlotFrame *plotFrame;
   QMenu *popupMenu;
   bool showPopupMenu;
   QPoint mouseStartPos;
};

/*---------------------------------------------------------------------------
* DtaPlotFrame
*  - UI erstellen
*---------------------------------------------------------------------------*/
DtaPlotFrame::DtaPlotFrame(QWidget *parent) :
    QFrame(parent)
{
   inScaleSync = false;

   // Hauptlayout
   QHBoxLayout *mainLayout = new QHBoxLayout();
   this->setLayout(mainLayout);

   // Baumstruktur fuer Signale
   signalTree = new QTreeWidget();
   signalTree->setDragEnabled(true);
   signalTree->setDragDropMode(QAbstractItemView::DragOnly);
   signalTree->setAlternatingRowColors(true);
   signalTree->setSelectionMode(QAbstractItemView::MultiSelection);
   signalTree->setUniformRowHeights(true);
   signalTree->setHeaderHidden(true);

   // Gruppe fuer den Baum
   QGroupBox *gb= new QGroupBox();
   gb->setTitle(tr("Signale"));
   gb->setMaximumWidth(250);
   QHBoxLayout *gbLayout = new QHBoxLayout();
   gbLayout->addWidget(signalTree);
   gb->setLayout(gbLayout);
   mainLayout->addWidget(gb);

   // Splitter fuer Plots
   plotSplitter = new QSplitter();
   plotSplitter->setOrientation(Qt::Vertical);
   plotSplitter->setAcceptDrops(true);
   mainLayout->addWidget(plotSplitter,100);

   // Baum mit Signalnamen fuellen
   insertFieldsToTree();
}

/*---------------------------------------------------------------------------
* Baum mit Signalnamen fuellen
*  - Felder sind Kategorien zugeordnet
*  - Text mit Endungen verwenden
*  - im Datenfeld "UserRole" befindet sich der Feldname zur Idendifikation
*---------------------------------------------------------------------------*/
void DtaPlotFrame::insertFieldsToTree()
{
   QStringList fields = DtaFile::fieldNames();
   signalTree->clear();

   // Kategorien hinzufuegen
   QStringList cats = DtaFile::fieldCategories();
   for( int i=0; i<cats.size(); i++)
   {
      QTreeWidgetItem *item = new QTreeWidgetItem(QTreeWidgetItem::Type);
      item->setText(0,cats.at(i));
      item->setExpanded(true);
      signalTree->insertTopLevelItem(i,item);
   }

   // Felder hinzufuegen
   for( int i=0; i<fields.size(); i++)
   {
      const DtaFieldInfo *info = DtaFile::fieldInfo(i);

      QTreeWidgetItem *item = new QTreeWidgetItem(QTreeWidgetItem::Type);
      item->setText(0, info->prettyName);
      item->setToolTip(0, info->toolTip);
      item->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      item->setData(0, Qt::UserRole, fields.at(i)); // Feldname zum Identifizieren

      // Icon erstellen
      QPixmap pixmap(20, 20);
      QPainter p(&pixmap);
      p.setBrush(info->color);
      p.setPen(info->color);
      p.drawRect(0,0,20,20);
      p.end();
      QIcon icon(pixmap);
      item->setIcon(0, icon);

      // Kategorie suchen und Feld anhaengen
      QTreeWidgetItem *cat = signalTree->findItems( info->category, Qt::MatchFixedString|Qt::MatchCaseSensitive, 0).first();
      cat->insertChild(0,item);
   }

   // Felder sortieren
   for( int i=0; i<signalTree->topLevelItemCount(); i++)
   {
      QTreeWidgetItem *item = signalTree->topLevelItem(i);
      item->sortChildren(0, Qt::AscendingOrder);
   }

   // alle Felder anzeigen
   signalTree->expandAll();
}

/*---------------------------------------------------------------------------
* Daten-Array zuweisen
*---------------------------------------------------------------------------*/
void DtaPlotFrame::setData(DtaDataMap *data)
{
   this->data = data;
}

/*---------------------------------------------------------------------------
* Diagramm hinzufuegen/entfernen
*---------------------------------------------------------------------------*/
DtaPlot* DtaPlotFrame::addPlot()
{
   // Plot hinzufuegen
   DtaPlot *plot = new DtaPlot(this, plotList.isEmpty());
   plot->installEventFilter(new PlotEventHandler(this));
   plotSplitter->insertWidget( 0, plot);

   // Aenderung der x-Achse des Plots verfolgen
   connect( plot->axisWidget(QwtPlot::xBottom), SIGNAL(scaleDivChanged()),
            this, SLOT(scaleDivChanged()));

   // Plot zur Liste hinzufuegen
   plotList << plot;

   return plot;
}

// alle loeschen
void DtaPlotFrame::clear()
{
   // alle Diagramme loeschen
   for( int i=plotList.size()-1; i>=0; i--)
      delete plotList.at(i);
   plotList.clear();
}

// entfernen
void DtaPlotFrame::removePlot()
{
   // eine Diagramm loeschen (das Diagramm, das das Signal geschickt hat)
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());

   if( !plotList.contains(plot)) return;
   plotList.removeOne(plot);
   delete plot;
}

// alle neu zeichnen
void DtaPlotFrame::replotAll()
{
   for( int i=0; i<plotList.size(); i++)
      plotList.at(i)->replot();
}

/*---------------------------------------------------------------------------
* Kurven
*---------------------------------------------------------------------------*/

// hinzufuegen
void DtaPlotFrame::addCurveToPlot(DtaPlot *plot, QString field)
{
   // Feldinformationen
   const DtaFieldInfo *info = DtaFile::fieldInfo(field);

   // Daten extrahieren
   QPolygonF curveData = extractCurveData(field);

   // Kurve zum Plot hinzufuegen
   plot->addCurve( field, &curveData, info->color, info->analog);
}
void DtaPlotFrame::addCurveToPlot(int index, QString field)
{
   if( plotList.size()-1<index) return;
   addCurveToPlot( plotList.at(index), field);
}

// Daten extrahieren
QPolygonF DtaPlotFrame::extractCurveData(QString field)
{
   // Feldinformationen
   const DtaFieldInfo *info = DtaFile::fieldInfo(field);
   quint16 index = DtaFile::fieldIndex(field);

   QPolygonF result;
   DtaDataMapIterator i(*data);
   while(i.hasNext())
   {
      i.next();
      result << QPointF( i.key(), i.value()[index] * info->scale + info->offset);
   }
   return result;
}

// entfernen
void DtaPlotFrame::removeCurveFromPlot()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   QString curveName = act->data().toString();
   plot->removeCurve(curveName);
}

// Farbe
void DtaPlotFrame::setCurveColor()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   QString curveName = act->data().toString();
   QPen pen = plot->curvePen(curveName);
   QColor color = QColorDialog::getColor(pen.color(),this);
   if( color.isValid())
      pen.setColor(color);
      plot->setCurvePen(curveName, pen);
}

// Linienstaerke
void DtaPlotFrame::setCurveLineWidth()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   QString curveName = act->data().toString();
   QPen pen = plot->curvePen(curveName);
   int width = pen.width();
   if( width == 0) width = 1;
   bool ok;
   width = QInputDialog::getInt( this,
                                 tr("Linienst\344rke"),
                                 tr("Linienst\344rke:"),
                                 width,
                                 0,
                                 100,
                                 1,
                                 &ok);
   if(ok)
   {
      pen.setWidth(width);
      plot->setCurvePen( curveName, pen);
   }
}


/*---------------------------------------------------------------------------
* Daten aller Kurven neu laden und Diagramme neu zeichnen
*---------------------------------------------------------------------------*/
void DtaPlotFrame::update()
{
   // Daten der Kurven aktualisieren
   for( int i=0; i<plotList.size(); i++)
   {
      DtaPlot *plot = plotList.at(i);
      plot->setAllowReplot(false);

      // welche Kurven sind auf dem Diagramm
      QStringList fields = plot->curveNames();

      // jede Kurve aktualisieren
      for( int j=0; j<fields.size(); j++)
      {
         QString field = fields.at(j);
         QPolygonF curveData = extractCurveData(field);
         plot->updateCurveData( field, &curveData);
      }

      plot->fit(DtaPlot::xyDirection);

      plot->setAllowReplot(true);
      plot->replot();
   }
}

/*---------------------------------------------------------------------------
* Diagramme synchronisieren
*---------------------------------------------------------------------------*/

// bei einem Diagramm hat sich die Skalierung geaendert
void DtaPlotFrame::scaleDivChanged()
{
   // Funktion nur einmal aufrufen
   if(inScaleSync) return;
   inScaleSync = true;

   DtaPlot *senderPlot = NULL;
   int axisId = -1;

   // Sender finden
   for( int i=0; i<plotList.size(); i++)
   {
      DtaPlot *plot = plotList.at(i);
      for( int axis=0; axis<QwtPlot::axisCnt; axis++)
      {
         if( plot->axisWidget(axis) == sender())
         {
            senderPlot = plot;
            axisId = axis;
         }
      }
   }

   // Sender gefunden?
   if(senderPlot)
   {
      // Achsen synchronisieren
      for( int i=0; i<plotList.size(); i++)
      {
         DtaPlot *plot = plotList.at(i);
         if( plot != senderPlot)
            plot->setAxisScaleDiv( axisId, *senderPlot->axisScaleDiv(axisId));
      }
   }

   // Achsen ausrichten
   alignPlots();

   // alles neu zeichnen
   replotAll();

   inScaleSync = false;
}

void DtaPlotFrame::alignPlots()
{
   int maxScaleExtentLeft = 0;
   int maxScaleExtentRight = 0;
   int maxLegendWidth = 0;
   int maxBorderDistStart = 0;
   int maxBorderDistEnd = 0;

   // jedes Diagramm bearbeiten
   for( int i=0; i<plotList.size(); i++)
   {
      DtaPlot *plot = plotList.at(i);

      QwtScaleWidget *scaleWidget;
      QwtScaleDraw *sd;

      // maximale Breite der linken y-Achse
      if( plot->axisEnabled(QwtPlot::yLeft))
      {
         scaleWidget = plot->axisWidget(QwtPlot::yLeft);
         sd = scaleWidget->scaleDraw();
         sd->setMinimumExtent(0);
         const int extent = sd->extent(QPen(), scaleWidget->font());
         if( extent > maxScaleExtentLeft) maxScaleExtentLeft = extent;
      }

      // maximale Breite der rechten y-Achse
      if( plot->axisEnabled(QwtPlot::yRight))
      {
         scaleWidget = plot->axisWidget(QwtPlot::yRight);
         sd = scaleWidget->scaleDraw();
         sd->setMinimumExtent(0);
         const int extent = sd->extent(QPen(), scaleWidget->font());
         if( extent > maxScaleExtentRight) maxScaleExtentRight = extent;
      }

      // Breite der x-Achsen Beschriftung
      scaleWidget = plot->axisWidget(QwtPlot::xBottom);
      scaleWidget->setMinBorderDist(0,0);
      const int start = scaleWidget->startBorderDist();
      if( start > maxBorderDistStart) maxBorderDistStart = start;
      const int end = scaleWidget->endBorderDist();
      if( end > maxBorderDistEnd) maxBorderDistEnd = end;

      // maximale Breite der Legende
      QwtLegend *legend = plot->legend();
      for( int j=0; j<legend->legendItems().size(); j++)
      {
         const int width = legend->legendItems().at(j)->sizeHint().width();
         if( width > maxLegendWidth) maxLegendWidth = width;
      }
   }

   // neue Masse setzen
   for( int i=0; i<plotList.size(); i++)
   {
      DtaPlot *plot = plotList.at(i);

      QwtScaleWidget *scaleWidget;

      // linke y-Achse
      if( plot->axisEnabled(QwtPlot::yLeft))
      {
         scaleWidget = plot->axisWidget(QwtPlot::yLeft);
         scaleWidget->scaleDraw()->setMinimumExtent(maxScaleExtentLeft);
      }

      // linke y-Achse
      if( plot->axisEnabled(QwtPlot::yRight))
      {
         scaleWidget = plot->axisWidget(QwtPlot::yRight);
         scaleWidget->scaleDraw()->setMinimumExtent(maxScaleExtentRight);
      }

      // x-Achse
      scaleWidget = plot->axisWidget(QwtPlot::xBottom);
      scaleWidget->setMinBorderDist(maxBorderDistStart,maxBorderDistEnd);

      // Legende
      QwtLegend *legend = plot->legend();
      legend->setMinimumWidth(maxLegendWidth+10);
   }
}

/*---------------------------------------------------------------------------
* Zoom
*---------------------------------------------------------------------------*/
void DtaPlotFrame::plotZoomFitX()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   plot->fit(DtaPlot::xDirection);
}

void DtaPlotFrame::plotZoomFitY()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   plot->fit(DtaPlot::yDirection);
}

void DtaPlotFrame::plotZoomFitXY()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   plot->fit(DtaPlot::xyDirection);
}

/*---------------------------------------------------------------------------
* Achseneinteilung festlegen
*---------------------------------------------------------------------------*/
void DtaPlotFrame::maxMajorYTicks()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   int i = plot->axisMaxMajor(QwtPlot::yLeft);
   bool ok;
   i = QInputDialog::getInt( this,
                             tr("max. Haupt-Teilung"),
                             tr("maximale Anzahl der Haupt-Teilungen:"),
                             i,
                             0,
                             100,
                             1,
                             &ok);
   if(ok)
   {
      plot->setAxisMaxMajor( QwtPlot::yLeft, i);
      plot->replot();
   }
}

void DtaPlotFrame::maxMinorYTicks()
{
   QAction *act = static_cast<QAction *>(QObject::sender());
   DtaPlot *plot = static_cast<DtaPlot *>(act->parent());
   int i = plot->axisMaxMinor(QwtPlot::yLeft);
   bool ok;
   i = QInputDialog::getInt( this,
                             tr("max. Unter-Teilung"),
                             tr("maximale Anzahl der Unter-Teilungen:"),
                             i,
                             0,
                             100,
                             1,
                             &ok);
   if(ok)
   {
      plot->setAxisMaxMinor( QwtPlot::yLeft, i);
      plot->replot();
   }
}

/*---------------------------------------------------------------------------
* Drucken
*---------------------------------------------------------------------------*/
void DtaPlotFrame::printPlot()
{
   QPrinter *printer = new QPrinter;
   QPrintDialog printDialog(printer, this);
   if (printDialog.exec() == QDialog::Accepted) {

      QAction *act = static_cast<QAction *>(QObject::sender());
      DtaPlot *plot = static_cast<DtaPlot *>(act->parent());

      QRect rect;
      QPainter *painter = new QPainter(printer);

      // Kopfzeile drucken
      QFontMetrics fm(painter->font());
      quint32 textHeight= fm.height() + 5;
      rect = QRect(0, 0, printer->width(), fm.height());
      painter->drawText( rect, "DtaGui - http://opendta.sourceforge.net/ - opendta@gmx.de");

      QwtPlotPrintFilter filter;
      // Hintergrund nicht mit drucken
      filter.setOptions( QwtPlotPrintFilter::PrintAll&(~QwtPlotPrintFilter::PrintBackground));

      // Groesse der Seite
      quint32 pageHeight = printer->height() - textHeight;
      rect = QRect( 0, textHeight, printer->width(), pageHeight);

      // Seitenverhaeltnis anpassen
      qreal aspect = qreal(rect.width())/qreal(rect.height());
      if ((aspect < 1.0))
          rect.setHeight(int(aspect*rect.width()));

      // drucken
      plot->print(painter, rect, filter);

      delete painter;
   }
}

void DtaPlotFrame::printAll(QPaintDevice *paintDev)
{
   if(plotList.size()==0) return;

   QRect rect;
   QPainter *painter = new QPainter(paintDev);

   // Kopfzeile drucken
   QFontMetrics fm(painter->font());
   quint32 textHeight= fm.height() + 5;
   rect = QRect(0, 0, paintDev->width(), fm.height());
   painter->drawText( rect, "DtaGui - http://opendta.sourceforge.net/ - opendta@gmx.de");

   QwtPlotPrintFilter filter;
   // Hintergrund nicht mit drucken
   filter.setOptions( QwtPlotPrintFilter::PrintAll&(~QwtPlotPrintFilter::PrintBackground));

   // Groesse der Plots ermitteln
   quint32 totalHeight = 0;
   QList<quint32> plotHeights;
   for( int i=0; i<plotList.size(); i++)
   {
      totalHeight += plotList.at(i)->height();
      plotHeights << plotList.at(i)->height();
   }

   // Groesse der Seite
   quint32 pageHeight = paintDev->height() - textHeight;

   // alle Diagramme drucken (von oben nach unten)
   quint32 pos = textHeight;
   for( int i=plotList.size()-1; i>=0; i--)
   {
      quint32 height = qFloor( qreal(plotHeights.at(i))/qreal(totalHeight)*qreal(pageHeight));
      rect = QRect(0, pos, paintDev->width(), height);
      plotList.at(i)->print( painter, rect, filter);
      pos += height + 1;
   }

   delete painter;
}

/*---------------------------------------------------------------------------
* Siztung (Diagramme mit Signalen) speichern/laden
*---------------------------------------------------------------------------*/
void DtaPlotFrame::saveSession(QString fileName)
{
   QSettings *ini = new QSettings(fileName, QSettings::IniFormat, this);
   ini->clear();
   ini->setValue( "global/diagrams", plotList.size());

   for( int i=0; i<plotList.size(); i++)
   {
      DtaPlot *plot = plotList.at(i);
      QStringList curves = plot->curveNames();
      ini->setValue( QString("diagram%1/curves").arg(i), curves.size());
      ini->setValue( QString("diagram%1/leftMaxMajorTicks").arg(i), plot->axisMaxMajor(QwtPlot::yLeft));
      ini->setValue( QString("diagram%1/leftMaxMinorTicks").arg(i), plot->axisMaxMinor(QwtPlot::yLeft));

      for( int j=0; j<curves.size(); j++)
      {
         ini->setValue( QString("diagram%1/curve%2/name").arg(i).arg(j),
                        curves.at(j));
         ini->setValue( QString("diagram%1/curve%2/color").arg(i).arg(j),
                        plot->curvePen(curves.at(j)).color());
         ini->setValue( QString("diagram%1/curve%2/linewidth").arg(i).arg(j),
                        plot->curvePen(curves.at(j)).width());
         ini->setValue( QString("diagram%1/curve%2/visible").arg(i).arg(j),
                        plot->isCurveVisible(curves.at(j)));
      }
   }

   delete ini;
}

void DtaPlotFrame::loadSession(QString fileName)
{
   QSettings *ini = new QSettings(fileName, QSettings::IniFormat, this);

   // alle Diagramme loeschen
   this->clear();

   // Anzahl der Diagramme
   qint32 diagrams = ini->value("global/diagrams", -1).toInt();
   if(diagrams==0)
   {
      QMessageBox::warning(
            this,
            tr("Fehler beim Laden der Sitzung"),
            QString(tr("Fehler beim Laden der Sitzung!\nSchl\374ssel 'global/diagrams' nicht gefunden!")));
      return;
   }

   for( int i=0; i<diagrams; i++)
   {
      // Diagramm erstellen
      DtaPlot *plot = this->addPlot();

      // neu zeichnen verhindern
      plot->setAllowReplot(false);

      // Anzahl der Kurven
      qint32 curves = ini->value(QString("diagram%1/curves").arg(i), 0).toInt();

      for( int j=0; j<curves; j++)
      {
         // Name der Kurve
         QString curve = ini->value(QString("diagram%1/curve%2/name").arg(i).arg(j)).toString();
         this->addCurveToPlot( i, curve);

         // Farbe und Linienstaerke
         qint32 width = ini->value(QString("diagram%1/curve%2/linewidth").arg(i).arg(j),1).toInt();
         QColor color = ini->value(QString("diagram%1/curve%2/color").arg(i).arg(j),Qt::red).value<QColor>();
         QPen pen;
         pen.setColor(color);
         pen.setWidth(width);
         plot->setCurvePen( curve, pen);

         // sichtbar oder nicht
         bool visible = ini->value(QString("diagram%1/curve%2/visible").arg(i).arg(j),true).toBool();
         plot->showCurve( curve, visible);
      }

      // Achseneinteilung - erst jetzt laden, weil sie beim Einfuegen der
      // Kurven wieder veraendert werden
      plot->setAxisMaxMajor( QwtPlot::yLeft,
                             ini->value( QString("diagram%1/leftMaxMajorTicks").arg(i), 1).toInt());
      plot->setAxisMaxMinor( QwtPlot::yLeft,
                             ini->value( QString("diagram%1/leftMaxMinorTicks").arg(i), 1).toInt());

      // neu zeichen wieder zulassen
      plot->setAllowReplot(true);
   }

   delete ini;

   // alle Diagramm neu zeichnen
   replotAll();
}
