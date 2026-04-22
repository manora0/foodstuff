#include "recipes.hpp"

void register_recipe_routes(httplib::Server &svr, sqlite3 *db) {
  svr.Get("/recipe-detail", [db](const httplib::Request &req,
                                 httplib::Response &res) {
    if (!req.has_param("id")) {
      res.status = 400;
      return;
    }
    int id = std::stoi(req.get_param_value("id"));

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
                       "SELECT title, author, url, category, cuisine, "
                       "description, image_url, total_time, yields, ratings "
                       "FROM recipe WHERE recipe_id = ?",
                       -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
      res.status = 404;
      res.set_content("Recipe not found", "text/html");
      sqlite3_finalize(stmt);
      return;
    }

    auto col = [&](int i) -> std::string {
      const char *v =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
      return v ? v : "";
    };

    std::string title = col(0);
    std::string author = col(1);
    std::string url = col(2);
    std::string category = col(3);
    std::string cuisine = col(4);
    std::string desc = col(5);
    std::string image_url = col(6);
    std::string time = col(7);
    std::string yields = col(8);
    std::string ratings = col(9);
    sqlite3_finalize(stmt);

    std::string html;
    if (!image_url.empty())
      html += "<img src=\"" + image_url + "\" alt=\"" + title +
              "\" class=\"w-full rounded-xl shadow-md object-cover max-h-72 "
              "mb-6\">";
    html += "<h1 class=\"text-3xl font-bold text-gray-800 mb-2\">" + title +
            "</h1>";
    if (!desc.empty())
      html += "<p class=\"text-gray-600 mb-4\">" + desc + "</p>";
    html += "<div class=\"flex flex-wrap gap-3 text-sm text-gray-500 mb-6\">";
    if (!category.empty())
      html += "<span class=\"bg-gray-100 px-3 py-1 rounded-full\">" + category +
              "</span>";
    if (!cuisine.empty())
      html += "<span class=\"bg-gray-100 px-3 py-1 rounded-full\">" + cuisine +
              "</span>";
    if (!time.empty())
      html += "<span class=\"bg-gray-100 px-3 py-1 rounded-full\">&#x23F1; " +
              time + " min</span>";
    if (!yields.empty())
      html += "<span class=\"bg-gray-100 px-3 py-1 rounded-full\">&#x1F37D; " +
              yields + "</span>";
    if (!ratings.empty())
      html += "<span class=\"bg-gray-100 px-3 py-1 rounded-full\">&#x2605; " +
              ratings + "</span>";
    html += "</div>";
    if (!author.empty())
      html += "<p class=\"text-sm text-gray-400 mb-4\">By " + author + "</p>";
    if (!url.empty())
      html += "<a href=\"" + url +
              "\" class=\"inline-block text-blue-500 hover:underline "
              "text-sm\">View original recipe &rarr;</a>";

    html += "<h2 class=\"text-xl font-semibold text-gray-700 mt-6 mb-3\">Ingredients</h2>";

    sqlite3_stmt* ing_stmt;
    sqlite3_prepare_v2(db, R"(
        SELECT ig.purpose, i.ingredient
        FROM ingredient_group ig
        JOIN ingredient_group_items igi ON ig.group_id = igi.group_id
        JOIN ingredients i ON igi.ingredient_id = i.ingredient_id
        WHERE ig.recipe_id = ?
        ORDER BY ig.group_id
    )", -1, &ing_stmt, nullptr);
    sqlite3_bind_int(ing_stmt, 1, id);

    std::string current_group;
    while (sqlite3_step(ing_stmt) == SQLITE_ROW) {
        const char* purpose_raw = reinterpret_cast<const char*>(sqlite3_column_text(ing_stmt, 0));
        std::string purpose = purpose_raw ? purpose_raw : "";
        std::string ingredient = reinterpret_cast<const char*>(sqlite3_column_text(ing_stmt, 1));

        if (purpose != current_group) {
            if (!current_group.empty()) html += "</ul>";
            if (!purpose.empty())
                html += "<h3 class=\"font-medium text-gray-600 mt-4 mb-1\">" + purpose + "</h3>";
            html += "<ul class=\"list-disc list-inside text-gray-600 space-y-1\">";
            current_group = purpose;
        }
        html += "<li>" + ingredient + "</li>";
    }
    if (!current_group.empty()) html += "</ul>";
    sqlite3_finalize(ing_stmt);

    html += "<h2 class=\"text-xl font-semibold text-gray-700 mt-8 mb-3\">Instructions</h2>";
    html += "<ol class=\"list-decimal list-inside space-y-3 text-gray-600\">";

    sqlite3_stmt* ins_stmt;
    sqlite3_prepare_v2(db, R"(
        SELECT text FROM instructions
        WHERE recipe_id = ?
        ORDER BY step
    )", -1, &ins_stmt, nullptr);
    sqlite3_bind_int(ins_stmt, 1, id);

    while (sqlite3_step(ins_stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(ins_stmt, 0));
        if (text) html += "<li class=\"pl-2\">" + std::string(text) + "</li>";
    }
    sqlite3_finalize(ins_stmt);
    html += "</ol>";

    res.set_content(html, "text/html");
  });
}
