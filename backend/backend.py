from fastapi import FastAPI
from pydantic import BaseModel
from fastapi.middleware.cors import CORSMiddleware



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


def get_locations(): #update to use db lol
    return [{'name':'Location A', 'emergency': False},{'name':'Location B', 'emergency':True},{'name':'Location C', 'emergency':True},{'name':'Location D', 'emergency':True},{'name':'Location E', 'emergency':True}]




@app.get('/')
async def root():
    return {'status':'ok','message':'Backend api running'}

@app.get('/locations')
async def locations():
    return get_locations()

@app.post('/reset')
async def reset_location(request: ResetRequest):
    print(request.location)
