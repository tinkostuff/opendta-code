#--------------------------#
# OpenDTA - opendta@gmx.de #
#--------------------------#

Beschreibung:
=============

OpenDTA ist eine Sammlung von Werkzeugen, mit denen DTA-Dateien
   - konvertiert
   - visualisiert
   - statistisch analysiert
werden koennen.

DTA-Dateien werden von der Waermepumpensteuerung Luxtronik 2 der Firma
AlphaInnotec (C) erzeugt. Sie enhalten den Betriebszustand (Temperaturen, 
Eingangs- und Ausgangssignale) der Waermepumpe der letzten 48h.

Baugleiche Steuerung sind: Siemens-Novelan WPR-NET, ?

Author:
=======
   opendta@gmx.de
   http://opendta.sf.net

Werkzeuge:
==========

dtagui:
  - grafische Darstellung und statistische Analyse von DTA-Dateien

dta2csv:
  - Konvertierung von DTA-Dateien in das CSV-Tabellen-Format
  - dabei werden die Rohwerte der DAT-Datei (soweit bekannt) in 
    Messwerte umgerechnet

dtaraw2csv:
  - Konvertierung von DTA-Dateien in das CSV-Tabellen-Format bei der
    nur die Rohwerte ausgegeben werden

Lizenz:
=======

Copyright (C) 2012  opendta@gmx.de

Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
der GNU General Public License, wie von der Free Software Foundation
veroeffentlicht, weitergeben und/oder modifizieren, entweder gemaess
Version 3 der Lizenz oder (nach Ihrer Option) jeder spaeteren Version.

Die Veroeffentlichung dieses Programms erfolgt in der Hoffnung, dass es
Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, sogar ohne die
implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT FUER EINEN
BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License.

Sie sollten ein Exemplar der GNU General Public License zusammen mit
diesem Programm erhalten haben. Falls nicht,
siehe <http://www.gnu.org/licenses/>.
