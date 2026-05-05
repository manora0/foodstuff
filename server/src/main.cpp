#include "../include/httplib.h"
#include <sqlite3.h>
#include <cstdio>
#include <iostream>
#include <sodium.h>
#include <map>
#include <iostream>
#include "routes/search.hpp"
#include "routes/user.hpp"
#include "routes/recipes.hpp"

#define SERVER_CERT_FILE "cert.pem"
#define SERVER_PRIVATE_KEY_FILE "key.pem"

std::map<std::string, int> sessions; // session_id -> user_id

std::string dump_headers(const httplib::Headers &headers) {
  std::string s;
  char buf[BUFSIZ];
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%s %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }
  return s;
}

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

// define loginUser ABOVE main so it's in scope
int loginUser(sqlite3* db, const std::string& username, const std::string& password) { // not secure, just for testing
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT user_id, password FROM user WHERE username = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    return -1; // user not found
  }

  int user_id = sqlite3_column_int(stmt, 0);
  std::string stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
  sqlite3_finalize(stmt);

  // if (crypto_pwhash_str_verify(stored_hash.c_str(), password.c_str(), password.size()) != 0) {
  //   return -1; // wrong password
  // }

  return user_id;
}

int main(void) {
  if (sodium_init() < 0) {
    printf("libsodium init failed\n");
    return -1;
  }

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  httplib::SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
  std::cout << "ssl\n";
#else
  httplib::Server svr;
  std::cout << "no ssl\n";
#endif

  sqlite3* db;
  if (sqlite3_open_v2("../../data/recipies.db", &db,
    SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
    printf("failed to open database: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  if (!svr.is_valid()) {
    printf("server is not valid... check keys\n");
    return -1;
  }

  svr.set_mount_point("/", "../../web/htmx");
  svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
    res.set_redirect("/search/tags.html");
  });

  register_user_routes(svr, db);
  register_recipe_routes(svr, db);
  register_search_routes(svr, db);

  svr.Post("/login", [db](const httplib::Request& req, httplib::Response& res) { // currently allows typing username and password in url for testing, should be changed to form submission in the future
    std::string username = req.get_param_value("username");
    std::string password = req.get_param_value("password");

    int user_id = loginUser(db, username, password); // not secure, just for testing

    if (user_id == -1) {
      res.set_content("Invalid username or password", "text/html");
    } else {
      res.set_header("Set-Cookie", "user_id=" + std::to_string(user_id) + "; HttpOnly; Path=/");
      res.set_header("HX-Redirect", "/");
      res.set_content("", "text/html");
    }
  });

  svr.set_error_handler([](const httplib::Request &, httplib::Response &res) {
    const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), fmt, res.status);
    res.set_content(buf, "text/html");
  });

  svr.set_logger([](const httplib::Request &req, const httplib::Response &res) {
    printf("%s", log(req, res).c_str());
  });

  svr.listen("localhost", 8080);

  return 0;
}