# IoT Safety Device with NodeMCU ESP8266

## Project Description

This project aims to create an IoT safety device using NodeMCU ESP8266 microcontroller, GPS module, and piezoelectric sensor. The device incorporates several key features to ensure user safety in various scenarios.

### Features:

1. **SOS Button**: The device is equipped with an emergency SOS button. When pressed, it triggers a message containing the current location of the user to a pre-defined emergency contact. This feature ensures quick assistance in case of emergencies.

2. **Geofence Monitoring**: The device continuously monitors the user's location using the GPS module. If the user goes beyond a predefined geofence area, an email notification is sent to alert the user or designated contacts. This helps in tracking the user's movements and ensures their safety within specified boundaries.

3. **Fall Detection**: Utilizing the piezoelectric sensor, the device can detect sudden falls or impacts. Upon detection, an alert is triggered to notify the user or emergency contacts. This feature is particularly useful for elderly individuals or those prone to accidents, ensuring prompt assistance in case of falls or incapacitation.

### Components Used:

- NodeMCU ESP8266: Acts as the main microcontroller and handles communication with other components.
- GPS Module: Provides accurate location tracking capabilities.
- Piezoelectric Sensor: Detects falls or sudden impacts.
- Additional components as required for power supply, connectivity, and housing.


### Circuit Diagram:

!(/iot-sos-device-circdiag.png)

### How to Use:

1. **Setup**: Connect the components according to the provided circuit diagram.
2. **Configure**: Set up the device with the necessary parameters such as emergency contact information and geofence boundaries.
3. **Activation**: Press the SOS button to trigger emergency alerts when needed.
4. **Monitoring**: Regularly monitor email notifications for geofence breaches and fall detection alerts.

### Future Improvements:

- Integration with mobile applications for real-time tracking and remote control.
- Enhanced data encryption and security measures for sensitive information transmission.
- Optimizations for power consumption to prolong battery life.

