/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#if defined(__OpenBSD__)
	#include <sys/socket.h>
	#include <sys/types.h>
#elif defined(__FreeBSD__)
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

#include "../slstatus.h"
#include "../util.h"

static const char *
ip(const char *interface, unsigned short sa_family)
{
	struct ifaddrs *ifaddr, *ifa;
	int s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) < 0) {
		warn("getifaddrs:");
		return NULL;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}
		s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
		                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if (!strcmp(ifa->ifa_name, interface) &&
		    (ifa->ifa_addr->sa_family == sa_family)) {
			freeifaddrs(ifaddr);
			if (s != 0) {
				warn("getnameinfo: %s", gai_strerror(s));
				return NULL;
			}
			return bprintf("%s", host);
		}
	}

	freeifaddrs(ifaddr);

	return NULL;
}

const char *
ipv4(const char *interface)
{
	return ip(interface, AF_INET);
}

const char *
ipv6(const char *interface)
{
	return ip(interface, AF_INET6);
}

const char *
leaked_ip(void)
{
	static const char *check_domain = "ip-api.com";
	static const char *check_request =
		"GET /line/?fields=query,country HTTP/1.1\r\n"
		"Host: ip-api.com\r\n\r\n";
	static const char *bad_addr = "X.X.X.X(Unavailable)";
	
	struct addrinfo *ai;
	int remote_fd;
	char *p;
	int s, n;
	int gai_errno;

	if ((gai_errno = getaddrinfo(check_domain, "http", NULL, &ai)))
	{
		warn("Can't resolve domain %s: %s", check_domain, gai_strerror(gai_errno));
		return bad_addr;
	}
	if ((remote_fd = socket(ai->ai_family, ai->ai_socktype, 0)) == -1)
	{
		freeaddrinfo(ai);
		warn("socket:");
		return bad_addr;
	}
	
	if (connect(remote_fd, ai->ai_addr, ai->ai_addrlen) == -1)
	{
		freeaddrinfo(ai);
		close(remote_fd);
		warn("connect:", check_domain);
		return bad_addr;
	}
	freeaddrinfo(ai);

	// send request
	n = strlen(check_request);
	p = (char *) check_request;
	while (n)
	{
		s = write(remote_fd, p, n);

		if (s == -1)
		{
			if (errno == EINTR)
				continue;
			close(remote_fd);
			warn("write:");
			return bad_addr;
		}
		n -= s;
		p += s;
	}
	
	p = buf;
	n = sizeof(buf);
	s = read(remote_fd, p, n);
	close(remote_fd);
	p = strstr(buf, "\r\n\r\n");
	if (!p)
	{
		warn("Can't resolve %s: Bad response from server", check_domain);
		return bad_addr;
	}
	p += 4;
	sscanf(p, "%*s%s", buf);
	strcat(buf, "(");
	sscanf(p, "%s", buf+strlen(buf));
	strcat(buf, ")");
	
	return buf;
}
