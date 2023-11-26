module;
#include <format>
#include <stdio.h>
#include "network_import.h"

module network.tcp.client;
import network.types;
import <utility>;
import <string>;

using namespace std;

TCP::TCP(SOCKET_TYPE sockfd, const SocketPair& sp) : sockfd{ sockfd }, socket_pair{ sp } {}

TCP::TCP(const SocketPair& sp)
{
#if WINDOWS
	WSAWrapper::instance();
#endif
	sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == INVALID_SOCKET)
		throw std::runtime_error(std::format("'socket' error while creating TCP socket {} : {}", (std::string)sp, get_err_str()));

	sockaddr_in serv_addr = sp.self;

	if (::bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		throw std::runtime_error(std::format("'bind' error while binding TCP socket on {}: {}", (std::string)sp, get_err_str()));

	serv_addr = sp.remote;
	if (::connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		throw std::runtime_error(format("'connect' error while creating TCP socket {} : {}", (std::string)sp, get_err_str()));


	// Fill the socket pair details. 
	// This can be different from input socket details because of INADDR_ANY and Ephemeral port.
	this->socket_pair.remote = sp.remote;
	struct sockaddr_in addr {};
	socklen_t len = sizeof(addr);
	if (getsockname(sockfd, (struct sockaddr*)&addr, &len) < 0)
		throw std::runtime_error(format("'getsockname' error after creating TCP socket {} : {}", (std::string)sp, get_err_str()));

	this->socket_pair.self = addr;
}

TCP::TCP(TCP&& other)
{
	this->sockfd = other.sockfd;
	this->socket_pair = other.socket_pair;
	other.sockfd = INVALID_SOCKET;
	other.socket_pair = {};
}

TCP& TCP::operator=(TCP&& other)
{
	if (&other == this)
		return *this;

	this->sockfd = other.sockfd;
	this->socket_pair = other.socket_pair;
	other.sockfd = INVALID_SOCKET;
	other.socket_pair = {};
	return *this;
}

size_t TCP::send(string_view sv) const
{
	auto nleft = sv.size();
	const char* ptr = sv.data();

	while (nleft > 0)
	{
#if WINDOWS
		int nwritten = ::send(sockfd, ptr, (int)nleft, 0);
#elif UNIX
		int nwritten = ::write(sockfd, ptr, nleft);
#endif
		if (nwritten == SOCKET_ERROR)
		{
#if WINDOWS
			if (nwritten < 0 && WSAGetLastError() == WSAEINTR)
#elif UNIX
			if (nwritten < 0 && errno == EINTR)
#endif
				nwritten = 0;
			else
				throw runtime_error(format("'write' error while sending message '{}' to {}: {}", sv.data(), (string)this->socket_pair, get_err_str()));
		}

		nleft -= nwritten;
		ptr += nwritten;
	}

	return sv.size();
}

std::string TCP::receive(size_t n) const
{
	if (n == 0)
	{
		std::string str(READ_MAX_SIZE, '\0');
#if WINDOWS
		int nread = ::recv(sockfd, str.data(), READ_MAX_SIZE, 0);
#elif UNIX
		int nread = ::read(this->sockfd, str.data(), READ_MAX_SIZE);
#endif
		if (nread < 0)
			throw std::runtime_error(format("'read' error while reading message from {}: {}", (string)this->socket_pair, get_err_str()));

		str.resize(nread);
		return str;
	}

	std::string buffer(n, '\0');
	size_t nleft = n;
	char* ptr = buffer.data();

	while (nleft > 0)
	{
#if WINDOWS
		int nread = recv(sockfd, ptr, (int)nleft, 0);
#elif UNIX
		int nread = ::read(this->sockfd, ptr, nleft);
#endif
		if (nread < 0)
		{
#if WINDOWS
			if (WSAGetLastError() == WSAEINTR)
#elif UNIX
			if (errno == EINTR)
#endif
				nread = 0;
			else
				throw std::runtime_error(format("'read' error while reading message from {}: {}", (string)this->socket_pair, get_err_str()));
		}
		else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}

	return buffer;
}

const SocketPair& TCP::get_socket_pair() const
{
	return socket_pair;
}


void TCP::close()
{
	if (sockfd != INVALID_SOCKET)
#if WINDOWS
		::closesocket(sockfd);
#elif UNIX
		::close(sockfd);
#endif
	sockfd = INVALID_SOCKET;
}


TCP::~TCP()
{
	this->close();
}