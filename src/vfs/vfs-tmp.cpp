#include "vfs-uri.h"
#include "vfs-tmp.h"
#include "urlparser/LUrlParser.h"

static const char rootStr[] = { '/' , 0};
static FSString rootFSStr(rootStr);

FSPath FSTmp::rootPathName(rootFSStr);
//beware, because of static scope the statement below to replace the above does not work:
//FSPath FSTmp::rootPathName(FSString({ DIR_SPLITTER, 0 }));


FSTmpNode::FSTmpNode(FSPath* _baseFSPath, FSStat* _baseFSNodeStat, FSTmpNode* _parentDir, const unicode_t* _name)
: FSTmpNode(NODE_FILE, _name, _parentDir)
{
	baseFSPath = *_baseFSPath;
	fsStat=*_baseFSNodeStat;
	//fsStat.link = _baseFSPath->GetUnicode();
	if (!_name)
		name = _baseFSPath->GetUnicode();
}

FSTmpNode::FSTmpNode(const unicode_t* _name, FSTmpNode* _parentDir)
: FSTmpNode(NODE_DIR, _name, _parentDir)
{
	fsStat.mode |= S_IFDIR;
#ifdef _WIN32	
	fsStat.m_CreationTime = FSTime(FSTime::TIME_CURRENT);
	fsStat.m_LastAccessTime = FSTime(FSTime::TIME_CURRENT);
	fsStat.m_LastWriteTime = FSTime(FSTime::TIME_CURRENT);
	fsStat.m_ChangeTime = FSTime(FSTime::TIME_CURRENT);
#else
	fsStat.m_CreationTime = time(0);
	fsStat.m_LastAccessTime = time(0);
	fsStat.m_LastWriteTime = time(0);
	fsStat.m_ChangeTime = time(0);
#endif
}

FSTmpNode* FSTmpNode::findByBasePath(FSPath* basePath, bool isRecursive)
{
	// first, try all file nodes in current dir
	for (FSTmpNode& n : content)
	{
		if (n.nodeType == FSTmpNode::NODE_FILE)
		{
			if (n.baseFSPath.Equals(basePath))
				return &n;
		} 
	}
	// only if not found in current dir, try inside subdirs
	if (isRecursive)
	{
		for (FSTmpNode& n : content )
		{
			if (n.nodeType == FSTmpNode::NODE_DIR)
			{
				FSTmpNode* pn = n.findByBasePath(basePath, isRecursive);
				if (pn)
					return pn;
			}
		}
	}
	return 0;
}

FSTmpNode* FSTmpNode::findByFsPath(FSPath* fsPath, int fsPathLevel)
{
	FSString* curName = fsPath->GetItem(fsPathLevel);
	if (parentDir == 0 && fsPathLevel == fsPath->Count()-1 && (curName == 0 || curName->GetUnicode() == 0 || curName->GetUnicode()[0] == 0)) // request to find root node, and we are root
		return this;

	if (name.Cmp(*curName) == 0)
	{
		if (fsPath->Count() <= fsPathLevel)
		{   // exact match
			return this;
		}
		else
		{
			FSString* childName = fsPath->GetItem(fsPathLevel + 1);
			FSTmpNode* n = findByName(childName, false);
			if (n == 0)
				return 0;
			if (fsPath->Count() <= fsPathLevel + 2) // no further recursion
				return n;
			else if (n->nodeType == NODE_DIR) // recurse into subdir
				return n->findByFsPath(fsPath, fsPathLevel + 1);
			else
				return 0;
		}
	}
	else
		return 0;
}


