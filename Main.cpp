#include "NetFrame/Server.hpp"
#include "NetFrame/tool/Log.hpp"

using namespace std;

int main()
{
	Log::timeEnable = true;
	Log::colorEnable = true;
	Log::logLevel = Log::LogLevel::Info | Log::LogLevel::Warning | Log::LogLevel::Error;

	Server app;
	app.Start();

	return 0;
}
