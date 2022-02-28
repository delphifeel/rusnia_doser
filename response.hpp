#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <vector>

struct Response
{
public:
	Response(int code, std::vector<char> &&data) :
		m_code{code},
		m_data{std::move(data)}
	{
	}

	int m_code;
	std::vector<char> m_data;
};

#endif // RESPONSE_HPP
