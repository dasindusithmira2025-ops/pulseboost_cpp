import urllib.request


request = urllib.request.Request("http://127.0.0.1:8000/")
with urllib.request.urlopen(request, timeout=10) as response:
    print(response.status)
    body = response.read(200).decode("utf-8", errors="replace")
    print(body)
