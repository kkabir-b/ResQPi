from fastapi import FastAPI

app = FastAPI(title = "ResQPi backend api", version = "1.0.0")


def get_locations(): #update to use db lol
    return [{'name':'Location A', 'emergency': False},{'name':'Location B', 'emergency':True}]




@app.get('/')
async def root():
    return {'status':'ok','message':'Backend api running'}

@app.get('/getLocations')
async def getLocations():
    return get_locations
