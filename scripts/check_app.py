import time
import urllib.request


URLS = ("http://127.0.0.1:8000/healthz", "http://127.0.0.1:8000/")


deadline = time.time() + 90
while True:
    try:
        for url in URLS:
            with urllib.request.urlopen(url) as response:
                print(f"{url} {response.status}")
        break
    except Exception as exc:
        if time.time() >= deadline:
            raise
        time.sleep(3)
