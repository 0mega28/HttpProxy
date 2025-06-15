#pragma once

#include <string>
#include <optional>

struct proxy_url {
    const std::string scheme;
    const std::string host;
    std::optional<const std::string> port;
    const std::string rest;
};

// hacky to get it working
const std::optional<const proxy_url> parse_proxy_url(const std::string& url) noexcept;
