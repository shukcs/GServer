#ifndef _SOCKETS_Ipv4Address_H
#define _SOCKETS_Ipv4Address_H

#include "stdconfig.h"
#include "SocketAddress.h"


#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif


/* Ipv4 address implementation.
	\ingroup basic */
class Ipv4Address : public SocketAddress
{
public:
	/** Create empty Ipv4 address structure.
		\param port Port number */
    SHARED_DECL Ipv4Address(port_t port = 0);
	/** Create Ipv4 address structure.
		\param a Socket address in network byte order (as returned by Utility::u2ip)
		\param port Port number in host byte order */
    SHARED_DECL Ipv4Address(ipaddr_t a,port_t port);
	/** Create Ipv4 address structure.
		\param a Socket address in network byte order
		\param port Port number in host byte order */
    SHARED_DECL Ipv4Address(struct in_addr& a,port_t port);
	/** Create Ipv4 address structure.
		\param host Hostname to be resolved
		\param port Port number in host byte order */
    SHARED_DECL Ipv4Address(const std::string& host,port_t port);
    SHARED_DECL Ipv4Address(struct sockaddr_in&);
    SHARED_DECL ~Ipv4Address();

	// SocketAddress implementation

    SHARED_DECL operator struct sockaddr *();
    SHARED_DECL operator socklen_t();
    SHARED_DECL bool operator==(SocketAddress&);

    SHARED_DECL void SetPort(port_t port);
    SHARED_DECL port_t GetPort();

    SHARED_DECL void SetAddress(struct sockaddr *sa);
	int GetFamily();

    SHARED_DECL bool IsValid();

	/** Convert address struct to text. */
	std::string Convert(bool include_port = false);
	std::string Reverse();

	/** Resolve hostname. */
static	bool Resolve(const std::string& hostname,struct in_addr& a);
	/** Reverse resolve (IP to hostname). */
static	bool Reverse(struct in_addr& a,std::string& name);
	/** Convert address struct to text. */
static	std::string Convert(struct in_addr& a);

private:
	Ipv4Address(const Ipv4Address& ) {} // copy constructor
	Ipv4Address& operator=(const Ipv4Address& ) { return *this; } // assignment operator
	struct sockaddr_in m_addr;
	bool m_valid;
};




#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif
#endif // _SOCKETS_Ipv4Address_H

