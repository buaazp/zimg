#! /bin/bash
curl -F "blob=@test.jpg;type=image/jpeg" "http://127.0.0.1:8080/upload"
