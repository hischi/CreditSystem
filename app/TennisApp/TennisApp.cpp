// TennisApp.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#define _USE_32BIT_TIME_T

#include "pch.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include "../../src/data_handler/business_model.h"
#include "csv.h"

#define APP_VERSION "0.1"


#define MITGLIEDSNUMMER "MitgliedsNr"
#define NACHNAME "Nachname"
#define VORNAME "Vorname"
#define EIGENSCHAFTEN "Eigenschaften"
#define RABATT "Rabatt"
#define KARTE "Karte"
#define VERSION "Version"
#define DATE "Datum"
#define AUTHOR "Bearbeiter"

enum eTasks;

void print_header();
void print_help();
bool check_args(int argc, char *argv[], eTasks *task);
int pack(char csv_file[], char db_file[]);
int unpack(char db_file[], char csv_file[]);

enum eTasks {
	T_Pack,
	T_Unpack,
	T_Unknown
};

int main(int argc, char *argv[])
{
	eTasks task;

	print_header();
	if (!check_args(argc, argv, &task)) {
		std::cout << "Invalid arguments." << std::endl;
		print_help();
		goto display_error;
	}

	switch (task) {
	case T_Pack: 
		if (pack(argv[2], argv[3]) != 0)
			goto display_error;
		else
			return 0;
	case T_Unpack: 
		if(unpack(argv[2], argv[3]) != 0)
			goto display_error;
		else
			return 0;
	default:
		goto display_error;
	}

	return 0;

display_error:
	std::cout << "Press ENTER to exit." << std::endl;
	std::cin.get();
	return 1;
}

int pack(char csv_file[], char db_file[]) {

	// Check if both files exist otherwise return error (1)
	FILE *csv_fp, *db_fp;
	if (csv_fp = fopen(csv_file, "r")) {
		fclose(csv_fp);
	}
	else {
		std::cout << "Invalid CSV file. File can't be opended" << std::endl;
		return 1;
	}

	if ((db_fp = fopen(db_file, "wb")) == NULL) {
		std::cout << "Invalid DB file. File can't be opended" << std::endl;
		return 1;
	}

	// Reopen CSV file with CSV reader
	io::CSVReader<6, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> reader(csv_file);

	reader.read_header(io::ignore_missing_column, VERSION, DATE, AUTHOR, "Format", "Format Version", "EOL");
	if (!reader.has_column(VERSION) || !reader.has_column(DATE) || !reader.has_column(AUTHOR))
	{
		std::cout << "CSV has wrong structure. Expected Header: " << VERSION << "; " << DATE << "; " << AUTHOR << std::endl;
		return 1;
	}

	// Read&Write Database header
	sDataBaseHeader db_header;
	char *author, *date;
	char *format;
	char *formatversion;
	char *eol;
	try {
		if (!reader.read_row(db_header.version, date, author, format, formatversion, eol)) {
			std::cout << "Can't read dateabase header from CSV" << std::endl;
			return 1;
		}
	}
	catch (const io::error::base &ex) {
		std::cout << "Can't read header. Reason:" << std::endl;
		ex.format_error_message();
		std::cout << ex.error_message_buffer << std::endl;
		return 1;
	}

	strncpy(db_header.author, author, 16);
	std::istringstream is(date);
	int d, m, y;
	char delimiter;
	if (is >> d >> delimiter >> m >> delimiter >> y) {
		struct tm t = { 0 };
		t.tm_mday = d;
		t.tm_mon = m - 1;
		t.tm_year = y - 1900;
		t.tm_hour = 12;

		// normalize:
		time_t when = mktime(&t);
		db_header.datetime_modified = when;
	}


	fwrite(&db_header, sizeof(db_header), 1, db_fp);

	reader.next_line();

	// Read data
	reader.read_header(io::ignore_missing_column, MITGLIEDSNUMMER, NACHNAME, VORNAME, EIGENSCHAFTEN, RABATT, KARTE);
	if (!reader.has_column(MITGLIEDSNUMMER) || !reader.has_column(NACHNAME) || !reader.has_column(VORNAME) || !reader.has_column(EIGENSCHAFTEN) || !reader.has_column(RABATT) || !reader.has_column(KARTE))
	{
		std::cout << "CSV has wrong structure. Expected: " << MITGLIEDSNUMMER << "; " << NACHNAME << "; " << VORNAME << "; " << EIGENSCHAFTEN << "; " << RABATT << "; " << KARTE << std::endl;
		return 1;
	}

	// Read CSV and fill DB file
	sMember member;
	char *name;
	char *given_name;
	db_header.entry_count = 0;
	try {
		while (reader.read_row(member.id, name, given_name, member.properties, member.discount, member.card_id)) {
			strncpy(member.name, name, 16);
			strncpy(member.given_name, given_name, 16);
			fwrite(&member, sizeof(member), 1, db_fp);
			db_header.entry_count++;
		}
	}
	catch (const io::error::base &ex ) {
		std::cout << "Can't read entry " << db_header.entry_count << " (counting starting from 0). Reason:" << std::endl;
		ex.format_error_message();
		std::cout << ex.error_message_buffer << std::endl;
		return 1;
	}

	// Update entry count in header and write it again
	fseek(db_fp, 0, SEEK_SET); 
	fwrite(&db_header, sizeof(db_header), 1, db_fp);

	fclose(db_fp);

	return 0;
}

