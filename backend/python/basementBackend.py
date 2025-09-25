
# Don't import stuff from other files above this block!
import logging
from systemd.journal import JournalHandler 
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
formatter = logging.Formatter("[%(levelname)s] %(message)s")
handler = JournalHandler()
handler.setFormatter(formatter)
logger.addHandler(handler)

from tasks import start_scheduler

import time


if __name__ == "__main__":
    logger.info("Main entry")
    #start_mqtt()       # start MQTT in background
    start_scheduler()  # start periodic tasks
    while 1:
        time.sleep(10)
    
