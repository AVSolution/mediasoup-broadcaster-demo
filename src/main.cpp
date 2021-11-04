#include "Broadcaster.hpp"
#include "mediasoupclient.hpp"
#include <cpr/cpr.h>
#include <csignal> // sigsuspend()
#include <cstdlib>
#include <iostream>
#include <string>

using json = nlohmann::json;

void signalHandler(int signum)
{
	std::cout << "[INFO] interrupt signal (" << signum << ") received" << std::endl;

	std::cout << "[INFO] leaving!" << std::endl;

	std::exit(signum);
}

int main(int /*argc*/, char* /*argv*/[])
{
	// Register signal SIGINT and signal handler.
	signal(SIGINT, signalHandler);

	// Retrieve configuration from environment variables.
	//const char* envServerUrl    = std::getenv("SERVER_URL");
	//const char* envRoomId       = std::getenv("ROOM_ID");
	//const char* envEnableAudio  = std::getenv("ENABLE_AUDIO");
	//const char* envUseSimulcast = std::getenv("USE_SIMULCAST");
	//const char* envWebrtcDebug  = std::getenv("WEBRTC_DEBUG");
	//const char* envVerifySsl    = std::getenv("VERIFY_SSL");
	const char* envServerUrl    = "https://192.168.25.10:4443"; //"https://192.168.26.133:4443";
	const char* envRoomId       = "vbsumz8k";                   
	const char* envEnableAudio  = "true";
	const char* envUseSimulcast = "true";
	const char* envWebrtcDebug  = "info";
	const char* envVerifySsl    = "false";

	if (envServerUrl == nullptr)
	{
		std::cerr << "[ERROR] missing 'SERVER_URL' environment variable" << std::endl;

		return 1;
	}

	if (envRoomId == nullptr)
	{
		std::cerr << "[ERROR] missing 'ROOM_ID' environment variable" << std::endl;

		return 1;
	}

	std::string baseUrl = envServerUrl;
	baseUrl.append("/rooms/").append(envRoomId);

	bool enableAudio = true;

	if (envEnableAudio && std::string(envEnableAudio) == "false")
		enableAudio = false;

	bool useSimulcast = true;

	if (envUseSimulcast && std::string(envUseSimulcast) == "false")
		useSimulcast = false;

	bool verifySsl = true;
	if (envVerifySsl && std::string(envVerifySsl) == "false")
		verifySsl = false;

	//std::cout << "baseUrl: " << baseUrl << std::endl
	//          << "enableAudio:" << enableAudio << std::endl
	//          << "useSimulcast:" << useSimulcast << std::endl;
	// Set RTC logging severity.
	if (envWebrtcDebug)
	{
		if (std::string(envWebrtcDebug) == "info")
			rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_INFO);
		else if (std::string(envWebrtcDebug) == "warn")
			rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_WARNING);
		else if (std::string(envWebrtcDebug) == "error")
			rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_ERROR);
	}

	auto logLevel = mediasoupclient::Logger::LogLevel::LOG_DEBUG;
	mediasoupclient::Logger::SetLogLevel(logLevel);
	mediasoupclient::Logger::SetDefaultHandler();
	mediasoupclient::Logger::setLogPath("broadcaster.log");

	MSC_DEBUG( "envServerUrl: %s, \n envRoomId: %s, \n envEnableAudio:%s, \n  ,\
	envUseSimulcast : %s \n,  envWebrtcDebug : %s \n  , envVerifySsl  : %s  \n",
	  envServerUrl,
	  envRoomId,
	  envEnableAudio,
	  envUseSimulcast,
	  envWebrtcDebug,
	  envVerifySsl);

	// Initilize mediasoupclient.
	mediasoupclient::Initialize();

	
	std::cout << "[INFO] welcome to mediasoup broadcaster app!\n" << std::endl;

	std::cout << "[INFO] verifying that room '" << envRoomId << "' exists..." << std::endl;
	auto r = cpr::GetAsync(cpr::Url{ baseUrl }, cpr::VerifySsl{ verifySsl }).get();

	if (r.status_code != 200)
	{
		std::cerr << "[ERROR] unable to retrieve room info"
		          << " [status code:" << r.status_code
		          << ", body:" << r.text 
				  << ", error: " <<r.error.message<<"\"]" << std::endl;
		
		return 1;
	}
	else
	{
		std::cout << "[INFO] found room " << envRoomId << std::endl;
	}

	MSC_DEBUG("get request : %s \n response: %s ", baseUrl.c_str(),r.text.c_str());
	auto response = nlohmann::json::parse(r.text);

	Broadcaster broadcaster;

	broadcaster.Start(baseUrl, enableAudio, useSimulcast, response, verifySsl);

	std::cout << "[INFO] press Ctrl+C or Cmd+C to leave..." << std::endl;

	while (true)
	{
		std::cin.get();
	}

	mediasoupclient::Cleanup();
	return 0;
}
