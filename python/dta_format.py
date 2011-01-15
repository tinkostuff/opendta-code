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
# - Format-Definition einer DTA-Datei und eines DTA-Datensatzes
#
#---------------------------------------------------------------------------
#
# Aufbau der DTA-Dateien:
#  - die ersten 8 Byte sind ein Header
#  - dann folgen 2880 Datensaetze (48 Stunden, ein Datensatz pro Minute)
#  - jeder Datensatz ist 168 Byte lang
#  - die Byte-Order ist little-endian
#  - die Datensaetze sind nur bedingt sortiert, sie werden in einem 
#    Round-Robin-Verfahren gespeichert (die Schreibposition rueckt mit jedem
#    Datensatz weiter und springt am Ende der Datei wieder an den Anfang)
#  
# Aufbau eines Datensatzes
#  -   [0:3]   Datum und Uhrzeit in Sekunden von 1.1.1970 (Unixzeit)
#  -   [8:9]   StatusA = Status der Ausgaenge 
#        bit 0:  HUP  = Heizungsumwaelzpumpe
#        bit 1:  ZUP  = Zusatzumwaelzpumpe
#        bit 2:  BUP  = Brauswarmwasserumwaelzpumpe oder Drei-Wege-Ventil auf Brauchwassererwaermung
#        bit 3:  ZW2  = Zusaetzlicher Waermeerzeuger 2 / Sammelstoerung
#        bit 4:  MA1  = Mischer 1 auf
#        bit 5:  MZ1  = Mischer 1 zu
#        bit 6:  ZIP  = Zirkulationspumpe
#        bit 7:  VD1  = Verdichter 1
#        bit 8:  VD2  = Verdichter 2
#        bit 9:  VENT = Ventilation des WP Gehaeses / 2. Stufe des Ventilators
#        bit 10: AV   = Abtauventil (Kreislaufumkehr)
#        bit 11: VBS  = Ventilator, Brunnen- oder Soleumwaelzpumpe
#        bit 12: ZW1  = Zusaetzlicher Waermeerzeuger 1
#  -   [44:45] StatusE = Status der Eingaenge (die Bits sind invertiert zur Funktion)
#        bit 0:  HD_  = Hochdruckpressostat
#        bit 1:  ND_  = Niederdruckpressostat
#        bit 2:  MOT_ = Motorschutz
#        bit 3:  ASD_ = Abtau/Soledruck/Durchfluss
#        bit 4:  EVU_ = EVU Sperre
#  -   [52:53] TFB1    = Temperatur Fussbodenheizung 1
#  -   [54:55] TBW     = Temperatur Brauch-Warm-Wasser
#  -   [56:57] TA      = Aussentemperatur
#  -   [58:59] TRL     = Temperatur Heizung Ruecklauf extern
#  -   [60:61] TRL     = Temperatur Heizung Ruecklauf
#  -   [62:63] TVL     = Temperatur Heizung Vorlauf
#  -   [64:65] THG     = Temperatur Heissgas
#  -   [66:67] TWQaus  = Temperatur Waermequelle Austritt
#  -   [70:71] TWQein  = Temperatur Waermequelle Eintritt
#  -   [80:81] TRLsoll = Solltemperatur Heizung Ruecklauf
#  -   [82:83] TRLsoll_highbytes = zwei Extra-Byte fuer TRLsoll (werden nicht ausgelesen)
#  -   [84:85] TMK1soll= Solltemperatur Mischer Kreis 1
#  -   [86:87] TMK1soll_highbytes = zwei Extra-Byte fuer TMK1soll (werden nicht ausgelesen)
#  - [128:129] ComfortPlatine: Indikator, ob und welche Erweiterung eingebaut ist
#  - [132:133] StatusA_CP = Status der Ausgaenge der ComfortPlatine
#        bit 6:  AI1DIV = Spannungsteiler an AI1: wann AI1DIV dann AI1 = AI1/2
#        bit 7:  SUP = Schwimmbadumwaelzpumpe
#        bit 8:  FUP2 = Mischkreispumpe 2 / Kuehlsignal 2
#        bit 9:  MA2 = Mischer 2 auf
#       bit 10:  MZ2 = Mischer 2 zu
#       bit 11:  MA3 = Mischer 3 auf
#       bit 11:  MZ3 = Mischer 3 zu
#       bit 12:  FUP3 = Mischkreispumpe 3 / Kuehlsignal 3
#       bit 14:  ZW3 = Zusaetzlicher Waermeerzeuger 3
#       bit 15:  SLP = Solarladepumpe
#  - [136:137] AO1 = ComfortPlatine: Analoger Ausgang 1
#  - [138:139] AO2 = ComfortPlatine: Analoger Ausgang 2
#  - [140:141] StatusE_CP = Status der Eingaenge der ComfortPlatine
#        bit 4:  SWT_ = Schwimmbadthermostat
#  - [158:159] AI1 = ComfortPlatine: Analoger Eingang 1
#  
# Umrechnung der Werte:
#  Fuer die oben genannten Werte sind in den Datensaetzen natuerliche
#  Zahlen gespeichert. Diese muessen noch in die eigentlichen
#  Temperatur- und Spannungswerte umgerechnet werden. Dies erfolgt entweder
#  ueber eine lineare Berechnung (TRLsoll, TMK1soll, AO1, AO2 und AI1) oder
#  mit Hilfe einer Wertetabelle (restliche Temperaturen).
#
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Datei
#---------------------------------------------------------------------------
dtaDatasetLength       = 168  # bytes
dtaDatasetCount        = 2880 # 48h * 60min
dtaTimeInterval        = 60   # Sekunden
dtaDatasetUnpackFormat = "<iHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
dtaDatasetFieldCount   = 83

