import math

import logging
logger = logging.getLogger(__name__)

def estimate_absolute_humidity(t : float, rhumPercent : float):
    """
    Estimates ahum [g/m3] from temperature [°C] and relative humidity using a LUT. 
    This function replicates the functionality in the embedded device to avoid sending extra data to backend. Inaccuracies are an intended result of replication.
    """

    # temp -> ahum LUT, starting at T = 10°C. Each step of lut is a delta of 1°C
    LUT =           [9.4, 10.01, 10.66, 11.343, 12.065, 12.826, 13.63, 14.474, 15.365, 16.303, 
                    17.29, 18.328, 19.42, 20.566, 21.77, 23.037, 24.364, 25.76, 27.22, 28.75, 
                    30.355, 32.036, 33.796, 35.64, 37.566, 39.58, 41.69, 43.89, 46.19, 48.59,
                    51.1]

    LUT0 = 10 # LUT[0] corresponds to this temperature 

    # interpolate LUT vals
    i0 = int(math.floor(t-LUT0))
    i1 = int(math.ceil(t-LUT0))

    interpolatedMaxAbsHum = None

    if i0 < 0:
        interpolatedMaxAbsHum = LUT[0]
    elif i1 >= len(LUT):
        interpolatedMaxAbsHum = LUT[len(LUT) - 1]
    else:
        w0 = t - math.floor(t)
        w1 = 1.0 - w0
        interpolatedMaxAbsHum = w1*LUT[i0] + w0*LUT[i1]

    absHum = (rhumPercent / 100.0) * interpolatedMaxAbsHum
    return absHum


if __name__ == "__main__":
    for i in range(0,51,5):
        rH = 100.0
        print(f"T = {i}, rH = {rH}  -> aH = {estimate_absolute_humidity(i,rH)}")
