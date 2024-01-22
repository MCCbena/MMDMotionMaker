import json

import requests


def get_motions():
    motion_data = []

    first = True
    view_counter = 0
    while True:
        query = {
            "q": "https://bowlroll.net/",
            "targets": "description",
            "_sort": "+viewCounter",
            "fields": "description,title,contentId,viewCounter",
            "_limit": 100,
            "filters[viewCounter][gte]": view_counter + 1
        }
        url = "https://api.search.nicovideo.jp/api/v2/snapshot/video/contents/search"

        response = requests.get(url, params=query)

        if len(response.json()["data"]) == 0:
            return motion_data
        print(f"再生回数:{view_counter}〜")

        for i in response.json()["data"]:
            title = i["title"]
            content_id = i["contentId"]
            description = i["description"]
            view_counter = i["viewCounter"]

            if "モーション" not in title:
                continue
            print(title, end=":")
            start = description.find("https://bowlroll.net/")
            if start == -1:
                continue

            print(description[start:start+32])

            motion_data.append({
                "MovieName": title,
                "MovieURL": f"https://nico.ms/{content_id}",
                "Motion": description[start:start+32]
            })


content = json.dumps(get_motions())
with open("motions.json", "w") as f:
    f.write(content)
