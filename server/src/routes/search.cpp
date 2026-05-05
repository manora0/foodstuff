#include "search.hpp"
#include <set>
#include <sstream>

void register_search_routes(httplib::Server& svr, sqlite3* db){
  svr.Get("/recipe", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("id")) { res.status = 400; return; }
    int id = std::stoi(req.get_param_value("id"));

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT * FROM recipe WHERE recipe_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      res.set_content("<h1>" + title + "</h1>", "text/html");
      for (int i = 1; i < sqlite3_column_count(stmt); ++i) {
        const char* col_name = sqlite3_column_name(stmt, i);
        const char* col_value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
        res.set_content(res.body + "<p><strong>" + col_name + ":</strong> " + col_value + "</p>", "text/html");
      }
    } else {
      res.status = 404;
      res.set_content("Recipe not found", "text/html");
    }
    sqlite3_finalize(stmt);
  });

  svr.Get("/recipes-all", [db](const httplib::Request&, httplib::Response& res) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT title, recipe_id FROM recipe ORDER BY title", -1, &stmt, nullptr);
    std::string html;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      std::string recipe_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      html += "<li class=\"py-3\"><a href=\"../recipe/recipe.html?id=" + recipe_id + "\" class=\"text-gray-800 hover:text-gray-600 hover:underline font-medium\">" + title + "</a></li>\n";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

  svr.Get("/keywords", [db](const httplib::Request&, httplib::Response& res) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "SELECT k.keyword_id, k.name FROM keywords k "
      "JOIN recipe_keywords rk ON k.keyword_id = rk.keyword_id "
      "GROUP BY k.keyword_id ORDER BY k.name",
      -1, &stmt, nullptr);

    std::string html = "<button class=\"bg-gray-200 text-gray-700 hover:bg-gray-400 hover:text-white px-3 py-1 rounded-full text-sm font-medium transition-colors cursor-pointer\" hx-get=\"/recipes-all\" hx-target=\"#recipe-list\" hx-swap=\"innerHTML\">All</button>\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        html += "<button "
                "class=\"bg-green-100 text-green-700 hover:bg-green-500 hover:text-white px-3 py-1 rounded-full text-sm font-medium transition-colors cursor-pointer\" "
                "hx-get=\"/recipes-by-keyword?id=" + std::to_string(id) + "\" "
                "hx-target=\"#recipe-list\" "
                "hx-swap=\"innerHTML\">"
                + name +
                "</button>\n";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

  svr.Get("/categories", [db](const httplib::Request&, httplib::Response& res) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "SELECT DISTINCT category FROM recipe WHERE category IS NOT NULL AND category != ''",
      -1, &stmt, nullptr);

    std::set<std::string> seen;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::stringstream ss(raw);
        std::string token;
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) seen.insert(token);
        }
    }
    sqlite3_finalize(stmt);

    std::string html = "<button class=\"bg-gray-200 text-gray-700 hover:bg-gray-400 hover:text-white px-3 py-1 rounded-full text-sm font-medium transition-colors cursor-pointer\" hx-get=\"/recipes-all\" hx-target=\"#recipe-list\" hx-swap=\"innerHTML\">All</button>\n";
    for (const auto& name : seen) {
        html += "<button "
                "class=\"bg-purple-100 text-purple-700 hover:bg-purple-500 hover:text-white px-3 py-1 rounded-full text-sm font-medium transition-colors cursor-pointer\" "
                "hx-get=\"/recipes-by-category?category=" + name + "\" "
                "hx-target=\"#recipe-list\" "
                "hx-swap=\"innerHTML\">"
                + name +
                "</button>\n";
    }
    res.set_content(html, "text/html");
  });

  svr.Get("/recipes-by-category", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("category")) { res.status = 400; return; }
    std::string category = req.get_param_value("category");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "SELECT title, recipe_id FROM recipe WHERE category = ? OR category LIKE ? || ',%' OR category LIKE '%,' || ? OR category LIKE '%,' || ? || ',%' ORDER BY title",
      -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, category.c_str(), -1, SQLITE_TRANSIENT);

    std::string html;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      std::string recipe_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      html += "<li class=\"py-3\"><a href=\"../recipe/recipe.html?id=" + recipe_id + "\" class=\"text-gray-800 hover:text-purple-600 hover:underline font-medium\">" + title + "</a></li>\n";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

  svr.Get("/cuisines", [db](const httplib::Request&, httplib::Response& res) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "SELECT DISTINCT cuisine FROM recipe WHERE cuisine IS NOT NULL AND cuisine != '' ORDER BY cuisine",
      -1, &stmt, nullptr);

    std::string html = "<button class=\"bg-gray-200 text-gray-700 hover:bg-gray-400 hover:text-white px-3 py-1 rounded-full text-sm font-medium transition-colors cursor-pointer\" hx-get=\"/recipes-all\" hx-target=\"#recipe-list\" hx-swap=\"innerHTML\">All</button>\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        html += "<button "
                "class=\"bg-blue-100 text-blue-700 hover:bg-blue-500 hover:text-white px-3 py-1 rounded-full text-sm font-medium transition-colors cursor-pointer\" "
                "hx-get=\"/recipes-by-cuisine?cuisine=" + name + "\" "
                "hx-target=\"#recipe-list\" "
                "hx-swap=\"innerHTML\">"
                + name +
                "</button>\n";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

  svr.Get("/recipes-by-cuisine", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("cuisine")) { res.status = 400; return; }
    std::string cuisine = req.get_param_value("cuisine");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "SELECT title, recipe_id FROM recipe WHERE cuisine = ? ORDER BY title",
      -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, cuisine.c_str(), -1, SQLITE_TRANSIENT);

    std::string html;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      std::string recipe_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      html += "<li class=\"py-3\"><a href=\"../recipe/recipe.html?id=" + recipe_id + "\" class=\"text-gray-800 hover:text-blue-600 hover:underline font-medium\">" + title + "</a></li>\n";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

  svr.Get("/recipes-by-keyword", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("id")) { res.status = 400; return; }
    int id = std::stoi(req.get_param_value("id"));

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "SELECT r.title, r.recipe_id FROM recipe r "
      "JOIN recipe_keywords rk ON r.recipe_id = rk.recipe_id "
      "WHERE rk.keyword_id = ? ORDER BY r.title",
      -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    std::string html;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      std::string recipe_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      html += "<li class=\"py-3\"><a href=\"../recipe/recipe.html?id=" + recipe_id + "\" class=\"text-gray-800 hover:text-green-600 hover:underline font-medium\">" + title + "</a></li>\n";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });
}