import requests
from requests.auth import HTTPBasicAuth

def printResponse(r):
	print("==STATUSCODE==")
	print(r.status_code)
	print("==HEADERS==")
	print(r.headers)
	print("==BODY==")
	print(r.text)
	print("******************\n")

r = requests.post('http://localhost:8080/php', data={'login':'admin', 'pass':'password'})
printResponse(r)