FSTmpNode* FSTmpNode::findByName(FSString* name, bool isRecursive)
{
	//dbg_printf("FSTmpNodeDir::findByName name=%s\n",name->GetUtf8());
	// first, try all nodes in current	
	for (FSTmpNode& n : content)
	{
		//dbg_printf("FSTmpNodeDir::findByName *it.name=%s\n", (*it).name.GetUtf8());

		if (n.name.Cmp(*name)==0)
			return &n;
	}
	// only if not found in current dir, try inside subdirs
	if (isRecursive)
	{
		for (FSTmpNode& n : content)
		{
			if (n.nodeType == FSTmpNode::NODE_DIR)
			{
				FSTmpNode* pn = n.findByName(name, isRecursive);
				if (pn)
					return pn;
			}
		}
	}
	return 0;
}

bool FSTmpNode::Remove(FSString* name, bool searchRecursive)
{
	// first, try all nodes in current dir
	for (auto it = content.begin(); it != content.end(); ++it)
	{
		if ((*it).name.Cmp(*name) == 0)
		{
			content.erase(it);
			return true;
		}
	}
	// only if not found in current dir, try inside subdirs
	if (searchRecursive)
	{
		for (FSTmpNode& n : content)
		{
			if (n.nodeType == FSTmpNode::NODE_DIR)
			{
				bool ret = n.Remove(name, searchRecursive);
				if (ret)
					return true;
			}
		}
	}
	return false;
}

bool FSTmpNode::Add(FSTmpNode* fsTmpNode)
{
	if (findByName(&fsTmpNode->name))
		return false;
	fsTmpNode->parentDir = this;
	content.push_back(*fsTmpNode);
	return true;
}

FSTmp::FSTmp(clPtr<FS> _baseFS)
	: FS(TMP)
{
	// to prevent build FSTmp on top of another FSTmp
	if (_baseFS->Type() == FS::TMP)
		baseFS = ((FSTmp*)_baseFS.Ptr())->baseFS;
	else
		baseFS = _baseFS;
}

int FSTmp::ReadDir(FSList* list, FSPath& path, int* err, FSCInfo* info)
{
	list->Clear();

	FSTmpNode* n = rootDir.findByFsPath(&path);
	if (!n || n->nodeType != FSTmpNode::NODE_DIR)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	}

	for (FSTmpNode& tn : n->content)
	{
		clPtr<FSNode> pNode = new FSNode();

		pNode->name=(std::string("") + tn.name.GetUtf8()).c_str();
		if (tn.nodeType == FSTmpNode::NODE_DIR)
			pNode->st = tn.fsStat;
		else
		{
			if (baseFS->Stat(tn.baseFSPath, & pNode->st, 0, 0) != 0)
				pNode->st = tn.fsStat;
		}
		list->Append(pNode);
	}
	return 0;
}

int FSTmp::StatSetAttr( FSPath& path, const FSStat* st, int* err, FSCInfo* info )
{
	FSTmpNode* n = rootDir.findByFsPath(&path);
	if (!n)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	}
	return FS::SetError(err, 0);
}

int FSTmp::Stat(FSPath& path, FSStat* st, int* err, FSCInfo* info)
{
	//return baseFS ? baseFS->Stat( path, st, err, info ) : FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);

	//path.dbg_printf("FSTmp::Stat ");
	FSTmpNode* n = rootDir.findByFsPath(&path);
	if (!n)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	}
	*st = n->fsStat;
	return FS::SetError(err, 0);
}

int FSTmp::StatVfs(FSPath& path, FSStatVfs* st, int* err, FSCInfo* info)
{	
	//path.dbg_printf("FSTmp::StatVfs ");
	if (rootPathName.Equals(&path))
	{
		*st = FSStatVfs();
		return 0;
	}
	else
		return baseFS->StatVfs(path, st, err, info);
}

int FSTmp::OpenRead(FSPath& path, int flags, int* err, FSCInfo* info)
{
	//path.dbg_printf("FSTmp::OpenRead ");
	FSTmpNode* n = rootDir.findByFsPath(&path);
	if (n == 0 || n->nodeType != FSTmpNode::NODE_FILE)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	}
	else
	{
		//n->baseFSPath.dbg_printf("FSTmp::OpenRead baseFSPath=");
		return baseFS->OpenRead(n->baseFSPath, flags, err, info);
	}
}
	

