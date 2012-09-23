#ifndef _whuser_h_
#define _whuser_h_

#ifdef WHUSER_EXPORTS
#define WHUSER_API __declspec(dllexport)
#else
#define WHUSER_API __declspec(dllimport)
#endif

#include <xmlrpc-c/client_simple.hpp>
#include <xmlrpc-c/server_abyss.hpp>

bool LOADED = false;

xmlrpc_c::clientSimple* xClient = NULL;
xmlrpc_c::serverAbyss* xServer = NULL;

void DLL_LOAD(); // loading dll
void DLL_UNLOAD(); // unloading dll
void DLL_START(); // Create XMLRPC client/server

void handle_xClient();

struct holdem_player
{
    char            m_name[16]          ;	//player name if known
    double          m_balance           ;	//player balance
    double          m_currentbet        ;	//player current bet
    unsigned char   m_cards[2]          ;	//player cards

    unsigned char   m_name_known    : 1 ;	//0=no 1=yes
    unsigned char   m_balance_known : 1 ;	//0=no 1=yes
    unsigned char   m_fillerbits    : 6 ;	//filler bits
    unsigned char   m_fillerbyte        ;	//filler bytes
};

struct holdem_state
{
    char            m_title[64]         ;	//table title
    double          m_pot[10]           ;	//total in each pot

    unsigned char   m_cards[5]          ;	//common cards

    unsigned char   m_is_playing    : 1 ;	//0=sitting-out, 1=sitting-in
    unsigned char   m_is_posting    : 1 ;	//0=autopost-off, 1=autopost-on
    unsigned char   m_fillerbits    : 6 ;	//filler bits

    unsigned char   m_fillerbyte        ;	//filler byte
    unsigned char   m_dealer_chair      ;	//0-9

    holdem_player   m_player[10]        ;	//player records
};

typedef double (*process_message_t)(const char* message, const void* param );

WHUSER_API double process_message( const char* message, const void* param );

typedef double (*pfgws_t)( int c, const char* psym, bool& iserr );

//#define BOOL unsigned long
//#define APIENTRY __stdcall
/*
#define DLL_PROCESS_DETACH	0
#define DLL_PROCESS_ATTACH	1
#define DLL_THREAD_ATTACH	2
#define DLL_THREAD_DETACH	3
*/
// note about 'TRUE'
/*
Microsoft Specific:
In Visual C++4.2, the Standard C++ header files contained a typedef that equated
bool with int. In Visual C++ 5.0 and later, bool is implemented as a built-in type
with a size of 1 byte. That means that for Visual C++ 4.2, a call of sizeof(bool)
yields 4, while in Visual C++ 5.0 and later, the same call yields 1. This can cause
memory corruption problems if you have defined structure members of type bool in
Visual C++ 4.2 and are mixing object files (OBJ) and/or DLLs built with the 4.2 and
5.0 or later compilers.

quoted from
ms-help://MS.VSExpressCC.v80/MS.NETFramework.v20.en/dv_vclang/html/9abed3f2-d21c-4eb4-97c5-716342e613d8.htm
*/
// so be careful
//#define TRUE (unsigned long) 1;
//  ~nb//


#endif
