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
#  Aufbau eines Datensatzes
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
#        weitere Ausgaenge: 
#            FUP1 = Fussbodenheizungsumwaelzpumpe 1
#            ZW3  = Zusaetzlicher Waermeerzeuger 3
#            SLP  = Solarladepumpe
#            SUP  = Schwimmbadumwaelzpumpe
#            FUP2 = Mischkreispumpe 2 / Kuehlsignal 2
#            MA2  = Mischer 2 auf
#            MZ2  = Mischer 2 zu
#            FUP3 = Mischkreispumpe 3 / Kuehlsignal 3
#            MA3  = Mischer 3 auf
#            MZ3  = Mischer 3 zu
#  -   [44:45] StatusE = Status der Eingaenge
#        bit 3:  EVU Sperre
#        weitere Eingaenge: 
#            ?    = Abtau/Soledruck/Durchfluss
#            BWT  = Brauchwasserthermostat
#            HD   = Hochdruckpressostat
#            MOT  = Motorschutz
#            ND   = Niederdruckpressostat
#            PEX  = Fremdstromanode
#            SWT  = Schwimmbadthermostat
#  -   [54:55] TBW     = Temperatur Brauch-Warm-Wasser
#  -   [56:57] TA      = Aussentemperatur
#  -   [60:61] TRL     = Temperatur Heizung Ruecklauf
#  -   [62:63] TVL     = Temperatur Heizung Vorlauf
#  -   [64:65] THG     = Temperatur Heissgas
#  -   [66:67] TWQaus  = Temperatur Waermequelle Austritt
#  -   [70:71] TWQein  = Temperatur Waermequelle Eintritt
#  -   [80:81] TRLsoll = Solltemperatur Heizung Ruecklauf
#  - [158:159] VStrom  = Volumenstrom Heizkreis
#  
#  Umrechnung der Werte:
#  Fuer die oben genannten Temperaturen sind in den Datensaetzen natuerliche
#  Zahlen gespeichert. Diese muessen noch in die eigentlichen
#  Temperaturwerte umgerechnet werden. Dies wir hier mit folgender
#  Formel gemacht:
#     Temp = m * Wert + n
#  Die Koeffizienten m und n sind in der Variable dtaDsTempCoeffs zu finden.
#
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# DTA file format
#---------------------------------------------------------------------------
dtaHeaderLength        = 8    # bytes
dtaHeaderUnpackFormat  = "<II"

dtaDatasetLength       = 168  # bytes
dtaDatasetCount        = 2880 # 48h * 60min
dtaTimeInterval        = 60   # Sekunden
dtaDatasetUnpackFormat = "<iHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
dtaDatasetFieldCount   = 83

# Felder eines Datensatzes
dtaDsFieldNames = {
		# Position in Array: Name
		 				  0    : 'Datum',
						  8/2-1: 'StatusA',
						 44/2-1: 'StatusE',
						 54/2-1: 'TBW',
		  				 56/2-1: 'TA',
						 60/2-1: 'TRL',
		  				 62/2-1: 'TVL',
						 64/2-1: 'THG',
		  				 66/2-1: 'TWQaus',
						 70/2-1: 'TWQein',
		  				 80/2-1: 'TRLsoll',
						158/2-1: 'VStrom',
		}

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

# Status der Eingaenge
dtaDsStateInputs = {
		#   Pos: Name
            3: 'EVU' # EVU Sperre
		}

# Parameter zur Berechnung der Temperatur
dtaDsTempCoeffs = {
		#    Name: (        m,           n)
		    'TRL': (-0.097831,   75.878625),
		    'TVL': (-0.097831,   75.878625),
		    'TBW': (-0.112586,   81.871851),
		    'THG': (-0.117154,  114.742661),
		 'TWQein': (-0.081970,   41.812834),
		 'TWQaus': (-0.081970,   41.812834),
		'TRLsoll': ( 0.1,         0.0),
		 'VStrom': ( 7.129857, -708.482020),
		     'TA': (-0.078391,   40.814425),
}

#---------------------------------------------------------------------------
# MAIN
#---------------------------------------------------------------------------
if __name__ == "__main__":

	import pprint
	pprint.pprint(locals())

