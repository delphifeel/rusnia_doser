#ifndef CURL_WRAPPER_HPP
#define CURL_WRAPPER_HPP

#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <iostream>
#include <optional>

#include "curl/curl.h"
#include "response.hpp"

struct CURLDeleter
{
	void operator()(CURL *curlEnv)
	{
		curl_easy_cleanup(curlEnv);
	}
};

using PCURL = std::unique_ptr<CURL, CURLDeleter>;

class CURLWrapper
{
public:
	CURLWrapper()
	{
		std::call_once(m_initFlag, curl_global_init, CURL_GLOBAL_ALL);

		m_curlEnv.reset(curl_easy_init());
		if(!m_curlEnv)
		{
			std::cerr << "Failed to create curl instance" << std::endl;
			return;
		}

		curl_easy_setopt(m_curlEnv.get(), CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_HTTPPROXYTUNNEL, true);
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_SSL_VERIFYHOST, 0L);
	}

	void SetTarget(const std::string &address)
	{
		if(!m_curlEnv)
		{
			return;
		}
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_URL, address.c_str());

		if(size_t pos = address.find("://");
			pos != std::string::npos)
		{
			std::string scheme{address.begin(), address.begin() + pos};
			curl_easy_setopt(m_curlEnv.get(), CURLOPT_PROXYTYPE, scheme.c_str());
		}
	}

	void SetProxy(const std::string &proxyIP, const std::string &proxyAuth)
	{
		if(!m_curlEnv)
		{
			return;
		}
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_PROXY, proxyIP.c_str());
		if(!proxyAuth.empty())
		{
			curl_easy_setopt(m_curlEnv.get(), CURLOPT_PROXYUSERPWD, proxyAuth.c_str());
		}

		char *url{nullptr};

		curl_easy_getinfo(m_curlEnv.get(), CURLINFO_EFFECTIVE_URL, &url);

		if(url)
		{
			std::string data{std::move(url)};
			if(size_t pos = data.find("://");
				pos != std::string::npos)
			{
				std::string scheme{data.begin(), data.begin() + pos};
				curl_easy_setopt(m_curlEnv.get(), CURLOPT_PROXYTYPE, scheme.c_str());
			}
		}
	}

	void SetHeaders(const std::map<std::string, std::string> &headers)
	{
		struct curl_slist *headers_list{NULL};
		for(auto const &[key, value] : headers)
		{
			headers_list = curl_slist_append(headers_list, (key + value).c_str());
		}

		curl_easy_setopt(m_curlEnv.get(), CURLOPT_HTTPHEADER, headers_list);
	}

	std::optional<Response> Download(long timeout=5)
	{
		std::vector<char> readBuffer;

		curl_easy_setopt(m_curlEnv.get(), CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_WRITEFUNCTION, DataCallback);
		
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_CONNECTTIMEOUT, timeout);

		CURLcode code = curl_easy_perform(m_curlEnv.get());
		if(!ProcessCode(code))
		{
			return {};
		}
		
		long httpCode{0};
		curl_easy_getinfo(m_curlEnv.get(), CURLINFO_RESPONSE_CODE, &httpCode);

		return Response(httpCode, std::move(readBuffer));
	}

	long Ping(long timeout=5)
	{
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_WRITEDATA, NULL);
		curl_easy_setopt(m_curlEnv.get(), CURLOPT_WRITEFUNCTION, DoNothingCallback);

		curl_easy_setopt(m_curlEnv.get(), CURLOPT_TIMEOUT, timeout);

		CURLcode code = curl_easy_perform(m_curlEnv.get());
		if(!ProcessCode(code))
		{
			return {};
		}
		
		long httpCode{0};
		curl_easy_getinfo(m_curlEnv.get(), CURLINFO_RESPONSE_CODE, &httpCode);
		return httpCode;
	}

private:
	static size_t DataCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		std::vector<char> &readBuffer = *static_cast<std::vector<char>*>(userp);

		readBuffer.reserve(readBuffer.size() + nmemb);

		for(size_t i = 0; i < nmemb; i++)
		{
			readBuffer.push_back(((char*)contents)[i]);
		}

		return nmemb;
	}

	static size_t DoNothingCallback(void *, size_t, size_t size, void *)
	{
		return size;
	}


	bool ProcessCode(CURLcode code)
	{
		return code == CURLE_OK;
		// if(code != CURLE_OK)
		// {
		// 	switch (code)
		// 	{
		// 		case CURLE_COULDNT_RESOLVE_PROXY:
		// 		{
		// 			std::cerr << "Proxy non working" << std::endl;
		// 			break;
		// 		}
		// 		case CURLE_OPERATION_TIMEDOUT:
		// 		case CURLE_COULDNT_RESOLVE_HOST:
		// 		{
		// 			std::cerr << "Host is probably down, good work)" << std::endl;
		// 			break;
		// 		}
		// 		case CURLE_COULDNT_CONNECT:
		// 		{
		// 			std::cerr << "Connection error" << std::endl;
		// 			break;
		// 		}
		// 		case CURLE_REMOTE_ACCESS_DENIED:
		// 		{
		// 			std::cerr << "Those bastards are blocking us" << std::endl;
		// 			break;
		// 		}
		// 		default:
		// 		{
		// 			std::cerr << "Some other error, try again" << std::endl;
		// 		}
		// 	}
		// 	return false;
		// }
		// return true;
	}
private:
	PCURL m_curlEnv;

	inline static std::once_flag m_initFlag;
};

#endif // CURL_WRAPPER_HPP
