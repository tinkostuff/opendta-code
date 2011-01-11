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
# dta.py
# - Klasse zum Lesen und Decodieren von DTA-Dateien
# - Alpha-InnoTec Waermepumpen Steuerung 'Luxtronik 2' speichert die Daten
#   der letzten 48h in DTA-Dateien
#
#---------------------------------------------------------------------------

import os.path
from os import SEEK_SET, SEEK_CUR, SEEK_END
import struct
import datetime
import time

# Format der DTA-Dateien
from dta_format import *

#---------------------------------------------------------------------------
# Exception Class
#---------------------------------------------------------------------------
class DtaError(StandardError):
	pass

#---------------------------------------------------------------------------
# dta class
#---------------------------------------------------------------------------
class dta(object):

	#---------------------------------------------------------------------------
	# __init__
	#---------------------------------------------------------------------------
	def __init__( self, dtaFileName):

		self._fileHandle = None
		
		if type(dtaFileName) not in [str,unicode]:
			raise TypeError( "FEHLER: dtaFileName muss vom Typ 'str' oder 'unicode' sein!")

		if not os.path.isfile(dtaFileName):
			raise ValueError( "FEHLER: DTA-Datei '%s' nicht gefunden!" % dtaFileName)

		# private Variablen
		self._fileName = dtaFileName
		self._fileHandle = open( dtaFileName, 'rb')
		self._lastDS = None      # letzter gelesener Datensatz
		self._oldestDSPos = None # Position des aeltesten Datensatzes

		if self._fileHandle == None:
			raise DtaError( "FEHLER: DTA-Datei '%s' konnte nicht geoeffnet werden!" % dtaFileName)

		# Anzahl der Datensaetze ermitteln und pruefen
		size = os.path.getsize(self._fileName)
		dsCount = (size - dtaHeaderLength) / dtaDatasetLength
		if dsCount != dtaDatasetCount:
			raise DtaError( "FEHLER: Anzahl der Datensaetze (%d) weicht vom erwarteten Wert (%d) ab!" % (dsCount,dtaDatasetCount))

		# Dateikopf lesen, damit der Zeiger auf dem ersten Datensatz steht
		self.readHeader()

	#---------------------------------------------------------------------------
	# __del__
	#---------------------------------------------------------------------------
	def __del__(self):
		if self._fileHandle != None:
			self._fileHandle.close()
			self._fileHandle = None

	#---------------------------------------------------------------------------
	# Dateikopf lesen
	#---------------------------------------------------------------------------
	def readHeader(self):
		"""Dateikopf lesen und als Zeichenkette zurueckgeben
		   - an den Anfang der Datei springen
			- Dateikopf lesen
			- Zeiger bleibt hinter dem Dateikopf stehen
		"""
		self._fileHandle.seek( 0, SEEK_SET)
		header = self._fileHandle.read(dtaHeaderLength)
		if len(header) < dtaHeaderLength:
			return None
		else:
			return struct.unpack( dtaHeaderUnpackFormat, header)

	#---------------------------------------------------------------------------
	# Datensatz Lesefunktionen
	#---------------------------------------------------------------------------
	def readDataset(self):
		"""einen Datensatz lesen auspacken und Liste zureuckgeben"""
		ds = self._fileHandle.read(dtaDatasetLength)
		if len(ds) < dtaDatasetLength:
			self._lastDS = None
		else:
			self._lastDS = struct.unpack( dtaDatasetUnpackFormat, ds)
		return self._lastDS

	#---------------------------------------------------------------------------
	# Iterator fuer Datensaetze
	#---------------------------------------------------------------------------
	def __iter__(self): return self
	def next(self):
		ds = self.readDataset()
		if ds == None: raise StopIteration
		return ds

	#---------------------------------------------------------------------------
	# Datensatz decodieren
	#---------------------------------------------------------------------------
	def decodeDataset(self, dataset=None, returnDict=False):
		"""Datensatz decodieren:
		   - Datum/Zeit umrechnen
			- Status der Ein- und Ausgaenge aufloesen
			- Temperaturen ausrechnen
			Rueckgabe: 
			   returnDict==False: Liste mit allen Werten (wenn moeglich decodiert)
				returnDict==True:  Dict mit allen decodierten Werten
		"""
		ds = dataset
		if ds == None: ds = self._lastDS

		if returnDict: result = {}
		else: result = []

		for i in xrange(len(ds)):
			fieldName = None
			if dtaDsFieldNames.has_key(i): fieldName = dtaDsFieldNames[i]

			#
			# unbekanntes Feld
			#
			if fieldName == None:
				if not returnDict: result.append(ds[i])

			#
			# Datum
			#
			elif fieldName == "Datum":
				if returnDict: 
					result['Datum'] = datetime.datetime.fromtimestamp(ds[i])
				else: 
					result.append(datetime.datetime.fromtimestamp(ds[i]))

			#
			# StatusA
			#
			elif fieldName in ["StatusA", "StatusE"]:
				if returnDict:
					stateFields = dtaDsStateOutputs
					if fieldName == "StatusE": stateFields = dtaDsStateInputs
					for j in xrange(16):
						if stateFields.has_key(j):
							result[stateFields[j]] = (ds[i]>>j) & 1
				else:
					# Integer aufloesen und Binaer-Wert ausgeben
					for j in xrange(16):
						result.append( (ds[i]>>j) & 1)

			#
			# Temperaturen
			#
			elif fieldName in dtaDsTempCoeffs.keys():
				(m,n) = dtaDsTempCoeffs[fieldName]
				value = ds[i] * m + n
				if returnDict: result[fieldName] = value
				else: result.append(value)

		return result

	#---------------------------------------------------------------------------
	# Datensatz Felder
	#---------------------------------------------------------------------------
	def datasetFieldNames(self, decoded=False):
		"""Liste der Feldnamen es Datensatzes zurueckgeben"""
		result = []
		for i in xrange(dtaDatasetFieldCount):
			if dtaDsFieldNames.has_key(i):
				fieldName = dtaDsFieldNames[i]

				# Namen der Zustaende mit ausgeben
				if decoded and fieldName in ["StatusE", "StatusA"]:
					bitNames = dtaDsStateOutputs
					if fieldName == "StatusE": bitNames = dtaDsStateInputs
					for j in xrange(16):
						if bitNames.has_key(j): 
							result.append( "%s:%s" % (fieldName,bitNames[j]))
						else:
							result.append( "%s:bit%d" % (fieldName,j))
				else:
					result.append(fieldName)
			else:
				result.append(str(i))
		return result

#---------------------------------------------------------------------------
# MAIN
#---------------------------------------------------------------------------
if __name__ == "__main__":
	import sys

	if len(sys.argv) == 1: sys.exit(0)

	# DTA-Datei oeffnen
	dtaFileName = sys.argv[1]
	d = dta(sys.argv[1])

	# Dateikopf
	print "Dateikopf:", d.readHeader()

	# erster Datensatz
	print "Laenge Datensatz:", len(d.readDataset())
	print "Datensatz:", d.decodeDataset()
	print "decodierter Datensatz:", d.decodeDataset()
	print "Datensatz (dict):", d.decodeDataset(returnDict=True)

	# Feldnamen
	print "Feldnamen:", d.datasetFieldNames()
	print "Feldnamen (decodierter Datensatz):", d.datasetFieldNames(decoded=True)

	# Iterator
	n = 0
	for ds in d:
		n += 1
	print "Anzahl restliche Datensaetze:", n

