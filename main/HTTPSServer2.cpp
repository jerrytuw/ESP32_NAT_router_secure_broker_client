/*
    from httpsserver lib(modified) to add checkAvail
*/
#include "HTTPSServer2.hpp"

namespace httpsserver {


HTTPSServer2::HTTPSServer2(SSLCert * cert, const uint16_t port, const uint8_t maxConnections, const in_addr_t bindAddress):
  HTTPServer(port, maxConnections, bindAddress),
  _cert(cert) {

  // Configure runtime data
  _sslctx = NULL;
}

HTTPSServer2::~HTTPSServer2() {

}

/**
   This method starts the server and begins to listen on the port
*/
uint8_t HTTPSServer2::setupSocket() {
  if (!isRunning()) {
    if (!setupSSLCTX()) {
      Serial.println("setupSSLCTX failed");
      return 0;
    }

    if (!setupCert()) {
      Serial.println("setupCert failed");
      SSL_CTX_free(_sslctx);
      _sslctx = NULL;
      return 0;
    }

    if (HTTPServer::setupSocket()) {
      return 1;
    } else {
      Serial.println("setupSockets failed");
      SSL_CTX_free(_sslctx);
      _sslctx = NULL;
      return 0;
    }
  } else {
    return 1;
  }
}

void HTTPSServer2::teardownSocket() {

  HTTPServer::teardownSocket();

  // Tear down the SSL context
  SSL_CTX_free(_sslctx);
  _sslctx = NULL;
}

int HTTPSServer2::createConnection(int idx) {
  HTTPSConnection * newConnection = new HTTPSConnection(this);
  _connections[idx] = newConnection;
  return newConnection->initialize(_socket, _sslctx, &_defaultHeaders);
}

/**
   This method configures the ssl context that is used for the server
*/
uint8_t HTTPSServer2::setupSSLCTX() {
  _sslctx = SSL_CTX_new(TLSv1_2_server_method());
  if (_sslctx) {
    // Set SSL Timeout to 5 minutes
    SSL_CTX_set_timeout(_sslctx, 300);
    return 1;
  } else {
    _sslctx = NULL;
    return 0;
  }
}

/**
   This method configures the certificate and private key for the given
   ssl context
*/
uint8_t HTTPSServer2::setupCert() {
  // Configure the certificate first
  uint8_t ret = SSL_CTX_use_certificate_ASN1(
                  _sslctx,
                  _cert->getCertLength(),
                  _cert->getCertData()
                );

  // Then set the private key accordingly
  if (ret) {
    ret = SSL_CTX_use_RSAPrivateKey_ASN1(
            _sslctx,
            _cert->getPKData(),
            _cert->getPKLength()
          );
  }

  return ret;
}
HTTPConnection *HTTPSServer2::checkAvail() {

  // Only handle requests if the server is still running
  if (!_running)
  {
    log_v("not running");
    return 0;
  }

  // Step 1: Process existing connections
  // Process open connections and store the index of a free connection
  // (we might use that later on)
  int freeConnectionIdx = -1;
  for (int i = 0; i < _maxConnections; i++) {
    // Fetch a free index in the pointer array
    if (_connections[i] == NULL) {
      freeConnectionIdx = i;

    } else {
      // if there is a connection (_connections[i]!=NULL), check if its open or closed:
      if (_connections[i]->isClosed()) {
        // if it's closed, clean up:
        delete _connections[i];
        _connections[i] = NULL;
        freeConnectionIdx = i;
      } else {
        // if not, process it:
        //_connections[i]->loop();
      }
    }
  }

  // Step 2: Check for new connections
  // This makes only sense if there is space to store the connection
  if (freeConnectionIdx > -1) {

    // We create a file descriptor set to be able to use the select function
    fd_set sockfds;
    // Out socket is the only socket in this set
    FD_ZERO(&sockfds);
    FD_SET(_socket, &sockfds);

    // We define a "immediate" timeout
    timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0; // Return immediately, if possible

    // Wait for input
    // As by 2017-12-14, it seems that FD_SETSIZE is defined as 0x40, but socket IDs now
    // start at 0x1000, so we need to use _socket+1 here
    select(_socket + 1, &sockfds, NULL, NULL, &timeout);

    // There is input
    if (FD_ISSET(_socket, &sockfds)) {
      int socketIdentifier = createConnection(freeConnectionIdx);
        log_v("is connection");
      //HTTPS_LOGI("Loop. New connection. Socket FID=%d", socketIdentifier);

      // If initializing did not work, discard the new socket immediately
      if (socketIdentifier < 0) {
        delete _connections[freeConnectionIdx];
        _connections[freeConnectionIdx] = NULL;
      }
      else
      {
        log_v("avail connection");
        //HTTPS_LOGI("Avail. return=%d", 1);
        return _connections[freeConnectionIdx];
      }
    }
  }
  return nullptr;
}
} /* namespace HTTPSServer2 */
