/*
 * Part of Wal Commander GitHub Edition
 * https://github.com/corporateshark/WalCommander
 * walcommander@linderdaum.com
 */

#pragma once

#ifdef _WIN32
#  include "stdafx.h"
#endif

#include "swl.h"
#include <string.h>

using namespace wal;

extern clPtr<wal::GC> defaultGC;

extern const char* appName;
extern const char* appNameRoot;

enum NC_CLASSIDS
{
	CI_BUTTON_WIN = 100,
	CI_PANEL,
	CI_EDITOR,
	CI_VIEWER,
	CI_TERMINAL,
	CI_SHELLWIN
};

enum CommandIDs
{
	ID_HELP = 100,
	ID_USER_MENU,
	ID_VIEW,

	ID_EDIT,
	ID_EDIT_INP,
	ID_SHL_ONOFF,

	ID_COPY,
	ID_COPY_SHIFT,
	ID_MOVE,
	ID_MOVE_SHIFT,
	ID_MKDIR,
	ID_DELETE,
	ID_MENU,
	ID_QUIT,

	ID_WRAP,
	ID_HEX,

	ID_GROUP_SELECT,
	ID_GROUP_UNSELECT,
	ID_GROUP_INVERT,

	ID_SAVE,
	ID_SAVE_AS,
	ID_SEARCH_TEXT,
	ID_REPLACE_TEXT,
	ID_CHARSET,
	ID_CHARSET_TABLE,
	ID_GOTO_LINE,
	ID_UNDO,
	ID_REDO,

	ID_SORT_BY_NAME,
	ID_SORT_BY_EXT,
	ID_SORT_BY_MODIF,
	ID_SORT_BY_SIZE,
	ID_UNSORT,

	ID_SORT_BY_NAME_L,
	ID_SORT_BY_EXT_L,
	ID_SORT_BY_MODIF_L,
	ID_SORT_BY_SIZE_L,
	ID_UNSORT_L,

	ID_SORT_BY_NAME_R,
	ID_SORT_BY_EXT_R,
	ID_SORT_BY_MODIF_R,
	ID_SORT_BY_SIZE_R,
	ID_UNSORT_R,

	ID_PANEL_BRIEF_L,
	ID_PANEL_MEDIUM_L,
	ID_PANEL_FULL_L,
	ID_PANEL_FULL_ST_L,
	ID_PANEL_FULL_ACCESS_L,
	ID_PANEL_TWO_COLUMNS_L,

	ID_PANEL_BRIEF_R,
	ID_PANEL_MEDIUM_R,
	ID_PANEL_FULL_R,
	ID_PANEL_FULL_ST_R,
	ID_PANEL_FULL_ACCESS_R,
	ID_PANEL_TWO_COLUMNS_R,


	ID_CONFIG_SYSTEM,
	ID_CONFIG_PANEL,
	ID_CONFIG_EDITOR,
	ID_CONFIG_TERMINAL,
	ID_CONFIG_STYLE,
	ID_CONFIG_SAVE,

	ID_CTRL_O, //panel on/off
	ID_HISTORY,
	ID_PANEL_EQUAL,
	ID_SEARCH_2,
	ID_SHORTCUTS,
	ID_FILEASSOCIATIONS,
	ID_FILEHIGHLIGHTING,
	ID_REFRESH,

	ID_DEV_SELECT_LEFT,
	ID_DEV_SELECT_RIGHT,
	ID_DEV_ROOT,
	ID_DEV_HOME,
	ID_DEV_SMB,
	ID_DEV_SMB_SERVER,
	ID_DEV_FTP,
	ID_DEV_SFTP,
	ID_DEV_OTHER_PANEL,

	ID_MAX,
	ID_DEV_MS0 = 200,
	ID_MNT_UX0 = 300,
	ID_CHANGED_CONFIG_BROADCAST = 1000
};
