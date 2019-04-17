#include "Log.h"

ErrorTypeDesc ErrorTypeTable[] =
{
	{LOG_FORCE,		"FORCE"},
	{LOG_ERROR,		"ERROR"},
	{LOG_WARNING,	"WARNING"},
	{LOG_NOTICE,	"NOTICE"},
	{LOG_INFO,		"INFO"},
	{LOG_DEBUG,		"DEBUG"},
};

void debugLog(const char *mess,...)
{
	va_list args;
	char szMessage[1024] = {};

	va_start(args,mess);
	//int nMsgLen = _scprintf(mess, args);
	strncat_s(szMessage, sizeof(szMessage), "[GMClient] ", sizeof(szMessage) - 1);
	vsprintf_s(szMessage + 11, sizeof(szMessage) - 12, mess, args);
	va_end(args);

	//fprintf(stdout, szMessage);
	//OutputDebugStringA(szMessage);

#ifdef _WIN32
	//fflush(stderr);
#endif
}
