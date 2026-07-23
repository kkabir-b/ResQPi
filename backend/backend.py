from fastapi import FastAPI
from pydantic import BaseModel
import clickhouse_connect
from fastapi.middleware.cors import CORSMiddleware
from aiokafka import AIOKafkaConsumer
import asyncio
from contextlib import asynccontextmanager

KAFKA_SERVER = 'localhost:9092'
KAFKA_TOPIC = "detection-events"

clickhouse_client = clickhouse_connect.get_client(
    host = 'localhost',
    port = 8123,
    username = 'default',
    password ='resqpi123'
)


latest_detection = {
    "status": "idle",
    "location": None
}


class ResetRequest(BaseModel):
    location: str


async def kafka_consumer_loop():
    consumer = AIOKafkaConsumer(
        KAFKA_TOPIC,
        bootstrap_servers=KAFKA_SERVER,
        group_id="resqpi-backend-group",
        auto_offset_reset="latest"
    )
    
    await consumer.start()
    print(f"[FastAPI] Listening to Redpanda topic: '{KAFKA_TOPIC}'")
    
    try:
        async for msg in consumer:
            
            location_name = msg.value.decode("utf-8").strip().strip('"')
            print(f"[Kafka Received] Emergency event for location: '{location_name}'")
            
           
            latest_detection["status"] = "emergency"
            latest_detection["location"] = location_name
            
            # Update ClickHouse table setting emergency = true
            update_query = """
                ALTER TABLE resqpi_db.locations 
                UPDATE emergency = true 
                WHERE location = {loc:String}
            """
            
            
            await asyncio.to_thread(
                clickhouse_client.command, 
                update_query, 
                parameters={'loc': location_name}
            )
            print(f"[ClickHouse] Emergency set to TRUE for '{location_name}'")
            
    except asyncio.CancelledError:
        print("[FastAPI] Stopping Kafka consumer...")
    finally:
        await consumer.stop()

@asynccontextmanager
async def lifespan(app: FastAPI):
    consumer_task = asyncio.create_task(kafka_consumer_loop())
    yield
    consumer_task.cancel()


app = FastAPI(title = "ResQPi backend api", version = "1.0.0",lifespan=lifespan)


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
