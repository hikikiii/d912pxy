#include "stdafx.h"

d912pxy_log::d912pxy_log()
{
	d912pxy_s(log) = this;

	crashLog = NULL;
	crashLogLine = 0;

#ifndef DISABLE_P7LIB
	//create P7 client object		
	p7cli = P7_Create_Client(d912pxy_s(config)->GetValueRaw(PXY_CFG_LOG_P7CONFIG));

	if (!p7cli)
	{
		MessageBox(0, L"P7 init error", L"d912pxy", MB_ICONERROR);
		throw std::exception();
	}

	p7cli->Share(TM("logger"));

	m_log = P7_Create_Trace(p7cli, TM("d912pxy"));
	m_log->Register_Thread(TM("main thread"), 0);	
#else
	logfile = fopen(PXY_LOG_FILE_NAME, "w");
#endif
}


d912pxy_log::~d912pxy_log()
{
#ifndef DISABLE_P7LIB
	p7cli->Release();
#else
	if (logfile)
		fclose(logfile);
#endif

	if (crashLog)
		fclose(crashLog);
}

#ifndef DISABLE_P7LIB
IP7_Trace * d912pxy_log::GetP7Trace()
{
	return m_log;
}
#endif

void d912pxy_log::RegisterModule(const wchar_t * mdn, d912pxy_log_module * ret)
{
#ifndef DISABLE_P7LIB
	m_log->Register_Module(mdn, ret);
#else
	*ret = (wchar_t*)mdn;
#endif
}

void d912pxy_log::SyncCrashWrite(UINT lock)
{
	if (lock)
		crashLock.Hold();
	else
		crashLock.Release();
}

void d912pxy_log::WriteCrashLogLine(wchar_t * buf)
{
	char fn[255];

	sprintf(fn, "%s.txt", PXY_CRASH_LOG_FILE_PATH);

	if (crashLog == NULL)
	{
		int i = 0;
		while (d912pxy_helper::IsFileExist(fn))
		{
			++i;
			sprintf(fn, "%s%u.txt", PXY_CRASH_LOG_FILE_PATH, i);
		}

		crashLog = fopen(fn, "w");

		if (!crashLog)
		{
			//megai2: i think this will be enough
			MessageBoxW(0, buf, L"d912pxy crash", MB_ICONERROR);
			TerminateProcess(GetCurrentProcess(), -1);			
		}
	}

	fwprintf(crashLog, L"%u | %s \r\n", crashLogLine, buf);	
	fflush(crashLog);
}

void d912pxy_log::WriteLogLine(d912pxy_log_module module, const wchar_t * fmt, const wchar_t * cat, ...)
{
#ifdef DISABLE_P7LIB	
	logLock.Hold();

	fwprintf(logfile, L"%lX [ %s ] %s | ", GetTickCount(), cat, module);

	va_list arg;
	va_start(arg, cat);
	vfwprintf(logfile, fmt, arg);
	va_end(arg);

	fwprintf(logfile, L" \r\n");
	
	fflush(logfile);

	logLock.Release();
#endif
}
