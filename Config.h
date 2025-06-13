#pragma once

// Miscellaneous configuration

// If CONFIG_SENSOR_BME_BMP280 defined, use BME/BMP280 (vs SPL06/BME)
#ifdef ARDUINO_AVR_LEONARDO_ETH
#define CONFIG_SENSOR_BME_BMP280
#else
// prototype
//#define CONFIG_SENSOR_BME_BMP280
#endif

// If defined, display Celcius (vs Fahrenheit)
#define CONFIG_CELCIUS

// If defined, display hPa (vs inHg)
#define CONFIG_HECTO_PASCALS

// Height above MSL in meters. If defined, used to adjust the air pressure reading to sea level
//#define CONFIG_ALTITUDE_METERS    7

// If defined, display a random forecast (dithered) if none is available
#define RANDOM_FORECAST_IF_NONE

// If defined, dither the pressure, temperature & humidity units
#define DITHER_UNITS

// If defined, add folded page corner
#define FOLD_CORNER

// If defined, display demo values, see Weather::Loop()
//#define DEMO

// If defined, Serial is opened, no splash, refresh counter and forecast letter are shown (tiny)
//#define DEBUG
