from ingredient_parser import parse_ingredient

def parse(raw: str) -> dict:
    result = parse_ingredient(raw)

    name = result.name[0].text if result.name else None
    size = result.size.text if result.size else None
    quantity = float(result.amount[0].quantity) if result.amount else None
    unit = str(result.amount[0].unit) if result.amount else None
    preparation = result.preparation.text if result.preparation else None
    comment = result.comment.text if result.comment else None

    return {
        "name": name,
        "size": size,
        "quantity": quantity,
        "unit": unit,
        "preparation": preparation,
        "comment": comment,
    }

if __name__ == "__main__":
    test = "8 ounces extra-large shrimp (21 to 25 per pound), peeled, deveined, and tails removed"
    print(parse(test))
