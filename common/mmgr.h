// ---------------------------------------------------------------------------------------------------------------------------------
// Copyright 2000, Paul Nettle. All rights reserved.
//
// You are free to use this source code in any commercial or non-commercial product.
//
// mmgr.h - Memory manager & tracking software
//
// The most recent version of this software can be found at: ftp://ftp.GraphicsPapers.com/pub/ProgrammingTools/MemoryManagers/
//
// [NOTE: Best when viewed with 8-character tabs]
// ---------------------------------------------------------------------------------------------------------------------------------

// FragMented! {
//#define TRACK_MEMORY	// #define TRACK_MEMORY to look for mem leaks ;)

#ifdef _DEBUG
#	define TRACK_MEMORY
#endif

#ifdef TRACK_MEMORY
// }

#ifndef	_H_MMGR
#define	_H_MMGR

// ---------------------------------------------------------------------------------------------------------------------------------
// For systems that don't have the __FUNCTION__ variable, we can just define it here
// ---------------------------------------------------------------------------------------------------------------------------------

#define	__FUNCTION__ "??"

// ---------------------------------------------------------------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------------------------------------------------------------

typedef	struct tag_au
{
	size_t		actualSize;
	size_t		reportedSize;
	void		*actualAddress;
	void		*reportedAddress;
	char		sourceFile[40];
	char		sourceFunc[40];
	unsigned int	sourceLine;
	unsigned int	allocationType;
	bool		breakOnDealloc;
	bool		breakOnRealloc;
	unsigned int	allocationNumber;
	struct tag_au	*next;
	struct tag_au	*prev;
} sAllocUnit;

typedef	struct
{
	unsigned int	totalReportedMemory;
	unsigned int	totalActualMemory;
	unsigned int	peakReportedMemory;
	unsigned int	peakActualMemory;
	unsigned int	accumulatedReportedMemory;
	unsigned int	accumulatedActualMemory;
	unsigned int	accumulatedAllocUnitCount;
	unsigned int	totalAllocUnitCount;
	unsigned int	peakAllocUnitCount;
} sMStats;

// ---------------------------------------------------------------------------------------------------------------------------------
// External constants
// ---------------------------------------------------------------------------------------------------------------------------------

extern	const	unsigned int	m_alloc_unknown;
extern	const	unsigned int	m_alloc_new;
extern	const	unsigned int	m_alloc_new_array;
extern	const	unsigned int	m_alloc_malloc;
extern	const	unsigned int	m_alloc_calloc;
extern	const	unsigned int	m_alloc_realloc;
extern	const	unsigned int	m_alloc_delete;
extern	const	unsigned int	m_alloc_delete_array;
extern	const	unsigned int	m_alloc_free;

// ---------------------------------------------------------------------------------------------------------------------------------
// Used by the macros
// ---------------------------------------------------------------------------------------------------------------------------------

void		m_setOwner(const char *file, const unsigned int line, const char *func);

// ---------------------------------------------------------------------------------------------------------------------------------
// Allocation breakpoints
// ---------------------------------------------------------------------------------------------------------------------------------

bool		&m_breakOnRealloc(void *reportedAddress);
bool		&m_breakOnDealloc(void *reportedAddress);

// ---------------------------------------------------------------------------------------------------------------------------------
// The meat of the memory tracking software
// ---------------------------------------------------------------------------------------------------------------------------------

void		*m_allocator(const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc,
			     const unsigned int allocationType, const size_t reportedSize);
void		*m_reallocator(const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc,
			       const unsigned int reallocationType, const size_t reportedSize, void *reportedAddress);
void		m_deallocator(const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc,
			      const unsigned int deallocationType, const void *reportedAddress);

// ---------------------------------------------------------------------------------------------------------------------------------
// Utilitarian functions
// ---------------------------------------------------------------------------------------------------------------------------------

bool		m_validateAddress(const void *reportedAddress);
bool		m_validateAllocUnit(const sAllocUnit *allocUnit);
bool		m_validateAllAllocUnits();

// ---------------------------------------------------------------------------------------------------------------------------------
// Unused RAM calculations
// ---------------------------------------------------------------------------------------------------------------------------------

unsigned int	m_calcUnused(const sAllocUnit *allocUnit);
unsigned int	m_calcAllUnused();

// ---------------------------------------------------------------------------------------------------------------------------------
// Logging and reporting
// ---------------------------------------------------------------------------------------------------------------------------------

void		m_dumpAllocUnit(const sAllocUnit *allocUnit, const char *prefix = "");
void		m_dumpMemoryReport(const char *filename = "memreport.log", const bool overwrite = true);
sMStats		m_getMemoryStatistics();

// ---------------------------------------------------------------------------------------------------------------------------------
// Variations of global operators new & delete
// ---------------------------------------------------------------------------------------------------------------------------------

void	*operator new(size_t reportedSize);
void	*operator new[](size_t reportedSize);
void	*operator new(size_t reportedSize, const char *sourceFile, int sourceLine);
void	*operator new[](size_t reportedSize, const char *sourceFile, int sourceLine);
void	operator delete(void *reportedAddress);
void	operator delete[](void *reportedAddress);

#endif // _H_MMGR

// ---------------------------------------------------------------------------------------------------------------------------------
// Macros -- "Kids, please don't try this at home. We're trained professionals here." :)
// ---------------------------------------------------------------------------------------------------------------------------------

#include "nommgr.h"
#define	new		(m_setOwner  (__FILE__,__LINE__,__FUNCTION__),false) ? NULL : new
#define	delete		(m_setOwner  (__FILE__,__LINE__,__FUNCTION__),false) ? m_setOwner("",0,"") : delete
#define	malloc(sz)	m_allocator  (__FILE__,__LINE__,__FUNCTION__,m_alloc_malloc,sz)
#define	calloc(sz,num)	m_allocator  (__FILE__,__LINE__,__FUNCTION__,m_alloc_calloc,sz*num)
#define	realloc(ptr,sz)	m_reallocator(__FILE__,__LINE__,__FUNCTION__,m_alloc_realloc,sz,ptr)
#define	free(ptr)	m_deallocator(__FILE__,__LINE__,__FUNCTION__,m_alloc_free,ptr)

#endif	// _DEBUG

// ---------------------------------------------------------------------------------------------------------------------------------
// mmgr.h - End of file
// ---------------------------------------------------------------------------------------------------------------------------------
