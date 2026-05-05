import pandas as pd
import sqlite3
from recipe_scrapers import scrape_me
import cloudscraper
from bs4 import BeautifulSoup
import json
import argparse

database_path = './../data/recipes.db'
data_path = './../data/dump/meal_dump.json'

# database_path = 'recipes.db'
# data_path = 'meal_dump.json'
website = 'https://www.americastestkitchen.com'


def create_jason():
    print("creating scraper")
    scraper = cloudscraper.create_scraper()
    
    print("Fetchine Page")
    page = scraper.get("https://www.americastestkitchen.com/recipes/all?p=32", timeout=32)
    print(f"Got Page, {page.status_code}")
    
    soup = BeautifulSoup(page.content, 'html.parser')

    link_frame = soup.find('ul', class_="AlgoliaResults_grid__NXdqe")
    print("Found link frame")
    link_container = link_frame.find_all('li', class_="StandardCard_li__kJWEc")
    print(f"Found {len(link_container)} items")
    links = [website + container.find('a', class_="StandardCardV2-module_imgLink__XQsZr")['href'] for container in link_container]
    print(f"links: {links}")
    
    all_recipies = []

    for link in links:
        try:
            print(f"Attempting to scrape {link}")
            scraper = scrape_me(link)
            print("Got Page")
            all_recipies.append(scraper.to_json())
        except Exception as e:
            print(f"Failed: {link} - {e}")
    with open("meal_dump.json", 'w', encoding='utf-8') as file:
        json.dump(all_recipies, file, indent=4)

def create_tables(con:sqlite3.Connection, cur:sqlite3.Cursor):
    cur.execute('PRAGMA foreign_keys = ON')

    cur.executescript('''
        CREATE TABLE IF NOT EXISTS user (
        user_id INTEGER PRIMARY KEY AUTOINCREMENT,
        username VARCHAR(30) NOT NULL UNIQUE,
        password VARCHAR(30) NOT NULL,
        email VARCHAR(30) NOT NULL UNIQUE,
        active_plan_id INTEGER,
        FOREIGN KEY (active_plan_id) REFERENCES mealplan(plan_id)
    );

    CREATE TABLE IF NOT EXISTS recipe (
        recipe_id INTEGER PRIMARY KEY AUTOINCREMENT,
        title VARCHAR(255),
        author VARCHAR(30),
        url VARCHAR(255),
        category VARCHAR(255),
        cuisine VARCHAR(255),
        description TEXT,
        image_url VARCHAR(255),
        total_time INTEGER,
        yields VARCHAR(30),
        ratings FLOAT,
        ratings_count INTEGER
    );

    CREATE TABLE IF NOT EXISTS ingredients (
        ingredient_id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        size TEXT
    );

    CREATE TABLE IF NOT EXISTS recipe_ingredients (
        recipe_id INTEGER,
        ingredient_id INTEGER,
        quantity DECIMAL(8,2),
        unit VARCHAR(30),
        preparation TEXT,
        comment TEXT,
        section TEXT,
        PRIMARY KEY (recipe_id, ingredient_id),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id),
        FOREIGN KEY (ingredient_id) REFERENCES ingredients(ingredient_id)
    );

    CREATE TABLE IF NOT EXISTS ingredient_group (
        group_id INTEGER PRIMARY KEY AUTOINCREMENT,
        recipe_id INTEGER,
        purpose VARCHAR(255),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS ingredient_group_items (
        group_id INTEGER,
        ingredient_id INTEGER,
        PRIMARY KEY (group_id, ingredient_id),
        FOREIGN KEY (group_id) REFERENCES ingredient_group(group_id),
        FOREIGN KEY (ingredient_id) REFERENCES ingredients(ingredient_id)
    );

    CREATE TABLE IF NOT EXISTS instructions (
        instruction_id INTEGER PRIMARY KEY AUTOINCREMENT,
        recipe_id INTEGER,
        step INTEGER,
        section TEXT,
        text TEXT,
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS nutrients (
        recipe_id INTEGER PRIMARY KEY,
        calories TEXT,
        protein TEXT,
        carbs TEXT,
        fats TEXT,
        fiber TEXT,
        sodium TEXT,
        cholesterol TEXT,
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS keywords (
        keyword_id INTEGER PRIMARY KEY AUTOINCREMENT,
        name VARCHAR(30) UNIQUE
    );

    CREATE TABLE IF NOT EXISTS recipe_keywords (
        recipe_id INTEGER,
        keyword_id INTEGER,
        PRIMARY KEY (recipe_id, keyword_id),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id),
        FOREIGN KEY (keyword_id) REFERENCES keywords(keyword_id)
    );

    CREATE TABLE IF NOT EXISTS mealplan (
        plan_id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER,
        name TEXT NOT NULL,
        created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (user_id) REFERENCES user(user_id)
    );

    CREATE TABLE IF NOT EXISTS mealplan_recipe (
        plan_id INTEGER,
        recipe_id INTEGER,
        PRIMARY KEY (plan_id, recipe_id),
        FOREIGN KEY (plan_id) REFERENCES mealplan(plan_id),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS mealplan_schedule (
        schedule_id INTEGER PRIMARY KEY AUTOINCREMENT,
        plan_id INTEGER,
        recipe_id INTEGER,
        day INTEGER,
        meal_type TEXT,
        FOREIGN KEY (plan_id) REFERENCES mealplan(plan_id),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS user_likes (
        user_id INTEGER,
        recipe_id INTEGER,
        liked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (user_id, recipe_id),
        FOREIGN KEY (user_id) REFERENCES user(user_id),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS collection (
        collection_id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER,
        name TEXT NOT NULL,
        description TEXT,
        created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (user_id) REFERENCES user(user_id)
    );

    CREATE TABLE IF NOT EXISTS collection_recipe (
        collection_id INTEGER,
        recipe_id INTEGER,
        PRIMARY KEY (collection_id, recipe_id),
        FOREIGN KEY (collection_id) REFERENCES collection(collection_id),
        FOREIGN KEY (recipe_id) REFERENCES recipe(recipe_id)
    );

    CREATE TABLE IF NOT EXISTS grocery_list (
        list_id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER,
        generated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (user_id) REFERENCES user(user_id)
    );

    CREATE TABLE IF NOT EXISTS grocery_list_items (
        list_id INTEGER,
        ingredient_id INTEGER,
        total_quantity DECIMAL(8,2),
        unit VARCHAR(30),
        checked BOOLEAN DEFAULT 0,
        PRIMARY KEY (list_id, ingredient_id),
        FOREIGN KEY (list_id) REFERENCES grocery_list(list_id),
        FOREIGN KEY (ingredient_id) REFERENCES ingredients(ingredient_id)
    );
    ''')

    con.commit()