#---------------------------------------------------------------------------
# Dateikopf
#---------------------------------------------------------------------------
dtaHeaderLength        = 8    # bytes
dtaHeaderUnpackFormat  = "<II"

#---------------------------------------------------------------------------
# Datensatz
#---------------------------------------------------------------------------

# Status der Ausgaenge
dtaDsStateOutputs = {
		#   Pos: Name
            0: 'HUP',  # Heizungsumwaelzpumpe
            1: 'ZUP',  # Zusatzumwaelzpumpe
            2: 'BUP',  # Brauswarmwasserumwaelzpumpe oder Drei-Wege-Ventil auf Brauchwassererwaermung
            3: 'ZW2',  # Zusaetzlicher Waermeerzeuger 2 / Sammelstoerung
            4: 'MA1',  # Mischer 1 auf
            5: 'MZ1',  # Mischer 1 zu
            6: 'ZIP',  # Zirkulationspumpe
            7: 'VD1',  # Verdichter 1
            8: 'VD2',  # Verdichter 2
            9: 'VENT', # Ventilation des WP Gehaeses / 2. Stufe des Ventilators
           10: 'AV',   # Abtauventil (Kreislaufumkehr)
           11: 'VBS',  # Ventilator, Brunnen- oder Soleumwaelzpumpe
           12: 'ZW1',  # Zusaetzlicher Waermeerzeuger 1
		}

# Status der Ausgaenge der ComfortPlatine
dtaDsStateOutputsCP = {
		#   Pos: Name
			   6: 'AI1DIV', # wenn AI1DIV dann AI1 = AI1/2
            7: 'SUP',  # Schwimmbadumwaelzpumpe
				8: 'FUP2', # Mischkreispumpe 2 / Kuehlsignal 2
				9: 'MA2',  # Mischer 2 auf
			  10: 'MZ2',  # Mischer 2 zu
			  11: 'MA3',  # Mischer 3 auf
			  12: 'MZ3',  # Mischer 3 zu
			  13: 'FUP3', # Mischkreispumpe 3 / Kuehlsignal 3
			  14: 'ZW3',  # Zusaetzlicher Waermeerzeuger 3
			  15: 'SLP',  # Solarladepumpe
			  # AI2, AI3?
		}

# Status der Eingaenge
dtaDsStateInputs = {
		# Eingaenge mit '_' sind invers zum Bit-Wert
		#   Pos: Name
            0: 'HD_',  # Hochdruckpressostat
				1: 'ND_',  # Niederdruckpressostat
				2: 'MOT_', # Motorschutz
				3: 'ASD_', # Abtau/Soledruck/Durchfluss
            4: 'EVU_', # EVU Sperre
		}

# Status der Eingaenge der ComfortPlatine
dtaDsStateInputsCP = {
		# Eingaenge mit '_' sind invers zum Bit-Wert
		#   Pos: Name
            4: 'SWT_',  # Schwimmbadthermostat
		}

