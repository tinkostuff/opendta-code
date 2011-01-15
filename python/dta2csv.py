#!/usr/bin/env python
#---------------------------------------------------------------------------
# Copyright (C) 2010  opendta@gmx.de
#
# Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen 
# der GNU General Public License, wie von der Free Software Foundation
# veroeffentlicht, weitergeben und/oder modifizieren, entweder gemaess
# Version 3 der Lizenz oder (nach Ihrer Option) jeder spaeteren Version.
#
# Die Veroeffentlichung dieses Programms erfolgt in der Hoffnung, dass es
# Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, sogar ohne die
# implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT FUER EINEN
# BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License.
#
# Sie sollten ein Exemplar der GNU General Public License zusammen mit
# diesem Programm erhalten haben. Falls nicht, 
# siehe <http://www.gnu.org/licenses/>. 
#---------------------------------------------------------------------------
# $Id$
#---------------------------------------------------------------------------
#
# dta2csv.py <liste-mit-DTA-Dateien>
# - konvertiere DTA-Datei in CSV-Format
#
#---------------------------------------------------------------------------

import sys
import os.path
import csv
import datetime
import locale

import dta

#---------------------------------------------------------------------------
# globale Definitionen
#---------------------------------------------------------------------------
GERMAN_CSV = True

if GERMAN_CSV:
	locale.setlocale(locale.LC_ALL, '')

#---------------------------------------------------------------------------
# Argument pruefen
#---------------------------------------------------------------------------
if len(sys.argv)==1 or \
		(len(sys.argv)==2 and sys.argv[1]=="-r"):
	print >>sys.stderr, "Kommando: %s [-r] <Liste-mit-DTA-Dateien>" % os.path.basename(sys.argv[0])
	print >>sys.stderr, "  -r: Rohwerte ausgeben, Datensaetze nicht decodieren"
	sys.exit(0)

rawOutput = False
files = sys.argv[1:]

if sys.argv[1] == "-r": 
	rawOutput = True
	files = sys.argv[2:]

#---------------------------------------------------------------------------
# Dateien bearbeiten
#---------------------------------------------------------------------------
for fileName in files:
	print "Datei: %s" % fileName
	
	# existiert die Datei
	if not os.path.isfile(fileName):
		print >>sys.stderr, "WARNUNG: kann die Datei '%s' nicht finden!" % fileName
		continue

	# Ausgabedatei
	csvName = fileName + ".csv"
	if rawOutput: csvName = fileName + ".raw.csv"

	csvFile = open( csvName, 'wb')
	if GERMAN_CSV:
		csvWriter = csv.writer(csvFile, quoting=csv.QUOTE_MINIMAL, delimiter=';')
	else:
		csvWriter = csv.writer(csvFile, quoting=csv.QUOTE_NONNUMERIC)

	# DTA-Datei oeffnen
	dtaFile = dta.dta(fileName)

	# Dateikopf
	header = dtaFile.readHeader() # Dateikopf
	row = ["Kopf:"]
	row.extend(header)
	csvWriter.writerow(row)

	# leere Zeile
	csvWriter.writerow([])

	# Kopfzeile
	csvWriter.writerow( dtaFile.datasetFieldNames(decoded=(not rawOutput)))

	# Datensaetze
	for ds in dtaFile:
		if rawOutput:
			row = [datetime.datetime.fromtimestamp(ds[0])]
			row.extend(ds[1:])
		else:
			row = dtaFile.decodeDataset(ds)
			if GERMAN_CSV:
				# Dezimal-Separator aendern
				for i in range(len(row)):
					if type(row[i]) == float: row[i] = locale.format('%.2f',row[i])

		csvWriter.writerow(row)

	# Ausgabedatei schliessen
	csvWriter = None
	csvFile.close()


