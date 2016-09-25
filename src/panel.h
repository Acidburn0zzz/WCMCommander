/*
 * Part of WCM Commander
 * https://github.com/corporateshark/WCMCommander
 * wcm@linderdaum.com
 */

#pragma once

#include "swl.h"
#include "vfs.h"
#include "vfs-uri.h"
#include "operwin.h"
#include "fileopers.h"
#include "panel_list.h"

#define FC(key, mods) (((key)&0xFFFF) + ((mods)<<16))

using namespace wal;

extern int uiClassPanel;

class NCWin;
class PanelWin;

class PanelSearchWin: public Win
{
	PanelWin* _parent;
	EditLine _edit;
	StaticLine _static;
	Layout _lo;
	std::vector<unicode_t> oldMask;
public:
	cevent_key ret_key;
	PanelSearchWin( PanelWin* parent, cevent_key* key );
	virtual void Repaint();
	virtual void Paint( wal::GC& gc, const crect& paintRect );
	virtual bool EventShow( bool show );
	virtual bool EventKey( cevent_key* pEvent );
	virtual bool EventChildKey( Win* child, cevent_key* pEvent );
	virtual cfont* GetChildFont( Win* w, int fontId );
	virtual bool Command( int id, int subId, Win* win, void* data );

	virtual bool EventMouse( cevent_mouse* pEvent );

	void EndSearch( cevent_key* pEvent );
 };

class clPanelPlace
{
	struct Node
	{
		clPtr<FS> fsPtr;
		FSPath path;
		std::string current; // name of current item in the panel. Empty string means no current.
	};
	std::vector<Node> m_Stack;
	FSPath m_EmptyPath;

	void Clear( bool toSys = false )
	{
		while ( !m_Stack.empty() )
		{
			bool HasFS       = !m_Stack.back( ).fsPtr.IsNull();
			bool IsSystem    = m_Stack.back( ).fsPtr->Type( ) == FS::SYSTEM;
			bool NoMoreNodes = m_Stack.size() == 1;

			if ( toSys && HasFS && IsSystem && NoMoreNodes ) { break; }

			m_Stack.pop_back();
		}
	}

public:
	clPanelPlace( ) : m_Stack( ), m_EmptyPath() {}

	bool Pop()
	{
		// should be at least 1 item after popping
		if ( m_Stack.size() < 2 ) { return false; }

		m_Stack.pop_back();

		return true;
	}

	// prevPlaceCurrent - name of the current item in the previous panel location,
	//                    the one that sits on top before this Set() call.
	bool Set( clPtr<FS> fsPtr, FSPath& path, bool push, const char* prevPlaceCurrent = 0)
	{
		if ( fsPtr.IsNull() ) { return false; }

		Node node;
		node.fsPtr = fsPtr;
		node.path = path;

		if ( !push )
		{
			Clear( fsPtr->Type() != FS::SYSTEM );
		}
		else
		{
			// before adding new location to the stack
			// update current location with current cursor position
			if (m_Stack.size()>0 && prevPlaceCurrent)
				m_Stack.back().current = prevPlaceCurrent;
		}

		m_Stack.push_back( node );

		return true;
	}

	int Count() const
	{
		return m_Stack.size();
	}

	void Reset( clPtr<FS> fsPtr, FSPath& path )
	{
		if ( fsPtr.IsNull() ) { return; }

		if ( !m_Stack.empty() )
		{
			m_Stack.back().path = path;
			m_Stack.back( ).fsPtr = fsPtr;
		}
		else
		{
			Set( fsPtr, path, false );
		}
	}

	bool IsEmpty() const { return m_Stack.empty(); }

