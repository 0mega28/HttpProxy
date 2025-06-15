#include "url.h"
#include <optional>

const std::optional<const proxy_url> parse_proxy_url(const std::string& url) noexcept {
    // extract scheme
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return std::nullopt;
    }
    std::string scheme = url.substr(0, scheme_end);

    // extract host
    auto host_start = scheme_end + 3;
    auto host_end = url.find('/', host_start);
    std::string host;
    if (host_end == std::string::npos) {
        host = url.substr(host_start);
        return proxy_url{scheme, host, "/"};
    } else {
        host = url.substr(host_start, host_end - host_start);
    }

    // extract rest
    std::string rest = url.substr(host_end);

    return proxy_url{scheme, host, rest};
}
