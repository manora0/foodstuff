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

    html += "<button onclick=\"openMealPlanModal(" + std::to_string(id) + ")\" "
            "class=\"mt-4 px-4 py-2 bg-green-500 text-white rounded-lg hover:bg-green-600 text-sm\">+ Add to Meal Plan</button>";

    html += "<h2 class=\"text-xl font-semibold text-gray-700 mt-6 mb-3\">Ingredients</h2>";

    sqlite3_stmt* ing_stmt;
    sqlite3_prepare_v2(db, R"(
        SELECT i.name, i.size, ri.unit, ri.quantity, ri.preparation
        FROM recipe r
        JOIN recipe_ingredients ri ON r.recipe_id = ri.recipe_id
        JOIN ingredients i ON ri.ingredient_id = i.ingredient_id
        WHERE r.recipe_id = ?
    )", -1, &ing_stmt, nullptr);
    sqlite3_bind_int(ing_stmt, 1, id);

    html += "<ul class=\"space-y-2 text-gray-600 list-none\">";
    while (sqlite3_step(ing_stmt) == SQLITE_ROW) {
        auto icol = [&](int i) -> std::string {
            const char* v = reinterpret_cast<const char*>(sqlite3_column_text(ing_stmt, i));
            return v ? v : "";
        };

        std::string name        = icol(0);
        std::string size        = icol(1);
        std::string unit        = icol(2);
        std::string preparation = icol(4);
        

        char qty_buf[32];
        double quantity = sqlite3_column_double(ing_stmt, 3);
        if (quantity != 1.0 && !unit.empty())
          unit += "s";  // "2 teaspoons", "3 cups"
        if (quantity > 0)
            snprintf(qty_buf, sizeof(qty_buf), "%g", quantity);
        else
            qty_buf[0] = '\0';

        std::string label;
        if (qty_buf[0]) label += std::string(qty_buf) + " ";
        if (!unit.empty()) label += unit + " ";
        if (!size.empty()) label += size + " ";
        label += name;

        std::string full = label;
        if (!preparation.empty()) full += ", " + preparation;

        html += "<li class=\"flex items-center gap-2\">";
        html += "<input type=\"checkbox\" class=\"w-4 h-4 accent-green-500\">";
        html += "<span>" + full + "</span>";
        html += "</li>";
    }
    html += "</ul>";
    sqlite3_finalize(ing_stmt);

    html += "<h2 class=\"text-xl font-semibold text-gray-700 mt-8 mb-3\">Instructions</h2>";

    sqlite3_stmt* ins_stmt;
    sqlite3_prepare_v2(db, R"(
        SELECT section, text FROM instructions
        WHERE recipe_id = ?
        ORDER BY step
    )", -1, &ins_stmt, nullptr);
    sqlite3_bind_int(ins_stmt, 1, id);

    std::string current_section;
    while (sqlite3_step(ins_stmt) == SQLITE_ROW) {
        const char* section_raw = reinterpret_cast<const char*>(sqlite3_column_text(ins_stmt, 0));
        const char* text_raw = reinterpret_cast<const char*>(sqlite3_column_text(ins_stmt, 1));
        std::string section = section_raw ? section_raw : "";
        if (!text_raw) continue;

        if (section != current_section) {
            if (!current_section.empty()) html += "</ol>";
            if (!section.empty())
                html += "<h3 class=\"font-medium text-gray-600 mt-6 mb-2\">" + section + "</h3>";
            html += "<ol class=\"list-decimal list-inside space-y-3 text-gray-600\">";
            current_section = section;
        }
        html += "<li class=\"pl-2\">" + std::string(text_raw) + "</li>";
    }
    if (!current_section.empty()) html += "</ol>";
    sqlite3_finalize(ins_stmt);

    res.set_content(html, "text/html");
  });

  auto build_mealplan_html = [](sqlite3* db, int recipe_id) -> std::string {
    const char* day_names[] = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    int user_id = 1;
    std::string id_str = std::to_string(recipe_id);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT plan_id, name FROM mealplan WHERE user_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);

    std::string options;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int plan_id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        options += "<option value=\"" + std::to_string(plan_id) + "\">" + std::string(name) + "</option>";
    }
    sqlite3_finalize(stmt);

    if (options.empty())
        return "<p class=\"text-gray-400 text-sm\">No meal plans yet.</p>";

    std::string slot_trigger =
        "hx-get=\"/mealplan-slot\" "
        "hx-target=\"#slot-preview\" "
        "hx-swap=\"innerHTML\" "
        "hx-trigger=\"change\" "
        "hx-include=\"[name='plan_id'],[name='weekday'],[name='meal_time']\" ";

    std::string html =
        "<form hx-post=\"/add-to-mealplan\" hx-target=\"#mealplan-list\" hx-swap=\"innerHTML\" "
        "class=\"flex flex-col gap-2\">"
        "<input type=\"hidden\" name=\"recipe_id\" value=\"" + id_str + "\">"
        "<select name=\"plan_id\" required " + slot_trigger + "class=\"border border-gray-200 rounded-lg px-3 py-2 text-sm\">"
        "<option value=\"\">Select Meal Plan</option>" + options +
        "</select>"
        "<select name=\"weekday\" required " + slot_trigger + "class=\"border border-gray-200 rounded-lg px-3 py-2 text-sm\">"
        "<option value=\"1\">Monday</option><option value=\"2\">Tuesday</option>"
        "<option value=\"3\">Wednesday</option><option value=\"4\">Thursday</option>"
        "<option value=\"5\">Friday</option><option value=\"6\">Saturday</option>"
        "<option value=\"7\">Sunday</option>"
        "</select>"
        "<select name=\"meal_time\" required " + slot_trigger + "class=\"border border-gray-200 rounded-lg px-3 py-2 text-sm\">"
        "<option value=\"Breakfast\">Breakfast</option>"
        "<option value=\"Lunch\">Lunch</option>"
        "<option value=\"Dinner\">Dinner</option>"
        "</select>"
        "<div id=\"slot-preview\"></div>"
        "<button type=\"submit\" class=\"bg-green-500 text-white px-4 py-2 rounded-lg text-sm hover:bg-green-600\">Add</button>"
        "</form>";

    sqlite3_stmt* sched_stmt;
    sqlite3_prepare_v2(db, R"(
        SELECT ms.schedule_id, mp.name, ms.day, ms.meal_type
        FROM mealplan_schedule ms
        JOIN mealplan mp ON ms.plan_id = mp.plan_id
        WHERE ms.recipe_id = ? AND mp.user_id = ?
        ORDER BY ms.day, ms.meal_type
    )", -1, &sched_stmt, nullptr);
    sqlite3_bind_int(sched_stmt, 1, recipe_id);
    sqlite3_bind_int(sched_stmt, 2, user_id);

    std::string list;
    while (sqlite3_step(sched_stmt) == SQLITE_ROW) {
        int schedule_id = sqlite3_column_int(sched_stmt, 0);
        const char* plan_name = reinterpret_cast<const char*>(sqlite3_column_text(sched_stmt, 1));
        int day = sqlite3_column_int(sched_stmt, 2);
        const char* meal_type = reinterpret_cast<const char*>(sqlite3_column_text(sched_stmt, 3));

        std::string day_str = (day >= 1 && day <= 7) ? day_names[day] : std::to_string(day);
        std::string label = std::string(plan_name ? plan_name : "") + " &mdash; " + day_str + " " + (meal_type ? meal_type : "");

        list += "<li class=\"flex items-center justify-between text-sm text-gray-600 py-1\">"
                "<span>" + label + "</span>"
                "<button hx-post=\"/remove-from-mealplan\" "
                "hx-vals='{\"schedule_id\":\"" + std::to_string(schedule_id) + "\",\"recipe_id\":\"" + id_str + "\"}' "
                "hx-target=\"#mealplan-list\" hx-swap=\"innerHTML\" "
                "class=\"text-red-400 hover:text-red-600 text-xs px-2 py-0.5 rounded hover:bg-red-50\">Remove</button>"
                "</li>";
    }
    sqlite3_finalize(sched_stmt);

    if (!list.empty())
        html += "<ul class=\"mt-4 border-t border-gray-100 pt-3 space-y-1\">" + list + "</ul>";

    return html;
  };

  svr.Get("/user-mealplans", [db, build_mealplan_html](const httplib::Request& req, httplib::Response& res) {
    int recipe_id = req.has_param("recipe_id") ? std::stoi(req.get_param_value("recipe_id")) : -1;
    res.set_content(build_mealplan_html(db, recipe_id), "text/html");
  });

  svr.Get("/mealplan-slot", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan_id") || !req.has_param("weekday") || !req.has_param("meal_time")) {
        res.set_content("", "text/html");
        return;
    }
    int plan_id = std::stoi(req.get_param_value("plan_id"));
    int weekday = std::stoi(req.get_param_value("weekday"));
    std::string meal_time = req.get_param_value("meal_time");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, R"(
        SELECT r.title
        FROM mealplan_schedule ms
        JOIN recipe r ON ms.recipe_id = r.recipe_id
        WHERE ms.plan_id = ? AND ms.day = ? AND ms.meal_type = ?
    )", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, plan_id);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_text(stmt, 3, meal_time.c_str(), -1, SQLITE_TRANSIENT);

    std::string html;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        html += "<p class=\"text-xs text-amber-600 bg-amber-50 rounded px-2 py-1\">"
                "&#9432; " + std::string(title ? title : "") + " is already scheduled here</p>";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

  svr.Post("/add-to-mealplan", [db, build_mealplan_html](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan_id") || !req.has_param("recipe_id")) { res.status = 400; return; }
    int plan_id   = std::stoi(req.get_param_value("plan_id"));
    int recipe_id = std::stoi(req.get_param_value("recipe_id"));
    int weekday   = std::stoi(req.get_param_value("weekday"));
    std::string meal_time = req.get_param_value("meal_time");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO mealplan_schedule (plan_id, recipe_id, day, meal_type) VALUES (?, ?, ?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, plan_id);
    sqlite3_bind_int(stmt, 2, recipe_id);
    sqlite3_bind_int(stmt, 3, weekday);
    sqlite3_bind_text(stmt, 4, meal_time.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    res.set_content(build_mealplan_html(db, recipe_id), "text/html");
  });

  svr.Post("/remove-from-mealplan", [db, build_mealplan_html](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("schedule_id") || !req.has_param("recipe_id")) { res.status = 400; return; }
    int schedule_id = std::stoi(req.get_param_value("schedule_id"));
    int recipe_id   = std::stoi(req.get_param_value("recipe_id"));

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "DELETE FROM mealplan_schedule WHERE schedule_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, schedule_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    res.set_content(build_mealplan_html(db, recipe_id), "text/html");
  });

}