	FS* GetFS() { return !m_Stack.empty() ? m_Stack.back().fsPtr.GetInternalPtr() : 0; }
	clPtr<FS> GetFSPtr() const { return !m_Stack.empty() ? m_Stack.back().fsPtr : clPtr<FS>(); }
	FSPath* GetPathPtr() { return !m_Stack.empty() ? &m_Stack.back().path : 0; }
	FSPath& GetPath() { return !m_Stack.empty() ? m_Stack.back().path : m_EmptyPath; }
	const FSPath& GetPath( ) const { return !m_Stack.empty( ) ? m_Stack.back( ).path : m_EmptyPath; }
	const char* GetCurrent() const { return !m_Stack.empty() ? m_Stack.back().current.c_str() : 0; }
	std::string GetLastPersistentPath()
	{
		FS* pfs = 0;
		FSPath* path = 0;
		for (Node& n : m_Stack)
		{
			if (n.fsPtr->IsPersistent())
			{
				pfs = n.fsPtr.ptr();
				path = &n.path;
			}
		}
		return pfs ? pfs->Uri(*path).GetUtf8() : "";
	}
};

class PanelWin: public NCDialogParent
{
public:
	enum VIEW_MODE
	{
		MEDIUM = 0,
		BRIEF = 1,
		FULL = 2,
		FULL_ST = 3,
		FULL_ACCESS = 4,
		TWO_COLUMNS = 5
	};

	enum LOAD_TYPE { SET = 0, PUSH, RESET };

private:
	static unicode_t dirPrefix[];
	static unicode_t exePrefix[];
	static unicode_t linkPrefix[];

	int dirPrefixW;
	int exePrefixW;

	Layout _lo;
	crect _centerRect;
	crect _headRect;
	crect _footRect;

	ScrollBar _scroll;
	PanelList _list;
	clPtr<PanelSearchWin> _search;

	cpoint _letterSize[2];
	cpoint _longNameMarkExtents;
	bool _longNameMarkExtentsValid;
	int _longNameMarkColor;
	int _itemHeight;
	int _rows;
	int _cols;
	int _first;
	int _current;
	int _viewMode;
	FSStatVfs _vst;

	ccollect<crect, 0x100> _rectList;
	ccollect<crect, 0x100> _emptyRectList;
	ccollect<crect, 0x100> _vLineRectList;

	crect* GetItemRect( int n )
	{
		if ( n < _first || n >= _rectList.count() )
		{
			return 0;
		}

		return & _rectList[n - _first];
	}

	int GetItemNumber( int r, int c )
	{
		int n = c * _rows + r;
		return _first + n;
	}

	void SetScroll();

	void Check();
	void DrawItem( wal::GC& gc,  int n );
	void DrawVerticalSplitters( wal::GC& gc, const crect& rect );
	void DrawFooter( wal::GC& gc );
	void RedrawList( wal::GC& gc );

	clPanelPlace _place;

	unicode_t userNameBuf[64];
	unicode_t groupNameBuf[64];

	const unicode_t* GetUserName( int id ) { FS* fs = GetFS(); if ( fs ) { return fs->GetUserName( id, userNameBuf ); } userNameBuf[0] = 0; return userNameBuf; }
	const unicode_t* GetGroupName( int id ) { FS* fs = GetFS(); if ( fs ) { return fs->GetGroupName( id, groupNameBuf ); } groupNameBuf[0] = 0; return groupNameBuf; }

	bool _inOperState;
	LOAD_TYPE _operType;
	OperRDData _operData;
	clPtr<cstrhash<bool, unicode_t> > _operSelected;
	FSNode _operCurrent;
	FSString _operCurrentStr;
	int _operCursorLoc;

public:
	PanelWin( Win* parent, int mode );
	bool IsSelectedPanel();
	void SelectPanel();

	FSPath& GetPath() { return _place.GetPath(); }
	const FSPath& GetPath( ) const { return _place.GetPath( ); }
	FS* GetFS() { return _place.GetFS(); }
	clPtr<FS> GetFSPtr() const { return _place.GetFSPtr(); }

	FSStatVfs StatVfs() const { return _vst; }

	std::string GetLastPersistentPath() { return _place.GetLastPersistentPath(); } // UTF8 path of last location on FS that IsPersistent(). For saving into registry

	FSString UriOfDir();
	FSString UriOfCurrent();

	void SortByName();
	void SortByExt();
	void SortBySize();
	void SortByMTime();
	void DisableSort();
	SORT_MODE GetSortMode() const { return _list.SortMode(); }
	bool IsAscendingSort() const { return _list.AscSort(); }

	cevent_key QuickSearch( cevent_key* key );
	bool Search( unicode_t* s, bool SearchForNext );

