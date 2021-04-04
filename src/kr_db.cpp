#include "kr_db.h"
#include "cb_interface.h"


void log_msg(char *msg)
{
}

/*
 * Open an endgame database it has not been done yet.
 * If a db setting changed, close any open db and the re-open with the new settings.
 */
void init_egdb(char msg[1024])
{
	if (!checkerBoard.did_egdb_init || checkerBoard.request_egdb_init) {
		checkerBoard.request_egdb_init = false;
		checkerBoard.did_egdb_init = true;
		if (engine.dbInfo.loaded) {
			if (engine.dbInfo.type == dbType::KR_WIN_LOSS_DRAW) {
				if (engine.dbInfo.kr_wld) {
					engine.dbInfo.kr_wld->close(engine.dbInfo.kr_wld);
					engine.dbInfo.kr_wld = nullptr;
				}
			}
			if (engine.dbInfo.type == dbType::EXACT_VALUES)
				close_trice_egdb(engine.dbInfo);
			if (engine.dbInfo.type == dbType::WIN_LOSS_DRAW)
				close_gui_databases(engine.dbInfo);
			engine.dbInfo.loaded = false;
		}
		if (checkerBoard.enable_wld) {
			if (strlen(checkerBoard.db_path) == 0) {
				/* This means he wants to load the 4-piece gui checkers db. */
				InitializeGuiDatabases(engine.dbInfo);
			}
			else {
				int egdb_found, dbpieces;
				EGDB_TYPE wld_type;

				egdb_found = !egdb_identify(checkerBoard.db_path, &wld_type, &dbpieces);
				dbpieces = std::min(checkerBoard.max_dbpieces, dbpieces);
				if (egdb_found) {
					switch (wld_type) {
					case EGDB_KINGSROW32_WLD:
					case EGDB_KINGSROW32_WLD_TUN:
						sprintf(msg, "Wait for db init; Kingsrow db, %d pieces, %d mb cache, %d mb hashtable ...",
							dbpieces, checkerBoard.wld_cache_mb, engine.TTable.sizeMb);
						engine.dbInfo.kr_wld = egdb_open(EGDB_ROW_REVERSED, dbpieces, checkerBoard.wld_cache_mb, checkerBoard.db_path, log_msg);
						if (engine.dbInfo.kr_wld) {
							engine.dbInfo.type = dbType::KR_WIN_LOSS_DRAW;
							engine.dbInfo.numPieces = dbpieces;
							engine.dbInfo.loaded = true;
							engine.dbInfo.numBlack = 5;
							engine.dbInfo.numWhite = 5;
						}
						break;

					case EGDB_CAKE_WLD:
						sprintf(msg, "Wait for db init; Cake db, %d pieces, %d mb cache, %d mb hashtable ...",
							dbpieces, checkerBoard.wld_cache_mb, engine.TTable.sizeMb);
						engine.dbInfo.kr_wld = egdb_open(EGDB_ROW_REVERSED, dbpieces, checkerBoard.wld_cache_mb, checkerBoard.db_path, log_msg);
						if (engine.dbInfo.kr_wld) {
							engine.dbInfo.type = dbType::KR_WIN_LOSS_DRAW;
							engine.dbInfo.numPieces = dbpieces;
							engine.dbInfo.loaded = true;
							engine.dbInfo.numBlack = 4;
							engine.dbInfo.numWhite = 4;
						}
						break;

					case EGDB_CHINOOK_WLD:
						sprintf(msg, "Wait for db init; Chinook db, %d pieces, %d mb cache, %d mb hashtable ...",
							dbpieces, checkerBoard.wld_cache_mb, engine.TTable.sizeMb);
						engine.dbInfo.kr_wld = egdb_open(EGDB_ROW_REVERSED, dbpieces, checkerBoard.wld_cache_mb, checkerBoard.db_path, log_msg);
						if (engine.dbInfo.kr_wld) {
							engine.dbInfo.type = dbType::KR_WIN_LOSS_DRAW;
							engine.dbInfo.numPieces = dbpieces;
							engine.dbInfo.loaded = true;
							engine.dbInfo.numBlack = 4;
							engine.dbInfo.numWhite = 4;
						}
						break;

					default:
						dbpieces = 0;
						return;
					}

					if (!engine.dbInfo.kr_wld) {
						sprintf(msg, "Cannot open endgame db driver.");
						Sleep(5000);
					}
				}
				else {
					if (!engine.dbInfo.loaded) {
						sprintf(msg, "Initializing Trice db...");
						InitializeEdsDatabases(engine.dbInfo);
					}
					if (!engine.dbInfo.loaded) {
						sprintf(msg, "Cannot find endgame database files.");
						Sleep(5000);
					}
				}
			}
		}
	}
}


int is_wld_db(EGDB_TYPE dbtype)
{
	switch (dbtype) {
	case EGDB_KINGSROW32_WLD:
	case EGDB_KINGSROW32_WLD_TUN:
	case EGDB_CAKE_WLD:
	case EGDB_CHINOOK_WLD:
		return(1);
	}
	return(0);
}


char *dbname(EGDB_TYPE egdb_type)
{
	switch (egdb_type) {
	case EGDB_KINGSROW32_WLD:
	case EGDB_KINGSROW32_WLD_TUN:
	case EGDB_KINGSROW32_MTC:
	case EGDB_KINGSROW_DTW:
		return("Kingsrow");

	case EGDB_CAKE_WLD:
		return("Cake");

	case EGDB_CHINOOK_WLD:
		return("Chinook");
	}
	return("unknown");
}


void check_wld_dir(const char *dir, char *reply)
{
	int status, pieces;
	EGDB_TYPE dbtype;
	FILE *fp;

	if (strlen(dir) == 0) {
		fp = fopen("engines\\database.jef", "rb");
		if (fp) {
			fclose(fp);
			sprintf(reply, "GUI checkers database found for 4 pieces");
		}
		else
			sprintf(reply, "No GUI checkers database found");
		return;
	}
	status = egdb_identify(dir, &dbtype, &pieces);
	if (status || !is_wld_db(dbtype)) {
		char filename[MAX_PATH];

		int len;
		char *sep;

		/* Check for the Trice db. */
		len = (int)strlen(dir);
		if (len && (dir[len - 1] != '\\'))
			sep = "\\";
		else
			sep = "";

		sprintf(filename, "%s%sdb_06_(0K3C_0K3C)", dir, sep);
		fp = fopen(filename, "rb");
		if (fp) {
			fclose(fp);
			sprintf(reply, "Trice DTW database found for 6 pieces");
		}
		else
			sprintf(reply, "No WLD database found in %s", dir);
	}
	else
		sprintf(reply, "%s WLD database found for %d pieces", dbname(dbtype), pieces);
}


