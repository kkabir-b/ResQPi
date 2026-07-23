from fastapi import FastAPI
from pydantic import BaseModel
import clickhouse_connect
from fastapi.middleware.cors import CORSMiddleware

clickhouse_client = clickhouse_connect.get_client(
    host = 'localhost',
    port = 8123,
    username = 'default',
    password ='resqpi123'
)


class ResetRequest(BaseModel):
    location: str

app = FastAPI(title = "ResQPi backend api", version = "1.0.0")


app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def get_locations():
    # Fetch rows and alias 'location' to 'name'
    query = "SELECT location AS name, emergency FROM resqpi_db.locations ORDER BY location"
    result = clickhouse_client.query(query)
    
    return [
        dict(zip(result.column_names, row)) 
        for row in result.result_rows
    ]




@app.get('/')
async def root():
    return {'status':'ok','message':'Backend api running'}

@app.get('/locations')
async def locations():
    return get_locations()

@app.post('/reset')
async def reset_location(request: ResetRequest):
    query = """
        ALTER TABLE resqpi_db.locations 
        UPDATE emergency = false 
        WHERE location = {loc:String}
    """
    
    # Execute the mutation using parameterized inputs to prevent SQL injection
    clickhouse_client.command(query, parameters={'loc': request.location})
    
    return {
        "status": "success", 
        "location": request.location, 
        "emergency": False
    }
