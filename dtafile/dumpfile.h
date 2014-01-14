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
* - Klasse zum Lesen und Decodieren von Dump-Dateien
*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
* Aufbau einer DUMP-Datei
* =======================
*  - Textdatei + BZ2 Kompression
*  - eine Zeile enthaelt einen Datensatz
*  - Format einer Zeile: Index1=Wert1,Index2=Wert2,...
*  - nur in der ersten Zeile einer Datei sind alle Indexes aufgefuehrt
*  - in den weiteren Zeilen werden nur die Indexes gespeichert, deren Werte
*      sich geaendert haben
*  - die Datensaetze entspechen den Daten, wie sie von der Luxtronik 2 ueber
*      die Netzwerkschnittstelle abgerufen werden koennen
*     # zu <IP-Adresse> und Port 8888 der Luxtronik 2 verbinden
*     # 3004 senden (big endian, 4-bytes)
*     # 0 senden
*     # 3004 lesen
*     # Status lesen
*     # Laenge des folgenden Datensatzes lesen
*     # Felder des Datensatzes lesen
*
* Felder eines Datensatzes (Softwarestand Luxtronik 2 ist v1.52)
* ==============================================================
*  10: Temperatur_TVL
*  11: Temperatur_TRL
*  12: Sollwert_TRL_HZ
*  13: Temperatur_TRL_ext
*  14: Temperatur_THG
*  15: Temperatur_TA
*  16: Mitteltemperatur
*  17: Temperatur_TBW
*  18: Einst_BWS_akt
*  19: Temperatur_TWE
*  20: Temperatur_TWA
*  21: Temperatur_TFB1
*  22: Sollwert_TVL_MK1
*  23: Temperatur_RFV
*  24: Temperatur_TFB2
*  25: Sollwert_TVL_MK2
*  26: Temperatur_TSK
*  27: Temperatur_TSS
*  28: Temperatur_TEE
*  29: ASDin
*  30: BWTin
*  31: EVUin
*  32: HDin
*  33: MOTin
*  34: NDin
*  35: PEXin
*  36: SWTin
*  37: AVout
*  38: BUPout
*  39: HUPout
*  40: MA1out
*  41: MZ1out
*  42: VENout
*  43: VBOout
*  44: VD1out
*  45: VD2out
*  46: ZIPout
*  47: ZUPout
*  48: ZW1out
*  49: ZW2SSTout
*  50: ZW3SSTout
*  51: FP2out
*  52: SLPout
*  53: SUPout
*  54: MZ2out
*  55: MA2out
*  56: Zaehler_BetrZeitVD1
*  57: Zaehler_BetrZeitImpVD1
*  58: Zaehler_BetrZeitVD2
*  59: Zaehler_BetrZeitImpVD2
*  60: Zaehler_BetrZeitZWE1
*  61: Zaehler_BetrZeitZWE2
*  62: Zaehler_BetrZeitZWE3
*  63: Zaehler_BetrZeitWP
*  64: Zaehler_BetrZeitHz
*  65: Zaehler_BetrZeitBW
*  66: Zaehler_BetrZeitKue
*  67: Time_WPein_akt
*  68: Time_ZWE1_akt
*  69: Time_ZWE2_akt
*  70: Timer_EinschVerz
*  71: Time_SSPAUS_akt
*  72: Time_SSPEIN_akt
*  73: Time_VDStd_akt
*  74: Time_HRM_akt
*  75: Time_HRW_akt
*  76: Time_LGS_akt
*  77: Time_SBW_akt
*  78: Code_WP_akt
*  79: BIV_Stufe_akt
*  80: WP_BZ_akt
*  81: SoftStand1
*  82: SoftStand2
*  83: SoftStand3
*  84: SoftStand4
*  85: SoftStand5
*  86: SoftStand6
*  87: SoftStand7
*  88: SoftStand8
*  89: SoftStand9
*  90: SoftStand10
*  91: AdresseIP_akt
*  92: SubNetMask_akt
*  93: Add_Broadcast
*  94: Add_StdGateway
*  95: ERROR_Time0
*  96: ERROR_Time1
*  97: ERROR_Time2
*  98: ERROR_Time3
*  99: ERROR_Time4
* 100: ERROR_Nr0
* 101: ERROR_Nr1
* 102: ERROR_Nr2
* 103: ERROR_Nr3
* 104: ERROR_Nr4
* 105: AnzahlFehlerInSpeicher
* 106: Switchoff_file_Nr0
* 107: Switchoff_file_Nr1
* 108: Switchoff_file_Nr2
* 109: Switchoff_file_Nr3
* 110: Switchoff_file_Nr4
* 111: Switchoff_file_Time0
* 112: Switchoff_file_Time1
* 113: Switchoff_file_Time2
* 114: Switchoff_file_Time3
* 115: Switchoff_file_Time4
* 116: Comfort_exists
* 117: HauptMenuStatus_Zeile1
* 118: HauptMenuStatus_Zeile2
* 119: HauptMenuStatus_Zeile3
* 120: HauptMenuStatus_Zeit
* 121: HauptMenuAHP_Stufe
* 122: HauptMenuAHP_Temp
* 123: HauptMenuAHP_Zeit
* 124: SH_BWW
* 125: SH_HZ
* 126: SH_MK1
* 127: SH_MK2
* 128: Einst_Kurzrpgramm
* 129: StatusSlave_1
* 130: StatusSlave_2
* 131: StatusSlave_3
* 132: StatusSlave_4
* 133: StatusSlave_5
* 134: AktuelleTimeStamp
* 135: SH_MK3
* 136: Sollwert_TVL_MK3
* 137: Temperatur_TFB3
* 138: MZ3out
* 139: MA3out
* 140: FP3out
* 141: Time_AbtIn
* 142: Temperatur_RFV2
* 143: Temperatur_RFV3
* 144: SH_SW
* 145: Zaehler_BetrZeitSW
* 146: FreigabKuehl
* 147: AnalogIn
* 148: SonderZeichen
* 149: SH_ZIP
* 150: WebsrvProgrammWerteBeobarten
* 151: WMZ_Heizung
* 152: WMZ_Brauchwasser
* 153: WMZ_Schwimmbad
* 154: WMZ_Seit
* 155: WMZ_Durchfluss
* 156: AnalogOut1
* 157: AnalogOut2
* 158: Time_Heissgas
* 159: Temp_Lueftung_Zuluft
* 160: Temp_Lueftung_Abluft
* 161: Zaehler_BetrZeitSolar
* 162: AnalogOut3
* 163: AnalogOut4
* 164: Out_VZU
* 165: Out_VAB
* 166: Out_VSK
* 167: Out_FRH
* 168: AnalogIn2
* 169: AnalogIn3
* 170: SAXin
* 171: SPLin
* 172: Compact_exists
* elt1: elektrical energy compressor
* elt2: elektrical energy Luxtronik2, pumps
*---------------------------------------------------------------------------*/
#ifndef DUMPFILE_H
#define DUMPFILE_H

#include <QtCore>

#include "dtafile/datafile.h"

/*---------------------------------------------------------------------------
* DumpFile
*---------------------------------------------------------------------------*/
class DumpFile : public DataFile
{
   Q_OBJECT
public:
   explicit DumpFile(QString fileName, QObject *parent=0);
   ~DumpFile();

   virtual bool open(); // DUMP-Datei oeffnen
   virtual void readDatasets(DataMap *data); // alle Datensaete lesen und in Map speichern
   virtual QString version() {return QString("DUMP");} // Datei-Version

private:
   QProcess bzcat;
};

#endif // DUMPFILE_H
