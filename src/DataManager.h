/*
 * DataManager.h
 *
 *  Created on: Aug 20, 2020
 *      Author: tsugua
 */

#ifndef DATAMANAGER_H_
#define DATAMANAGER_H_

#include "Arduino.h"
#include "DeviceControl.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define DEFAULT_OUT_1_SETPOINT	20
#define	MAX_OUT_1_SETPOINT		125
#define	MIN_OUT_1_SETPOINT		5

#define	DEFAULT_PID_P			70
#define	MAX_PID_P				100
#define	MIN_PID_P				0.00
#define DEFAULT_PID_I			0.0
#define MAX_PID_I				100
#define MIN_PID_I				0
#define DEFAULT_PID_D			2
#define MAX_PID_D				100
#define MIN_PID_D				0
#define DEFAULT_MQTT_UPDATE_INT	5000
#define MIN_MQTT_UPDATE_INT		500
#define DEFAULT_MQTT_SERVER		"192.168.101.20"
#define DEFAULT_MQTT_PUBLISH_TOPIC "Status"
#define DEFAULT_MQTT_SUBSCRIBE_TOPIC "Set"
#define DEFAULT_MQTT_CLIENT_ID	"Braukessel"
#define DEFAULT_MQTT_PASSWORD	""
#define MQTT_MASTER_STRING	"BTC"

#define WIFI_CONNECT_INTERVAL	10000	//interval in which the device tries to connect to wifi


class DataManager {
public:
	static bool getInternetConnected();
	static String getIPAddress();
	static double getOut1P();
	static void setOut1P(double p, bool send);
	static double getOut1I();
	static void setOut1I(double i, bool send);
	static double getOut1D();
	static void setOut1D(double d, bool send);

	static double getOut1Setpoint();
	static void setOut1Setpoint(double setPoint, bool send);

	static unsigned long getMqttUpdateInterval();
	static void setMqttUpdateInterval(unsigned long interval, bool send);
	static bool getMqttEnabled();
	static void setMqttEnabled(bool enabled);

	static void mqttPublishTemp(float temp, int sensorNumber);
	static void mqttPublishSensorError(bool error, int sensorNumber);

	static bool getWifiConnected();

	static String setWIFICredentials(const char* newSSID, const char* newPassword, const char* newHostName);

	static void init();
	static void update();
private:
	DataManager(){}
	virtual ~DataManager(){}

	static DeviceControl *dev;

	static double out1Setpoint;

	static double out1P;
	static double out1I;
	static double out1D;

	static unsigned long mqttUpdateInterval;
	static uint8_t mqttEnabled;
	static String mqttPublishTrunc;
	static String mqttSubscribeTrunc;

	static uint32_t calculateWIFIChecksum();
	static void eepromWrite(uint8_t *src, int addr, int len, bool commit);
	static void eepromRead(uint8_t *dst, int addr, int len);
	static void WIFISetupMode();
	static void startConfigPortal(bool blocking);
	static void configPortalCallback();
	static void saveParamsCallback();
	static void pubSubCallback(char* topic, byte* payload, unsigned int length);
	static void pubSubReconnect();

	static bool scheduleRestart;
	static WiFiManager *wifiManager;
	static PubSubClient *pubSubClient;

	static unsigned long lastWifiConnectTryTime;
};
#endif /* DATAMANAGER_H_ */

