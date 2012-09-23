#include <string>
#include <string.h>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>

#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include <windows.h>
#include <process.h>

#include "whuser.h"

using namespace std;

string const serverUrl("http://localhost:8080/RPC2");

pfgws_t m_pget_winholdem_symbol = NULL;

HANDLE symbol_need, symbol_ready, have_result;

double client_result;

string symbol_name;
double symbol_value;
int chair;


vector<string> subscribed_symbols;


void msg(const wchar_t* m, const wchar_t* t)
{
		int msgboxID = MessageBox(
				NULL,
				(LPCWSTR)m,
				(LPCWSTR)t,
				MB_OK);
}

class getSymbol: public xmlrpc_c::method
{
	public:
		getSymbol() {}

		void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP)
		{
			chair = paramList.getInt(0);
			symbol_name = paramList.getString(1);

			SetEvent(symbol_need);
			DWORD wait_result;
			wait_result = WaitForSingleObject(symbol_ready, INFINITE);
			if(WAIT_OBJECT_0 == wait_result)
			{
				*retvalP = xmlrpc_c::value_double(symbol_value);
			} else {
				msg(L"Something went wrong.", L"openholdem-xmlrpc-dll");
			}
		}
};

class subscribe: public xmlrpc_c::method
{
	public:
		subscribe() {}

		void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP)
		{
			xmlrpc_c::value_array
			/*std::vector<xmlrpc_c::value>*/ symbols = paramList.getArray(0);
			std::vector<xmlrpc_c::value> csymbols = symbols.vectorValueValue();

			vector<string> ss;
			for(unsigned int i=0; i<csymbols.size(); i++)
				ss.push_back(xmlrpc_c::value_string(csymbols[i]).cvalue());

			subscribed_symbols = ss;

			*retvalP = xmlrpc_c::value_boolean(true);
		}
};

void xServerThread(void* dummy)
{
	xmlrpc_c::registry* myRegistry = NULL;
	myRegistry = new xmlrpc_c::registry;

	xmlrpc_c::methodPtr const getSymbolP(new getSymbol);
	myRegistry->addMethod("getSymbol", getSymbolP);

	xmlrpc_c::methodPtr const subscribeP(new subscribe);
	myRegistry->addMethod("subscribe_for_symbols", subscribeP);

	xServer = new xmlrpc_c::serverAbyss(
			*myRegistry,
			9091,              // TCP port on which to listen
			"H:/xmlrpc.log"  // Log file
			);
	xServer->run();
}

struct ClientData
{
	char* format;
	char* type;
	void* data;
};

void xClientThread(void* client_data)
{
	xmlrpc_c::value call_result;
	double result = 0;

	ClientData* d = (ClientData*) client_data;

	if (!strcmp(d->type, "query"))
	{
		try {
			xClient->call(serverUrl, "process_message", d->format, &call_result, d->type, d->data);
			result = xmlrpc_c::value_double(call_result);
		} catch (...) {}
		free(d->data);
	} else if(!strcmp(d->type, "state")) {
		try {
			xClient->call(serverUrl, "process_message", d->format, &call_result, d->type, *(xmlrpc_c::value_struct*)d->data);
			result = xmlrpc_c::value_double(call_result);
		} catch (...) {}
		delete (xmlrpc_c::value_struct*) d->data;
	} else if(!strcmp(d->type, "symbols")) {
		try {
			xClient->call(serverUrl, "process_message", d->format, &call_result, d->type, *(xmlrpc_c::value_struct*)d->data);
			result = xmlrpc_c::value_double(call_result);
		} catch (...) {}
		delete (xmlrpc_c::value_struct*) d->data;
	} else if(!strcmp(d->type, "event")) {
		try {
			xClient->call(serverUrl, "process_message", d->format, &call_result, d->type, xmlrpc_c::value_nil());
			result = xmlrpc_c::value_double(call_result);
		} catch (...) {}
	}
	client_result = result;
	SetEvent(have_result);
	free(d);
}

void send_symbols()
{
	bool dupa;
	map<string, xmlrpc_c::value> structData;

	for(unsigned int i=0; i<subscribed_symbols.size(); i++)
		structData.insert(
				pair<string, xmlrpc_c::value> (
					subscribed_symbols[i],
					xmlrpc_c::value_double(m_pget_winholdem_symbol(0, subscribed_symbols[i].c_str(), dupa))
					)
				);

	xmlrpc_c::value_struct* s = new xmlrpc_c::value_struct(structData);

	xmlrpc_c::value result;

	ClientData* cd = (ClientData*)malloc(sizeof(ClientData));

	cd->format = "sS";
	cd->type = "symbols";
	cd->data = (void*) s;

	_beginthread(xClientThread, 0, cd);
	handle_xClient();
}

