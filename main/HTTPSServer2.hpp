/*
    from httpsserver lib(modified) to add checkAvail
*/
#ifndef SRC_HTTPSSERVER2_HPP_
#define SRC_HTTPSSERVER2_HPP_
/* modified from ESP32_HTTPS_Server

*/
// Standard library
#include <string>

// Arduino stuff
#include <Arduino.h>

// Required for SSL
#include "openssl/ssl.h"
#undef read

// Internal includes
#include "HTTPServer.hpp"
#include "HTTPSServerConstants.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPHeader.hpp"
#include "ResourceNode.hpp"
#include "ResourceResolver.hpp"
#include "ResolvedResource.hpp"
#include "HTTPSConnection.hpp"
#include "SSLCert.hpp"

namespace httpsserver {

/**
   \brief Main implementation of the HTTP Server with TLS support. Use HTTPServer for plain HTTP
*/
class HTTPSServer2 : public HTTPServer {
  public:
    HTTPSServer2(SSLCert * cert, const uint16_t portHTTPS = 443, const uint8_t maxConnections = 4, const in_addr_t bindAddress = 0);
    virtual ~HTTPSServer2();

    HTTPConnection *checkAvail();

  private:
    // Static configuration. Port, keys, etc. ====================
    // Certificate that should be used (includes private key)
    SSLCert * _cert;

    //// Runtime data ============================================
    SSL_CTX * _sslctx;
    // Status of the server: Are we running, or not?

    // Setup functions
    virtual uint8_t setupSocket();
    virtual void teardownSocket();
    uint8_t setupSSLCTX();
    uint8_t setupCert();

    // Helper functions
    virtual int createConnection(int idx);
};

} /* namespace httpsserver */

#endif /* SRC_HTTPSSERVER_HPP_ */