int FSTmp::OpenCreate(FSPath& path, bool overwrite, int mode, int flags, int* err, FSCInfo* info)
{
	FSTmpNode* n = rootDir.findByFsPath(&path);
	if (!n) // can not create orphan file in TMP panel
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	else if (!overwrite) 
		return FS::SetError(err, FSTMP_ERROR_FILE_EXISTS);
	else
	{
		int ret = baseFS->OpenCreate(n->baseFSPath, overwrite, mode, flags, err, info);
		if ( ret < 0 )
			Delete(path, err, info);
		return ret;
	}
}

int FSTmp::Rename(FSPath&  oldpath, FSPath& newpath, int* err, FSCInfo* info)
{
	FSTmpNode* n = rootDir.findByName(oldpath.GetItem(oldpath.Count() - 1));
	if (n == 0)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	}
	else
	{
		if (n->nodeType == FSTmpNode::NODE_FILE)
		{
			int ret = baseFS->Rename(n->baseFSPath, newpath, err, info);
			if (ret != 0)
				return ret;
			n->name = newpath.GetUnicode();
			return SetError(err, 0);
		}
		else
		{// XXX ??? add case when new and old path are in different dirs
			((FSTmpNode*)(n))->name = *newpath.GetItem(newpath.Count() - 1);
			return SetError(err, 0);
		}
	}
}

int FSTmp::Delete(FSPath& path, int* err, FSCInfo* info)
{
	FSString* dName = path.GetItem(path.Count() - 1);
	FSTmpNode* n = rootDir.findByName(dName);
	if (n == 0)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);;
	}
	else
	{
		/*
		if (n->nodeType == FSTmpNode::NODE_FILE)
		{ // remove file at base FS
			int ret = baseFS->Delete(n->baseFSPath, err, info);
			if (ret != 0)
				return ret;
		}
		*/
		// remove from tmpfs list
		for (auto it = n->parentDir->content.begin(); it != n->parentDir->content.end(); ++it)
		{ 
			if ((*it).name.Cmp(*dName) == 0)
			{
				n->parentDir->content.erase(it);
				return FS::SetError(err, 0);
			}
		}
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);;
	}
}

int FSTmp::RmDir(FSPath& path, int* err, FSCInfo* info)
{
	FSTmpNode* n = rootDir.findByFsPath(&path);
	FSString* dirName = path.GetItem(path.Count() - 1);

	for (auto it = n->parentDir->content.begin(); it != n->parentDir->content.end(); ++it)
	{
		if ((*it).name.Cmp(*dirName) == 0)
		{
			n->parentDir->content.erase(it);
			return FS::SetError(err, 0);
		}
	}
	return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
}

int FSTmp::SetFileTime(FSPath& path, FSTime cTime, FSTime aTime, FSTime mTime, int* err, FSCInfo* info)
{
	FSTmpNode* fsTemp = rootDir.findByName(path.GetItem(path.Count() - 1));
	if (fsTemp == 0)
	{
		return FS::SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	}
	else
	{
		fsTemp->fsStat.m_CreationTime = cTime;
		fsTemp->fsStat.m_LastAccessTime = aTime;
		fsTemp->fsStat.m_LastWriteTime = mTime;
		fsTemp->fsStat.m_ChangeTime = mTime;

		if (fsTemp->nodeType == FSTmpNode::NODE_FILE)
			return baseFS->SetFileTime(fsTemp->baseFSPath, cTime, aTime, mTime, err, info);
		else
			return FS::SetError(err, 0);
			
	}
}

// The Uri/ParseURI funcion is used in two cases:
// 1. Panel caption, to show folder/file - only Uri side of the pair
// 2. Dialogs, registry - to combine FS and path into URI, and parse the URI back - both functions.
// Approaches for Url-ParseURI implementations
// 1. Delegate Uri to baseFS. No need to have ParseURI
//    - copying to TMPFs does not work. 

