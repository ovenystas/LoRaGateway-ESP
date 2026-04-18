#pragma once

#include <stdint.h>

#include "DeviceClass.h"

class NumberDeviceClass : public DeviceClass {
 public:
  // From https://www.home-assistant.io/integrations/number/ at 2026-03-15
  enum class Type : uint8_t {
    NONE,  // Generic number. This is the default and doesn’t need to be set.
    ABSOLUTE_HUMIDITY,  // Absolute humidity in g/m³, mg/m³.
    APPARENT_POWER,     // Apparent power in mVA, VA or kVA.
    AQI,                // Air Quality Index (unitless).
    AREA,               // Area in m², cm², km², mm², in², ft², yd², mi², ac, ha
    ATMOSPHERIC_PRESSURE,  // Atmospheric pressure in cbar, bar, hPa, mmHg,
                           // inHg, kPa, mbar, Pa or psi
    BATTERY,               // Percentage of battery that is left in %
    BLOOD_GLUCOSE_CONCENTRATION,  // Blood glucose concentration in mg/dL,
                                  // mmol/L
    CARBON_DIOXIDE,               // Carbon Dioxide (CO₂) concentration in ppm
    CARBON_MONOXIDE,  // Carbon Monoxide (CO) concentration in ppb, ppm, µg/m³,
                      // mg/m³
    CURRENT,          // Current in A, mA
    DATA_RATE,  // Data rate in bit/s, kbit/s, Mbit/s, Gbit/s, B/s, kB/s, MB/s,
                // GB/s, KiB/s, MiB/s or GiB/s
    DATA_SIZE,  // Data size in bit, kbit, Mbit, Gbit, B, kB, MB, GB, TB, PB,
                // EB, ZB, YB, KiB, MiB, GiB, TiB, PiB, EiB, ZiB or YiB
    DISTANCE,   // Generic distance in km, m, cm, mm, mi, nmi, yd, or in
    DURATION,   // Duration in d, h, min, s, ms, or µs
    ENERGY,  // Energy in J, kJ, MJ, GJ, mWh, Wh, kWh, MWh, GWh, TWh, cal, kcal,
             // Mcal, or Gcal
    ENERGY_DISTANCE,  // Energy per distance in kWh/100km, Wh/km, mi/kWh, or
                      // km/kWh.
    ENERGY_STORAGE,   // Stored energy in J, kJ, MJ, GJ, mWh, Wh, kWh, MWh, GWh,
                      // TWh, cal, kcal, Mcal, or Gcal
    FREQUENCY,        // Frequency in Hz, kHz, MHz, or GHz
    GAS,              // Gas volume in L, m³, ft³, CCF, or MCF
    HUMIDITY,         // Percentage of humidity in the air in %
    ILLUMINANCE,      // The current light level in lx
    IRRADIANCE,       // Irradiance in W/m² or BTU/(h⋅ft²)
    MOISTURE,         // Percentage of water in a substance in %
    MONETARY,         // The monetary value (ISO 4217)
    NITROGEN_DIOXIDE,   // Concentration of Nitrogen Dioxide in ppb, ppm, µg/m³
    NITROGEN_MONOXIDE,  // Concentration of Nitrogen Monoxide in ppb, µg/m³
    NITROUS_OXIDE,      // Concentration of Nitrous Oxide in µg/m³
    OZONE,              // Concentration of Ozone in ppb, ppm, or µg/m³
    PH,                 // Potential hydrogen (pH) value of a water solution
    PM1,  // Concentration of particulate matter less than 1 micrometer in µg/m³
    PM25,  // Concentration of particulate matter less than 2.5 micrometers in
           // µg/m³
    PM4,   // Concentration of particulate matter less than 4 micrometers in
           // µg/m³
    PM10,  // Concentration of particulate matter less than 10 micrometers in
           // µg/m³
    POWER_FACTOR,             // Power factor (unitless), unit may be None or %
    POWER,                    // Power in mW, W, kW, MW, GW or TW
    PRECIPITATION,            // Accumulated precipitation in cm, in or mm
    PRECIPITATION_INTENSITY,  // Precipitation intensity in in/d, in/h, mm/d or
                              // mm/h
    PRESSURE,  // Pressure in mPa, Pa, hPa, kPa, bar, cbar, mbar, mmHg, inHg,
               // inH₂O or psi
    REACTIVE_ENERGY,  // Reactive energy in varh or kvarh
    REACTIVE_POWER,   // Reactive power in mvar, var, or kvar
    SIGNAL_STRENGTH,  // Signal strength in dB or dBm
    SOUND_PRESSURE,   // Sound pressure in dB or dBA
    SPEED,  // Generic speed in ft/s, in/d, in/h, in/s, km/h, kn, m/s, mph,
            // mm/d, or mm/s
    SULPHUR_DIOXIDE,    // Concentration of sulphur dioxide in ppb, µg/m³
    TEMPERATURE,        // Temperature in °C, °F or K
    TEMPERATURE_DELTA,  // Temperature representing a difference between two
                        // values in °C, °F or K
    VOLATILE_ORGANIC_COMPOUNDS,  // Concentration of volatile organic compounds
                                 // in µg/m³ or mg/m³
    VOLATILE_ORGANIC_COMPOUNDS_PARTS,  // Ratio of volatile organic compounds in
                                       // ppm or ppb
    VOLTAGE,                           // Voltage in V, mV, µV, kV, MV
    VOLUME,  // Generic volume in L, mL, gal, fl. oz., m³, ft³, CCF, or MCF
    VOLUME_FLOW_RATE,  // Volume flow rate in m³/h, m³/min, m³/s, ft³/min, L/h,
                       // L/min, L/s, gal/d, gal/h, gal/min, or mL/s
    VOLUME_STORAGE,    // Generic stored volume in L, mL, gal, fl. oz., m³, ft³,
                       // CCF, or MCF
    WATER,             // Water consumption in L, gal, m³, ft³, CCF, or MCF
    WEIGHT,            // Generic mass in kg, g, mg, µg, oz, lb, or st
    WIND_DIRECTION,    // Wind direction in °
    WIND_SPEED,        // Wind speed in Beaufort, ft/s, km/h, kn, m/s, or mph
  };

  NumberDeviceClass() : deviceClass(Type::NONE) {}
  NumberDeviceClass(Type d) : deviceClass(d) {}
  NumberDeviceClass(uint8_t d) : deviceClass(static_cast<Type>(d)) {}

  const char* getName() const override;
  Type getDeviceClass() const { return deviceClass; }
  uint8_t toByte() const override { return static_cast<uint8_t>(deviceClass); }
  DeviceClass* clone() const override { return new NumberDeviceClass(*this); }

 private:
  Type deviceClass;
};