# Werte-Tabelle fuer TRL, TVL, TBW, TFB1, TRLext
dtaDsDataConversionLUT1 = {
		"Offset": 0,
		 "Delta": 10,
		  "Data": ( 155.0, 155.0, 155.0, 143.8, 130.5, 120.5, 112.8, 106.3, 100.7, 95.9, 91.6, 87.8, 84.3, 81.1, 78.3, 75.6, 73.2, 70.8, 68.5, 66.4, 64.7, 62.5, 60.7, 59.0, 57.4, 55.8, 54.3, 52.9, 51.5, 50.1, 48.7, 47.4, 46.1, 44.8, 43.6, 42.4, 41.2, 40.1, 39.0, 37.9, 36.8, 35.8, 34.8, 33.8, 32.8, 31.8, 30.8, 29.9, 28.9, 27.9, 26.9, 26.0, 25.0, 24.1, 23.2, 22.3, 21.4, 20.5, 19.6, 18.7, 17.8, 17.0, 16.1, 15.2, 14.4, 13.5, 12.7, 11.8, 10.9, 10.0, 9.2, 8.3, 7.4, 6.5, 5.6, 4.7, 3.8, 2.9, 2.0, 1.1, 0.1, -0.7, -1.7, -2.6, -3.7, -4.8, -5.8, -6.8, -7.8, -9.0, -10.2, -11.2, -12.4, -13.7, -15.0, -16.2, -17.5, -19.0, -20.5, -22.0, -23.7, -25.5, -27.3 ),
	}

# Werte-Tabelle fuer TWQein, TWQaus, TA
dtaDsDataConversionLUT2 = {
		"Offset": 0,
		 "Delta": 10,
		  "Data": ( 155.0, 143.5, 113.3, 97.1, 86.2, 78.1, 71.8, 66.4, 61.8, 57.9, 54.5, 51.4, 48.6, 46.0, 43.5, 41.3, 39.2, 37.2, 35.4, 33.7, 32.1, 30.6, 29.0, 27.6, 26.1, 24.8, 23.5, 22.3, 21.0, 19.9, 18.8, 17.7, 16.6, 15.6, 14.6, 13.6, 12.7, 11.7, 10.8, 9.9, 9.0, 8.2, 7.3, 6.5, 5.7, 4.8, 4.0, 3.2, 2.5, 1.7, 0.9, 0.1, -0.5, -1.2, -2.0, -2.7, -3.5, -4.2, -5.0, -5.7, -6.3, -7.0, -7.7, -8.5, -9.2, -10.0, -10.7, -11.3, -12.0, -12.7, -13.4, -14.2, -15.0, -15.6, -16.3, -16.9, -17.7, -18.4, -19.2, -20.0, -20.7, -21.4, -22.1, -22.9, -23.7, -24.6, -25.4, -26.1, -26.9, -27.7, -28.6, -29.6, -30.4, -31.3, -32.2, -33.1, -34.2, -35.2, -36.2, -37.2, -38.4, -39.6, -40.8 ),
	}

# Werte Tabelle fuer THG
dtaDsDataConversionLUT3 = {
		"Offset": 0,
		 "Delta": 10,
		  "Data": (155.0, 155.0, 155.0, 155.0, 155.0, 155.0, 155.0, 153.7, 146.8, 140.9, 135.7, 131.1, 126.8, 122.9, 119.3, 116.0, 113.0, 110.0, 107.4, 104.8, 102.4, 100.0, 97.8, 95.6, 93.6, 91.6, 89.6, 87.9, 86.1, 84.3, 82.7, 81.1, 79.5, 78.0, 76.5, 75.0, 73.7, 72.3, 70.9, 69.5, 68.1, 66.8, 65.5, 64.6, 63.0, 61.7, 60.5, 59.3, 58.1, 57.0, 55.8, 54.7, 53.5, 52.4, 51.3, 50.2, 49.0, 47.9, 46.7, 45.6, 44.4, 43.3, 42.1, 41.0, 39.8, 38.7, 37.6, 36.4, 35.3, 34.1, 33.0, 31.8, 30.6, 29.4, 28.2, 26.9, 25.6, 24.3, 23.0, 21.7, 20.3, 18.9, 17.5, 16.1, 14.6, 13.1, 11.6, 9.9, 8.3, 6.5, 4.7, 2.7, 0.7, -1.4, -3.7, -6.2, -9.0, -12.0, -15.5, -19.4, -24.0, -30.0, -37.8 ),
	}