	void SetCurrent( int n );
	bool SetCurrent( FSString& a );
	void SetCurrent( int n, bool shift, LPanelSelectionType* SelectType, bool SelectLastItem );

	int ViewMode() { return _viewMode; }
	void SetViewMode( int m ) { _viewMode = m; Check(); SetScroll(); Invalidate(); }

	void LoadPath( clPtr<FS> fs, FSPath& path, FSString* current, clPtr<cstrhash<bool, unicode_t> > selected, LOAD_TYPE lType );
	void LoadPathStringSafe( const char* path );

	void Reread(bool resetCurrent = false, const char* newCurrentNameUtf8 = 0);

	int GetXMargin() const;

	void KeyPrior() { SetCurrent( _current - _rows * _cols + 1 ); }
	void KeyNext() { SetCurrent( _current + _rows * _cols - 1 ); }

	void KeyUp( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _current - 1, shift, selectType, false ); };
	void KeyDown( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _current + 1, shift, selectType, false ); };
	void KeyEnd( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _list.Count( HideDotsInDir( ) ) - 1, shift, selectType, true ); }
	void KeyHome( bool shift, LPanelSelectionType* selectType ) { SetCurrent( 0, shift, selectType, true ); }
	void KeyPrior( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _current - _rows * _cols + 1, shift, selectType, false ); }
	void KeyNext( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _current + _rows * _cols - 1, shift, selectType, false ); }
	void KeyLeft( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _current - _rows, shift, selectType, true ); }
	void KeyRight( bool shift, LPanelSelectionType* selectType ) { SetCurrent( _current + _rows, shift, selectType, true ); }

	void KeyIns() { _list.InvertSelection( _current, HideDotsInDir() ); SetCurrent( _current + 1 ); }

	int Current() { return _current; }
	FSNode* GetCurrent() { return _list.Get( _current, HideDotsInDir() ); }
	const unicode_t* GetCurrentFileName() { return _list.GetFileName( _current, HideDotsInDir() ); }

	PanelCounter GetSelectedCounter() { return _list.SelectedCounter(); }

	int Count( ) const { return _list.Count( HideDotsInDir() ); }
	const FSNode* Get( int n ) const { return _list.Get( n, HideDotsInDir() ); }
	void RecalcSelectedSize() { _list.RecalcSelectedSize(); }  //  // для перерасчёта общего разверы отображаемого в футере: используется при изменении размеров файлов или директорий

	//dir movies
	/// returns true if the directory was changed
	bool DirUp();
	void DirEnter(bool OpenInExplorer = false);
	void DirRoot();
	bool HideDotsInDir() const;

	virtual void OperThreadSignal( int info );
	virtual void OperThreadStopped();

	std::vector<std::string> GetMatchedFileNames( const std::string& Prefix, size_t MaxItems ) const;

	clPtr<FSList> GetSelectedList()
	{
		if ( _list.SelectedCounter().count > 0 ) { return _list.GetSelectedList(); }

		clPtr<FSList> p = new FSList;
		FSNode* node = GetCurrent();

		if ( node ) { p->CopyOne( node ); }

		return p;
	}

	NCWin* GetNCWin();

	void Invert() { _list.InvertSelection(); Invalidate(); }
	void ClearSelection( cstrhash<bool, unicode_t>* resList ) { _list.ClearSelection( resList ); }
	void Mark( const unicode_t* mask, bool enable ) { _list.Mark( mask, enable ); Invalidate(); }

	virtual int UiGetClassId();
	virtual void Paint( wal::GC& gc, const crect& paintRect );
	virtual void EventSize( cevent_size* pEvent );
	virtual bool EventMouse( cevent_mouse* pEvent );
	virtual bool Command( int id, int subId, Win* win, void* data );
	virtual bool Broadcast( int id, int subId, Win* win, void* data );
	virtual void OnChangeStyles();
public:
	static cicon folderIcon;
	static cicon folderIconHi;
private:
	static cicon linkIcon;
	static cicon executableIcon;
	static cicon serverIcon;
	static cicon workgroupIcon;
	static cicon runIcon;
};
#define PANEL_ICON_SIZE 16
