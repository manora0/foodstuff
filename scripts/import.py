import pandas as pd
import sqlite3
from recipe_scrapers import scrape_html

database_path = './../../data/recipes.db'
data_path = './../../data/dump/meal_dump.json'



con = sqlite3.connect(database_path)
cur = con.cursor()