// 2. Delegate file Uri to baseBS. Folder Uri is tmp:///folder/name#baseFSRootUri
//   - appears to work 
FSString FSTmp::Uri(FSPath& path)
{
	FSTmpNode* n = rootDir.findByFsPath(&path);
	if (n && n->nodeType == FSTmpNode::NODE_FILE)
		return baseFS->Uri(n->baseFSPath);
	else
	{
		std::string ret(std::string("tmp://") + path.GetUtf8('/') + "#" + baseFS->Uri(rootPathName).GetUtf8());
		return FSString(ret.c_str());
	}
}

// Handles only folder URIs.
// File URIs are not tmp://-based, but are baseFS-based
// The URI is: tmp:///folder/name#baseFSRoot
clPtr<FS> FSTmp::ParzeURI(const unicode_t* uri, FSPath& path, const std::vector<clPtr<FS>>& checkFS)
{
	std::string	uriUtf8(unicode_to_utf8_string(uri));
	
	LUrlParser::clParseURL clparseURL = LUrlParser::clParseURL::ParseURL(uriUtf8);
	
	if (clparseURL.IsValid() && clparseURL.m_Scheme == "tmp")
	{
		FSString pathFSstring(clparseURL.m_Path);
		path = FSPath(pathFSstring);

		for (clPtr<FS> clPtrFs : checkFS)
		{
			if (clPtrFs->Type() == FS::TMP)
			{
				FSTmp* fsTmp = (FSTmp*) clPtrFs.ptr();
				if (clparseURL.m_Fragment == fsTmp->baseFS->Uri(rootPathName).GetUtf8())
					return clPtrFs;
			}
		}
		// no tmpFS ptr found among checkFS, create baseFS, and FSTmp on top of it.
		FSPath tFSPath;
		FSString fragmentFSString(clparseURL.m_Fragment);
		clPtr<FS> baseFS = ::ParzeURI(fragmentFSString.GetUnicode(), tFSPath, checkFS);
		return new FSTmp(baseFS);
	}
	return new FSSys();
}

bool FSTmp::AddNode(FSPath& srcPath, FSNode* fsNode, FSPath& destPath)
{
	FSPath parentDir = destPath;
	parentDir.Pop();
	//dbg_printf("FSTmp::AddNode srcPath.Count()=%d, parentDir.Count()=%d\n", srcPath.Count(), parentDir.Count());
	//srcPath.dbg_printf("FSTmp::AddNode srcPath=");
	//destPath.dbg_printf("FSTmp::AddNode destPath=");

	FSTmpNode* dn = rootDir.findByFsPath(&parentDir);
	if (!dn || dn->nodeType != FSTmpNode::NODE_DIR)
	{
		return false;
	}
	FSTmpNode fsTmpNode(&srcPath, &fsNode->st, dn);
	return dn->Add(&fsTmpNode);
}

bool FSTmp::AddNode(FSPath& srcPath, FSPath& destDir)
{
	FSTmpNode* dn = rootDir.findByFsPath(&destDir);
	if (!dn || dn->nodeType != FSTmpNode::NODE_DIR)
	{
		return false;
	}
	FSStat st;
	if (baseFS->Stat(srcPath, &st, 0, 0)!=0)
		return false;
	if (!st.IsReg())
		return false;
	FSTmpNode fsTmpNode(&srcPath, &st, dn);
	return dn->Add(&fsTmpNode);
}

int FSTmp::MkDir(FSPath& path, int mode, int* err, FSCInfo* info)
{
	FSPath parentPath;
	parentPath.Copy(path, path.Count() - 1);
	FSTmpNode* parent = rootDir.findByFsPath(&parentPath);
	if (!parent)
		return SetError(err, FSTMP_ERROR_FILE_NOT_FOUND);
	FSTmpNode* parentDir = parent;
	FSTmpNode fsTmpNode(path.GetItem(path.Count() - 1)->GetUnicode(), parentDir);
	parentDir->Add(&fsTmpNode);
	return SetError(err, 0);
}
