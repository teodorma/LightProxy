#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/ocsp.h>
#include <chrono>
#include <thread>
#include <csignal>
#include "Logger.h"
#include "LoadBalancer.h"

#define PORT 443
SSL_CTX* ctx = nullptr;
int server_sock = -1;

SSL_CTX* init_ssl_context(Logger& logger) {
    const SSL_METHOD* method = SSLv23_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);

    if (!ctx) {
        ERR_print_errors_fp(stderr);
        logger.log("Error: Failed to create SSL context");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_certificate_chain_file(ctx, "<path-to-certificate>") <= 0) {
        ERR_print_errors_fp(stderr);
        logger.log("Error: Failed to load SSL certificate chain");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "<path-to-key>", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        logger.log("Error: Failed to load SSL private key");
        exit(EXIT_FAILURE);
    }

    logger.log("SSL context initialized successfully");
    return ctx;
}

void signal_handler(int signal) {
    std::cerr << "Received signal: " << signal << ", shutting down server..." << std::endl;

    if (server_sock >= 0) {
        close(server_sock);
        std::cerr << "Server socket closed." << std::endl;
    }

    if (ctx) {
        SSL_CTX_free(ctx);
        EVP_cleanup();
    }

    exit(signal);
}

unsigned char* fetch_ocsp_response(SSL_CTX* ctx, int& ocsp_resp_len, Logger& logger) {
    X509* cert = SSL_CTX_get0_certificate(ctx);
    STACK_OF(OPENSSL_STRING)* ocsp_urls = X509_get1_ocsp(cert);
    if (!ocsp_urls || sk_OPENSSL_STRING_num(ocsp_urls) == 0) {
        std::cerr << "No OCSP responder URL found in certificate." << std::endl;
        logger.log("Warning: No OCSP responder URL found in certificate");
        X509_email_free(ocsp_urls);
        return nullptr;
    }

    OCSP_REQUEST* req = OCSP_REQUEST_new();
    OCSP_CERTID* id = OCSP_cert_to_id(nullptr, cert, nullptr);
    OCSP_request_add0_id(req, id);

    const char* url = sk_OPENSSL_STRING_value(ocsp_urls, 0);
    BIO* bio = BIO_new_connect(url);
    if (!bio) {
        OCSP_REQUEST_free(req);
        std::cerr << "Failed to connect to OCSP responder" << std::endl;
        logger.log("Error: Failed to connect to OCSP responder");
        return nullptr;
    }

    OCSP_RESPONSE* resp = OCSP_sendreq_bio(bio, url, req);
    if (!resp) {
        std::cerr << "OCSP request failed." << std::endl;
        logger.log("Error: OCSP request failed");
        BIO_free_all(bio);
        OCSP_REQUEST_free(req);
        return nullptr;
    }

    ocsp_resp_len = i2d_OCSP_RESPONSE(resp, nullptr);
    auto* ocsp_resp_data = static_cast<unsigned char*>(OPENSSL_malloc(ocsp_resp_len));
    unsigned char* p = ocsp_resp_data;
    i2d_OCSP_RESPONSE(resp, &p);

    BIO_free_all(bio);
    OCSP_RESPONSE_free(resp);
    OCSP_REQUEST_free(req);
    X509_email_free(ocsp_urls);

    return ocsp_resp_data;
}

[[noreturn]] void schedule_ocsp_updates(SSL_CTX* ctx, Logger& logger) {
    while (true) {
        int ocsp_resp_len = 0;
        unsigned char* ocsp_response = fetch_ocsp_response(ctx, ocsp_resp_len, logger);
        if (ocsp_response && ocsp_resp_len > 0) {
#if defined(SSL_CTX_set_tlsext_status_ocsp_resp)
            SSL_CTX_set_tlsext_status_ocsp_resp(ctx, ocsp_response, ocsp_resp_len);
            logger.log("OCSP response updated");
#endif
        }
        std::this_thread::sleep_for(std::chrono::hours(24));
    }
}

std::string forward_to_backend(const std::string& request, const std::string& backend_host, int backend_port, Logger& logger) {
    int backend_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_sock < 0) {
        perror("Backend socket creation failed");
        logger.log("Error: Backend socket creation failed");
        return "";
    }

    sockaddr_in backend_addr{};
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(backend_port);
    inet_pton(AF_INET, backend_host.c_str(), &backend_addr.sin_addr);

    if (connect(backend_sock, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
        perror("Backend connection failed");
        logger.log("Error: Backend connection failed");
        close(backend_sock);
        return "";
    }

    send(backend_sock, request.c_str(), request.size(), 0);

    char buffer[4096];
    std::string response;
    int bytes_received;
    while ((bytes_received = recv(backend_sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytes_received);
    }

    close(backend_sock);
    return response;
}

void handle_client(int client_sock, SSL* ssl, LoadBalancer& load_balancer, Logger& logger) {
    char buffer[4096];
    int bytes = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytes <= 0) {
        ERR_print_errors_fp(stderr);
        logger.log("Error: Failed to read from SSL connection");
        return;
    }
    std::string request(buffer, bytes);

    auto [backend_host, backend_port] = load_balancer.getNextBackend();
    logger.log("Forwarding request to backend: " + backend_host + ":" + std::to_string(backend_port));

    std::string response = forward_to_backend(request, backend_host, backend_port, logger);

    SSL_write(ssl, response.c_str(), response.size());
    logger.log("Response sent to client");
}

[[noreturn]] void run_server(SSL_CTX* ctx, LoadBalancer& load_balancer, Logger& logger) {
    logger.log("Server startup");

    server_sock = socket(AF_INET6, SOCK_STREAM, 0);  // Use AF_INET6 for IPv6
    if (server_sock < 0) {
        perror("Socket creation failed");
        logger.log("Error: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow the socket to support both IPv4 and IPv6
    int option = 0;
    if (setsockopt(server_sock, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&option, sizeof(option)) < 0) {
        perror("Failed to set IPV6_V6ONLY option");
        logger.log("Warning: Failed to set IPV6_V6ONLY option");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    sockaddr_in6 server_addr{};  // Use sockaddr_in6 for IPv4/IPv6 support
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(PORT);
    server_addr.sin6_addr = in6addr_any;  // Bind to all available addresses

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        logger.log("Error: Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, SOMAXCONN) < 0) {
        perror("Listen failed");
        logger.log("Error: Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    logger.log("Server is listening on port " + std::to_string(PORT));
    // std::thread(schedule_ocsp_updates, ctx, std::ref(logger)).detach();

    while (true) {
        sockaddr_in6 client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Client accept failed");
            logger.log("Warning: Client accept failed");
            continue;
        }

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_sock);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            logger.log("Error: SSL handshake failed");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_sock);
            continue;
        }

        handle_client(client_sock, ssl, load_balancer, logger);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_sock);
    }

    close(server_sock);
}

int main() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    Logger logger("../Proxy/Log/server.log");
    ctx = init_ssl_context(logger);
    std::vector<std::string> backends = {"127.0.0.1:8081", "127.0.0.1:8082"};
    LoadBalancer load_balancer(backends);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);

    run_server(ctx, load_balancer, logger);

    SSL_CTX_free(ctx);
    EVP_cleanup();

    return 0;
}
