# Backend 
Run all from 
``` 
backend/
```
## Python 
Install dependencies: 
```bash 
pip install -r requirements.txt
```

Run the server 
```bash 
uvicorn backend:app --reload
```
## DB
Run server and attach/connect to saved data. 
```bash
docker run -d \
  --name resqpi-db \
  -p 8123:8123 \
  -p 9000:9000 \
  -e CLICKHOUSE_PASSWORD=resqpi123 \
  --ulimit nofile=262144:262144 \
  -v "$(pwd)/resqpi_data:/var/lib/clickhouse" \
  clickhouse/clickhouse-server
``` 

Initialize the DB
```bash
python3 init_db.py
```
Initialize DB with all locations having an emergency
```bash
python3 init_db.py --emergency
```
