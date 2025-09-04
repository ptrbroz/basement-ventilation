
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- measurements sent from the device: temp, rhum, plus absolute huminidity derived server-side
-- floats (mentioned measured quantities) rounded to 2 decimal places, multiplied by 100 and stored as integers
CREATE TABLE IF NOT EXISTS weather (
    time_device TIMESTAMPTZ NOT NULL, --part of msg
    dtime_server_ms INTEGER NOT NULL, --calculated on receive
    temp_in INTEGER, -- following are part of msg
    temp_out INTEGER, 
    rhum_in INTEGER,
    rhum_out INTEGER,
    ahum_in INTEGER, --calculated server-side
    ahum_out INTEGER, --calculated server-side
    fan_state INTEGER --TODO: document values
);

SELECT create_hypertable('weather', 'time_device');