int unpack(char db_file[], char csv_file[]) {

	int nrc = 0;

	// Check if both files exist otherwise return error (1)
	FILE *csv_fp, *db_fp;
	if (!(csv_fp = fopen(csv_file, "w"))) {
		std::cout << "Invalid CSV file. File can't be opended" << std::endl;
		return 1;
	}

	if (!(db_fp = fopen(db_file, "rb"))) {
		std::cout << "Invalid DB file. File can't be opended" << std::endl;
		fclose(csv_fp);
		return 1;
	}

	// Interprete header
	sTransactionHeaderCS header_cs;
	//std::cout << "Current file pos: " << ftell(db_fp) << "and " << sizeof(sTransactionHeaderCS) << std::endl;
	int test2 = fread(&header_cs, sizeof(sTransactionHeaderCS), 1L, db_fp);
	if (test2 != 1) {
		std::cout << "DB file too small to read header" << std::endl;
		fclose(csv_fp);
		fclose(db_fp);
		return 1;
	}
	fpos_t position = 16L;
	//fsetpos(db_fp, &position);
	//std::cout << "Current file pos: " << ftell(db_fp) << std::endl;

	char time_str[80];
	char status_str[8];

	// Check header signature
	uint32_t cs = calculate_checksum((uint16_t*)&header_cs.header, sizeof(header_cs.header));
	if (cs == header_cs.checksum) {
		sprintf(status_str, "ja");
	}
	else {
		std::cout << "DB header invalid checksum" << std::endl;
		std::cout << "Please check output!" << std::endl;
		sprintf(status_str, "nein");
		nrc = 1;
	}

	// Read date
	{
		time_t unix_time = header_cs.header.datetime_modified;
		tm *time_struct = gmtime(&unix_time);
		if (time_struct == 0) {
			std::cout << "Header has invalid timestamp" << std::endl;
			std::cout << "Please check output!" << std::endl;
			sprintf(time_str, "ungueltig (%u)", unix_time);
			nrc = 1;
		} else {
			strftime(time_str, sizeof(time_str), "%d.%m.%Y %H:%M:%S", time_struct);
		}
		
	}
	

	// Write header
	fprintf(csv_fp, "Tennis Transaktionen\n");
	fprintf(csv_fp, "Version;Datum;Anzahl Eintraege; Signatur; Signatur korrekt\n");
	fprintf(csv_fp, "%d; %s; %d; %u; %s\n\n", header_cs.header.version, time_str, header_cs.header.entry_count, header_cs.checksum, status_str);

	// Write transaction header
	fprintf(csv_fp, "ID; Datum; Mitglied-Nr; Status; Schacht; Preis; Einbezogener Rabatt; Signatur; Signatur korrekt\n");

	sTransactionCS transaction_cs;

	for (int i = 0; i < header_cs.header.entry_count; i++) {
		//std::cout << "Current file pos: " << ftell(db_fp) << std::endl;
		int test = fread(&transaction_cs, sizeof(transaction_cs), 1L, db_fp);

		// Read next transaction
		if (ferror(db_fp) || feof(db_fp)) {
			
			std::cout << "DB file contains an incomplete transaction" << std::endl;
			std::cout << "Please check output!" << std::endl;
			nrc = 1;
			break;
		}

		// Check signature
		cs = calculate_checksum((uint16_t*)&transaction_cs.transaction, sizeof(transaction_cs.transaction));
		if (cs == transaction_cs.checksum) {
			sprintf(status_str, "ja");
		}
		else {
			std::cout << "Transaction " << transaction_cs.transaction.id << " has invalid checksum" << std::endl;
			std::cout << "Please check output!" << std::endl;
			sprintf(status_str, "nein");
			nrc = 1;
		}

		// Read date
		time_t unix_time = transaction_cs.transaction.datetime_modified;
		tm *time_struct = gmtime(&unix_time);
		if (time_struct == 0) {
			std::cout << "Transaction " << transaction_cs.transaction.id << " has invalid timestamp" << std::endl;
			std::cout << "Please check output!" << std::endl;
			sprintf(time_str, "ungueltig (%u)", unix_time);
			nrc = 1;
		}
		else {
			strftime(time_str, sizeof(time_str), "%d.%m.%Y %H:%M:%S", time_struct);
		}

		// Write transaction		
		fprintf(csv_fp, "%d; %s; %d; %hhu; %hhu; %d; %hu; %u; %s\n", transaction_cs.transaction.id, time_str, transaction_cs.transaction.member_id, transaction_cs.transaction.status, transaction_cs.transaction.item_id, transaction_cs.transaction.cost, transaction_cs.transaction.discount, transaction_cs.checksum, status_str);
	}

	fclose(csv_fp);
	fclose(db_fp);
	return nrc;
}