void process_query(const char* pquery)
{
	try {
		ClientData* cd = (ClientData*)malloc(sizeof(ClientData));
		char* data = (char*)malloc(strlen(pquery) + 1);
		strcpy(data, pquery);
		
		cd->format = "ss";
		cd->type = "query";
		cd->data = (void*) data;

		_beginthread(xClientThread, 0, cd);
		handle_xClient();

	} catch(...){}
}

void process_state(holdem_state* pstate)
{
	map<string, xmlrpc_c::value> structData;
	// char            m_title[64]         ;	//table title
	pair<string, xmlrpc_c::value> m_title("m_title", xmlrpc_c::value_string((pstate)->m_title));
	structData.insert(m_title);

	// double          m_pot[10]           ;	//total in each pot
	vector<xmlrpc_c::value> am_pot;
	for(int i=0; i<10; i++)
		am_pot.push_back(xmlrpc_c::value_double((int)(pstate)->m_pot[i]));

	pair<string, xmlrpc_c::value_array> m_pot("m_pot", am_pot);
	structData.insert(m_pot);		

	// unsigned char   m_cards[5]          ;	//common cards
	vector<xmlrpc_c::value> am_cards;
	for(int i=0; i<5; i++)
		am_cards.push_back(xmlrpc_c::value_int((int)(pstate)->m_cards[i]));

	pair<string, xmlrpc_c::value_array> m_cards("m_cards", am_cards);
	structData.insert(m_cards);				

	// unsigned char   m_is_playing    : 1 ;	//0=sitting-out, 1=sitting-in
	pair<string, xmlrpc_c::value> m_is_playing("m_is_playing", xmlrpc_c::value_int((int)(pstate)->m_is_playing));
	structData.insert(m_is_playing);

	// unsigned char   m_is_posting    : 1 ;	//0=autopost-off, 1=autopost-on
	pair<string, xmlrpc_c::value> m_is_posting("m_is_posting", xmlrpc_c::value_int((int)(pstate)->m_is_posting));
	structData.insert(m_is_posting);

	// unsigned char   m_fillerbits    : 6 ;	//filler bits
	pair<string, xmlrpc_c::value> m_fillerbits("m_fillerbits", xmlrpc_c::value_int((int)(pstate)->m_fillerbits));
	structData.insert(m_fillerbits);

	// unsigned char   m_fillerbyte        ;	//filler byte
	pair<string, xmlrpc_c::value> m_fillerbyte("m_fillerbyte", xmlrpc_c::value_int((int)(pstate)->m_fillerbyte));
	structData.insert(m_fillerbyte);

	// unsigned char   m_dealer_chair      ;	//0-9
	pair<string, xmlrpc_c::value> m_dealer_chair("m_dealer_chair", xmlrpc_c::value_int((int)(pstate)->m_dealer_chair));
	structData.insert(m_dealer_chair);

	//holdem_player   m_player[10]        ;	//player records
	vector<xmlrpc_c::value> am_player;
	for(int p=0; p<10; p++)
	{
		map<string, xmlrpc_c::value> player;
		// char            m_name[16]          ;	//player name if known
		pair<string, xmlrpc_c::value> m_name("m_name", xmlrpc_c::value_string((pstate)->m_player[p].m_name));
		player.insert(m_name);
		// double          m_balance           ;	//player balance
		pair<string, xmlrpc_c::value> m_balance("m_balance", xmlrpc_c::value_double((pstate)->m_player[p].m_balance));
		player.insert(m_balance);
		// double          m_currentbet        ;	//player current bet
		pair<string, xmlrpc_c::value> m_currentbet("m_currentbet", xmlrpc_c::value_double((pstate)->m_player[p].m_currentbet));
		player.insert(m_currentbet);
		// unsigned char   m_cards[2]          ;	//player cards
		vector<xmlrpc_c::value> apm_cards;
		apm_cards.push_back(xmlrpc_c::value_int((int)(pstate)->m_player[p].m_cards[0]));
		apm_cards.push_back(xmlrpc_c::value_int((int)(pstate)->m_player[p].m_cards[1]));

		pair<string, xmlrpc_c::value_array> pm_cards("m_cards", apm_cards);
		player.insert(pm_cards);				

		// unsigned char   m_name_known    : 1 ;	//0=no 1=yes
		pair<string, xmlrpc_c::value> m_name_known("m_name_know", xmlrpc_c::value_int((int)(pstate)->m_player[p].m_name_known));
		player.insert(m_name_known);

		// unsigned char   m_balance_known : 1 ;	//0=no 1=yes
		pair<string, xmlrpc_c::value> m_balance_known("m_balance_know", xmlrpc_c::value_int((int)(pstate)->m_player[p].m_balance_known));
		player.insert(m_balance_known);

		// unsigned char   m_fillerbits    : 6 ;	//filler bits
		pair<string, xmlrpc_c::value> m_fillerbits("m_fillerbits", xmlrpc_c::value_int((int)(pstate)->m_player[p].m_fillerbits));
		player.insert(m_fillerbits);	
		// unsigned char   m_fillerbyte        ;	//filler bytes
		pair<string, xmlrpc_c::value> m_fillerbyte("m_fillerbyte", xmlrpc_c::value_int((int)(pstate)->m_player[p].m_fillerbyte));
		player.insert(m_fillerbyte);	

		am_player.push_back(xmlrpc_c::value_struct(player));
	}
	pair<string, xmlrpc_c::value_array> m_player("m_player", am_player);
	structData.insert(m_player);				
	
	xmlrpc_c::value_struct* s = new xmlrpc_c::value_struct(structData);

	xmlrpc_c::value result;

	ClientData* cd = (ClientData*)malloc(sizeof(ClientData));
		
	cd->format = "sS";
	cd->type = "state";
	cd->data = (void*) s;

	_beginthread(xClientThread, 0, cd);
	handle_xClient();
}


