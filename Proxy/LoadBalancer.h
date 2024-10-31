//
// Created by maciucateodor on 10/26/24.
//

#ifndef SOFTWARE_ENGINEERING_PRODUCT_TOPTEAM_LOADBALANCER_H
#define SOFTWARE_ENGINEERING_PRODUCT_TOPTEAM_LOADBALANCER_H

#include <string>
#include <vector>

class LoadBalancer {
public:
    explicit LoadBalancer(const std::vector<std::string>& backends) : backends_(backends), next_backend_(0) {}

    std::pair<std::string, int> getNextBackend() {
        auto& backend = backends_[next_backend_];
        next_backend_ = (next_backend_ + 1) % backends_.size();

        auto pos = backend.find(':');
        std::string host = backend.substr(0, pos);
        int port = std::stoi(backend.substr(pos + 1));
        return {host, port};
    }

private:
    std::vector<std::string> backends_;
    std::size_t next_backend_;
};

#endif //SOFTWARE_ENGINEERING_PRODUCT_TOPTEAM_LOADBALANCER_H
