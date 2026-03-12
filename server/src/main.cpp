#include <cstdio>
#include <iostream>
#include <chrono>
#include "../include/httplib.h"

#define SERVER_CERT_FILE "cert.pem"
#define SERVER_PRIVATE_KEY_FILE "key.pem"

std::string dump_headers(const httplib::Headers &headers) {
    std::string s;
    char buf[BUFSIZ];
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%s %s\n", x.first.c_str(), x.second.c_str());
        s+=buf;
    }
    return s;
} // Reads header key-value pairs and prints them

std::string log(const httplib::Request &req, const httplib::Response &res) {
    std::string s;
    char buf[BUFSIZ];

    s += "====================================\n";

    snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
                req.version.c_str(), req.path.c_str());
    s += buf;

    std::string query;
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%c%s=%s",
            (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
            x.second.c_str());
        query += buf;
    }
    snprintf(buf, sizeof(buf), "%s\n", query.c_str());
    s += buf;

    s += dump_headers(req.headers);

    s += "-------------------------------------\n";

    snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
    s += dump_headers(res.headers);
    s += "\n";

    if (!res.body.empty()) { s += res.body; }
    s += "\n";

    return s;
}


int main(void) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
    std::cout << "ssl\n";
#else
    httplib::Server svr;
    std::cout << "no ssl\n";
#endif

    if (!svr.is_valid()) {
        printf("server has an error...\n");
        return -1;
    }

    svr.set_mount_point("/", "./web");

    svr.Post("/post", [](const auto &req, auto &res) {
        res.set_content(req.body, "text/plain");
    });

    // svr.Get("/", [=](const httplib::Request & /*req*/, httplib::Response &res) {
    //     res.set_redirect("/hi");
    // });

    // svr.Get("/", [](const httplib::Request & /*req*/, httplib::Response &res) {
    //     res.set_file_content("web/index.html");
    // });


    // svr.Get("/hi", [](const httplib::Request & /*req*/, httplib::Response &res) {
    //     res.set_content("Hello World!\n", "text/plain");
    // });

    // svr.Get("/slow", [](const httplib::Request & /*req*/, httplib::Response &res) {
    //     std::this_thread::sleep_for(std::chrono::seconds(2));
    //     res.set_content("Slow...\n", "text/plain");
    // });

    // svr.Get("/dump", [](const httplib::Request &req, httplib::Response &res) {
    //     res.set_content(dump_headers(req.headers), "text/plain");
    // });

    // svr.Get("/stop",
    //     [&](const httplib::Request & /*req*/, httplib::Response & /*res*/) { svr.stop(); });

    svr.set_error_handler([](const httplib::Request & /*req*/, httplib::Response &res) {
      const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
      char buf[BUFSIZ];
      snprintf(buf, sizeof(buf), fmt, res.status);
      res.set_content(buf, "text/html");
    });

    svr.set_logger([](const httplib::Request & req, const httplib::Response &res) {
        printf("%s", log(req, res).c_str());
    });

    svr.listen("localhost", 8080);

    std::cout << "hello world\n";
    return 0;
}