WHUSER_API double process_message(const char* pmessage,	const void* param)
{
	if (pmessage == NULL || param == NULL)
		return 0;

	if (strcmp(pmessage, "event") == 0)
	{
		if(!LOADED)
		{
			LOADED = true;
			DLL_LOAD();
		}
		else
		{
			DLL_UNLOAD();
		}
	}

	else if (strcmp(pmessage, "state") == 0)
		process_state((holdem_state*)param);

	else if (strcmp(pmessage, "query") == 0)
	{
		send_symbols();
		process_query((const char*)param);
	}
	else if (strcmp(pmessage, "pfgws") == 0)
	{
		m_pget_winholdem_symbol = (pfgws_t)param;
		return 0;
	}
	else if (strcmp(pmessage, "phl1k") == 0)
		return 0;

	else if (strcmp(pmessage, "prw1326") == 0)
		return 0;

	else if (strcmp(pmessage, "p_send_chat_message") == 0)
		return 0;

	return client_result;
}

void handle_xClient()
{
	DWORD wait_result;
	bool dupa;
	while(true)
	{
		wait_result = WaitForSingleObject(have_result, 100);
		if(WAIT_OBJECT_0 == wait_result)
		{
			break;
		}
		wait_result = WaitForSingleObject(symbol_need, 100);
		if(WAIT_OBJECT_0 == wait_result)
		{
			symbol_value = m_pget_winholdem_symbol(chair, symbol_name.c_str(), dupa);
			SetEvent(symbol_ready);
		}
	}
}

void DLL_LOAD()
{
	symbol_need = CreateEvent(NULL, FALSE, FALSE, NULL);
	symbol_ready = CreateEvent(NULL, FALSE, FALSE, NULL);
	have_result = CreateEvent(NULL, FALSE, FALSE, NULL);

	xClient = new xmlrpc_c::clientSimple;
	uintptr_t th = _beginthread(xServerThread, 0, NULL);

	ClientData* cd = (ClientData*)malloc(sizeof(ClientData));
		
	cd->format = "sn";
	cd->type = "event";
	cd->data = NULL;

	_beginthread(xClientThread, 0, cd);
	handle_xClient();
}

void DLL_UNLOAD()
{
	ClientData* cd = (ClientData*)malloc(sizeof(ClientData));
		
	cd->format = "sn";
	cd->type = "event";
	cd->data = NULL;

	_beginthread(xClientThread, 0, cd);
	handle_xClient();

	delete xClient;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
