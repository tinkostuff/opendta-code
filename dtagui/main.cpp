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
* Hauptprogramm
*---------------------------------------------------------------------------*/
#include <QApplication>
#include <QTranslator>
#include <QSettings>

#include "config.h"
#include "mainwindow.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Spracheinstellung laden
    QSettings cfg(
             QSettings::IniFormat,
             QSettings::UserScope,
             ORG_NAME,
             APP_NAME,
             0);
    QString lang = cfg.value( "dtagui/lang", "de").toString();
    QString qmFileName = QString("%1/dtagui_%2.qm").arg(QCoreApplication::applicationDirPath()).arg(lang);

    // Uebersetzung laden
    if (lang != "de")
    {
       QTranslator *translator = new QTranslator;
       if(translator->load(qmFileName))
       {
          app.installTranslator(translator);
       } else {
          qWarning() << QString("WARNING: unable to load translation file '%1'!").arg(qmFileName);
       }
    }

    // Anwendung starten
    MainWindow *w = new MainWindow;
    w->show();

    return app.exec();
}