bool check_file_extension(const char *str, const char *ext) {
	int len = strlen(str);
	int ext_len = strlen(ext);

	return (len > ext_len && _stricmp(str + (len - ext_len), ext) == 0);
}

bool check_args(int argc, char *argv[], eTasks *task) {
	if (argc != 4)
		return false;

	if (_stricmp(argv[1], "PACK") == 0) {
		*task = T_Pack;

		return check_file_extension(argv[2], ".CSV") && check_file_extension(argv[3], ".DB");
	} 
	else if (_stricmp(argv[1], "UNPACK") == 0) {
		*task = T_Unpack;

		return check_file_extension(argv[2], ".DB") && check_file_extension(argv[3], ".CSV");
	}
	else {
		*task = T_Unknown;
	}
	return false;
}

void print_header() {
	std::cout << "Tennis App" << std::endl;
	std::cout << "Written by Florian Hisch" << std::endl;
	std::cout << std::endl;
	std::cout << "Version:          " << APP_VERSION << std::endl;
	std::cout << "Database Version: " << DB_VERSION << std::endl;
	std::cout << "---------------------------------------" << std::endl;
}

void print_help() {
	
	std::cout << std::endl;
	std::cout << "Usage: TennisApp.exe TASK INPUT OUTPUT" << std::endl;
	std::cout << "TASK:   Task which should be performed:" << std::endl;
	std::cout << "        PACK: Packs the given csv data-base (memberlist) into the given outputfile (.db)." << std::endl;
	std::cout << "        UNPACK: Unpacks the given transaction list (.db) into a csv outputfile." << std::endl;
	std::cout << std::endl;
	std::cout << "INPUT:  The input file with the correct file-extension according to the TASK." << std::endl;
	std::cout << std::endl;
	std::cout << "OUTPUT: The output file with the correct file-extension according to the TASK. If it does not exist, a file with this name is generated." << std::endl;
}