def parse_nutrient(value):
    if value is None:
        return None
    # strips " g" or any non-numeric characters, returns int
    return int(''.join(filter(str.isdigit, str(value)))) or None

def import_recipe(cur, r):
    # Insert recipe
    cur.execute('''
        INSERT INTO recipe (title, author, url, category, cuisine, description, image_url, total_time, yields, ratings, ratings_count)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ''', (
        r.get('title'),
        r.get('author'),
        r.get('canonical_url'),
        r.get('category'),
        r.get('cuisine'),
        r.get('description'),
        r.get('image'),
        r.get('total_time'),
        r.get('yields'),
        r.get('ratings'),
        r.get('ratings_count')
    ))
    recipe_id = cur.lastrowid

    # Insert ingredients
    for ingredient in r.get('ingredients', []):
        cur.execute('INSERT OR IGNORE INTO ingredients (name) VALUES (?)', (ingredient,))
        cur.execute('SELECT ingredient_id FROM ingredients WHERE name = ?', (ingredient,))
        ingredient_id = cur.fetchone()[0]
        cur.execute('INSERT OR IGNORE INTO recipe_ingredients (recipe_id, ingredient_id) VALUES (?, ?)', (recipe_id, ingredient_id))

    # Insert instructions
    for step, text in enumerate(r.get('instructions_list', []), start=1):
        cur.execute('INSERT INTO instructions (recipe_id, step, text) VALUES (?, ?, ?)', (recipe_id, step, text))

    # Insert keywords
    for keyword in r.get('keywords', []):
        cur.execute('INSERT OR IGNORE INTO keywords (name) VALUES (?)', (keyword,))
        cur.execute('SELECT keyword_id FROM keywords WHERE name = ?', (keyword,))
        keyword_id = cur.fetchone()[0]
        cur.execute('INSERT OR IGNORE INTO recipe_keywords (recipe_id, keyword_id) VALUES (?, ?)', (recipe_id, keyword_id))

    # Insert nutrients
    nutrients = r.get('nutrients')
    if nutrients:
        cur.execute('''
            INSERT OR IGNORE INTO nutrients (recipe_id, calories, protein, carbs, fats, fiber, sodium, cholesterol)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            recipe_id,
            parse_nutrient(nutrients.get('calories')),
            parse_nutrient(nutrients.get('proteinContent')),
            parse_nutrient(nutrients.get('carbohydrateContent')),
            parse_nutrient(nutrients.get('fatContent')),
            parse_nutrient(nutrients.get('fiberContent')),
            parse_nutrient(nutrients.get('sodiumContent')),
            parse_nutrient(nutrients.get('cholesterolContent'))
        ))

    return recipe_id

def import_data(con:sqlite3.Connection, cur:sqlite3.Cursor):
    with open(data_path) as f:
        recipes = json.load(f)
    
    seen = set()
    unique_recipes = []
    for r in recipes:
        url = r.get('canonical_url')
        if url not in seen:
            seen.add(url)
            unique_recipes.append(r)

    print(f"Importing {len(unique_recipes)} unique recipes")

    for r in unique_recipes:
        import_recipe(cur, r)

    con.commit()
    
def main():
    parser = argparse.ArgumentParser(description="Recipe scraper")
    parser.add_argument("--scrape", action="store_true", help="indicates whether or not it will re-scrape the webpage")
    args = parser.parse_args()
    
    if args.scrape:
        create_jason()
        
    con = sqlite3.connect(database_path)
    cur = con.cursor()
    
    create_tables(con, cur)
    import_data(con, cur)
    
    cur.execute("INSERT INTO user (username, password, email) VALUES (?, ?, ?)", ('test', 'test', 'test@email.com'))


if __name__ == "__main__":
    main()