/*
 * Part of WCM Commander
 * https://github.com/corporateshark/WCMCommander
 * wcm@linderdaum.com
 */

#pragma once

#include "fileopers.h"
#include "ncdialogs.h"
#include "operwin.h"

bool DirCalc( clPtr<FS> f, FSPath& path, clPtr<FSList> list, NCDialogParent* parent,  int64_t& curFileCount, int64_t& curSumSize, bool serviceMode);
