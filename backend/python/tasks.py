import requests
import os
import psycopg2
from dateutil import parser

import logging
logger = logging.getLogger(__name__)

from apscheduler.schedulers.background import BackgroundScheduler
from datetime import datetime

from utils import estimate_absolute_humidity


WSI_CHMU_KLEMENTINUM_STRING = "0-203-0-11514"




def fetch_CHMU_json(WSI = WSI_CHMU_KLEMENTINUM_STRING):
    "Returns json of current weather data from CHMU (10-minute increments, updated each hour)."
    now = datetime.now()
    dateString = now.strftime("%Y%m%d")
    preAddr = "https://opendata.chmi.cz/meteorology/climate/now/data/10m-"
    try:
        url = f"{preAddr}{WSI}-{dateString}.json"
        response = requests.get(url)
        json = response.json()
    except:
        logger.error(f"Could not obtain json from {url}. Response status {response.status_code}")
        return None
    return json

def get_latest_logged_timestamp():
    "Returns the latest timestamp from reference weather measurements database."
    try:
        dbConn = psycopg2.connect(
            dbname=os.getenv("DB_NAME"),
            user=os.getenv("DB_USER"),
            password=os.getenv("DB_PASSWORD"),
            host=os.getenv("DB_HOST", "localhost"),
            port=os.getenv("DB_PORT", "5432")
        )
    except:
        logger.error("Failed to connect to database.")
        return None

    try:
        cur = dbConn.cursor()
        cur.execute("""
            SELECT time
            FROM weather_reference
            ORDER BY time DESC
            LIMIT 1;
        """)
        latest_time = cur.fetchone()[0]
        cur.close()
        dbConn.close()
        return latest_time
    except:
        logger.error("Failed to get latest time from database.")
        return None

def task_update_reference_weather_databse():
    """Meant to run periodically once per hour. Downloads latest json with CHMU measurements (updated hourly) and saves new measurements to database."""

    latestTime = get_latest_logged_timestamp()
    json = fetch_CHMU_json()

    if json is None or latestTime is None:
        return  # something has failed, error already logged in functions

    #indices in CHMU :data
    STATION = 0 # disregard (const due to data source)
    ELEMENT = 1 # useful. T for temperature [°C], H for rel humidity [%]
    DT = 2      # useful. time of measurement
    VAL = 3     # useful. data
    FLAG = 4    # disregard (not used by temp or humidity measurements)

    ELEMENT_TEMP = "T"
    ELEMENT_RHUM = "H"

    data = json["data"]["data"]["values"] # this gets a list of sublists where each sublist uses indices as defined above

    """
    Following assumptions are made of the input data:
    1) timestamps for temperature and relative humidity match (ie. both T and H get measured at say 15:00, rather than one at 15:01 and the other at 15:02)
    2) measurements for a given sensor form chunks (ie. all measurements of temp first, then measurements of something else...)
    3) the timestamps within chunks are in ascending order
    4) chunks have same length
    """

    # find starts of chunks
    hChunkStart = None
    tChunkStart = None
    found = 0
    for i in range(0, len(data)):
        if data[i][ELEMENT] == ELEMENT_TEMP and tChunkStart == None:
            tChunkStart = i
            found += 1
        elif data[i][ELEMENT] == ELEMENT_RHUM and hChunkStart == None:
            hChunkStart = i
            found += 1
        if found == 2:
            break

    # find chunk length
    i = hChunkStart
    while(i<len(data) and data[i][ELEMENT] == ELEMENT_RHUM):
        i += 1
    chunkLen = i - hChunkStart

    logger.debug(f"Hchunk @ {hChunkStart}, Tchunk @ {tChunkStart}, chunklen {chunkLen}")

    # add entries to database if they are new
    try:
        dbConn = psycopg2.connect(
            dbname=os.getenv("DB_NAME"),
            user=os.getenv("DB_USER"),
            password=os.getenv("DB_PASSWORD"),
            host=os.getenv("DB_HOST", "localhost"),
            port=os.getenv("DB_PORT", "5432")
        )
        cur = dbConn.cursor()
    except:
        logger.error("Failed to connect to database.")
        return None

    # logger vars
    logFirstTime = None
    logLastTime = None
    logCounter = 0

    for i in range(chunkLen):
        thisTime = parser.parse(data[hChunkStart + i][DT])

        if thisTime <= latestTime:
            continue # skip since already in database

        logCounter += 1
        if logFirstTime is None:
            logFirstTime = thisTime
        else:
            logLastTime = thisTime
        
        thisT = data[tChunkStart + i][VAL]
        thisH = data[hChunkStart + i][VAL]
        thisAH = estimate_absolute_humidity(thisT, thisH)
        try:
            insertTuple = (thisTime, int(thisT*100), int(thisH*100), int(thisAH*100))
            cur.execute("INSERT INTO weather_reference (time, temp_out, rhum_out, ahum_out) VALUES (%s, %s, %s, %s)", insertTuple)
        except:
            logger.error(f"Failed to insert reference weather data tuple: {insertTuple}")
            return None

    dbConn.commit()
    cur.close()
    dbConn.close()

    logger.info(f"Performed {logCounter} ref weather insertions. Time range {logFirstTime} to {logLastTime}.")




def start_scheduler():
    logger.info("Starting scheduler.")
    scheduler = BackgroundScheduler()
    scheduler.add_job(task_update_reference_weather_databse, 'cron', hour='*', minute = 10)
    scheduler.start()



if __name__ == "__main__":
    print(task_update_reference_weather_databse())