# Konvertierungen:
#   Datum
#   Bits:
#      - jedes Bit des Wertes wir individuell behandelt
#      - Daten: Woerterbuch mit Bitposition: Name
#   Linear:
#      - lineare Umrechnung der Roh-Werte
#      - Daten: m, n, Praezision
#      - Umrechnung: Werte = Roh-Wert * m + n, 
#      - Rundung auf 1/'Praezision' genau
#   LookUp:
#      - Nachschlagen der Roh-Werte in einer Wertetabelle
#      - lineare Approximation bei Werten zwischen den Stuetzstellen
#      - Daten: Woerterbuch mit den Eintraegen 'Offset', 'Delta' und 'Data'
#      - Umrechnung: 'Data' ist eine Array mit Ergebniss-Werten die zu den
#            Roh-Werten Index * Delta + Offset passen.
#      - Rundung: auf eine Stelle nach dem Komma

# Felder eines Datensatzes
dtaDsFields= {
		# Position in Array: ( Name,         Konvertierung, Daten)               # Position im Array
						  0    : ( 'Datum',      'Datum', None),                     # 0
						  8/2-1: ( 'StatusA',    'Bits',  dtaDsStateOutputs),        # 3
						 44/2-1: ( 'StatusE',    'Bits',  dtaDsStateInputs),         # 21
						 52/2-1: ( 'TFB1',       'LookUp', dtaDsDataConversionLUT1), # 25
						 54/2-1: ( 'TBW',        'LookUp', dtaDsDataConversionLUT1), # 26 - wenn DS[0] kleiner 47, dann wird TBW nicht im LuxAdmin angezeigt, sondern Eingang BWT
						 56/2-1: ( 'TA',         'LookUp', dtaDsDataConversionLUT2), # 28
						 58/2-1: ( 'TRLext',     'LookUp', dtaDsDataConversionLUT1), # 28
						 60/2-1: ( 'TRL',        'LookUp', dtaDsDataConversionLUT1), # 29
						 62/2-1: ( 'TVL',        'LookUp', dtaDsDataConversionLUT1), # 30
						 64/2-1: ( 'THG',        'LookUp', dtaDsDataConversionLUT3), # 31
						 66/2-1: ( 'TWQaus',     'LookUp', dtaDsDataConversionLUT2), # 32
						 70/2-1: ( 'TWQein',     'LookUp', dtaDsDataConversionLUT2), # 34
						 80/2-1: ( 'TRLsoll',    'Linear', (0.1, 0.0, 1)),           # 39
						# 82/2-: ( 'TRLsoll_highbyte',                               # 40
						 84/2-1: ( 'TMK1soll',   'Linear', (0.1, 0.0, 1)),           # 41
						# 86/2-: ( 'TMK1soll_highbyte',                              # 42
						#128/2-1: ( 'ComPlat', # 63 - ComfortPlatine: 
						#                    #           1,3 = TSS;TSK;TFB2;TFB3;TEE;AI;AO1;AO2;MK2-Soll;MK3-Soll;SWT;SUP;FP2;MA2;MZ2;MA3;MZ3;FP3;ZW3;SLP
						#						   #                  (Schwimmbad, Kuehlung, Solar)
						#						   #             2 = SPL;SAX;VSK;FRH;SAX(LED);TZU;TAB;AIN2;AIN3;AO3; AO4; VZU; VAB
						# 130/2-1: 'CompPlat_highbyte',                              # 64
						132/2-1: ( 'StatusA_CP', 'Bits',   dtaDsStateOutputsCP),     # 65
						136/2-1: ( 'AO1',        'Linear', (0.002619, 0.0, 2)),      # 67
						138/2-1: ( 'AO2',        'Linear', (0.002619, 0.0, 2)),      # 68
						140/2-1: ( 'StatusE_CP', 'Bits',   dtaDsStateInputsCP),      # 69
						158/2-1: ( 'AI1',        'Linear', (0.003631, 0.0, 2)),      # 78
		}                    

#---------------------------------------------------------------------------
# MAIN
#---------------------------------------------------------------------------
if __name__ == "__main__":

	import pprint
	pprint.pprint(locals())

