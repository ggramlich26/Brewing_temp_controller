/*
 * DataManager.cpp
 *
 *  Created on: Aug 21, 2020
 *      Author: tsugua
 */

#include "DataManager.h"
#include "DeviceControl.h"
#include "DisplayManager.h"
#include "WIFI_config.h"

//#include <WiFiClient.h>

#include "Arduino.h"
#include "EEPROM.h"
#include <stdint.h>


double DataManager::out1Setpoint;
double DataManager::out1P;
double DataManager::out1I;
double DataManager::out1D;
unsigned long DataManager::mqttUpdateInterval;
uint8_t DataManager::mqttEnabled;
unsigned long DataManager::lastWifiConnectTryTime;
DeviceControl* DataManager::dev;
bool DataManager::scheduleRestart;
WiFiManager *DataManager::wifiManager;
PubSubClient *DataManager::pubSubClient;
String DataManager::mqttPublishTrunc;
String DataManager::mqttSubscribeTrunc;

//	Flash address parameters
#define SSID_MAX_LEN				30
#define SSID_ADDR					0
#define WIFI_PW_MAX_LEN				90
#define WIFI_PW_ADDR				31
#define	HOST_NAME_MAX_LEN			30
#define	HOST_NAME_ADDR				122
#define	OUT_1_P_ADDR				153
#define OUT_1_P_LEN					sizeof(double)
#define	OUT_1_I_ADDR				161
#define OUT_1_I_LEN					sizeof(double)
#define	OUT_1_D_ADDR				169
#define	OUT_1_D_LEN					sizeof(double)
#define MQTT_SERVER_ADDR			177
#define MQTT_SERVER_MAX_LEN			30
#define MQTT_PUBLISH_TOPIC_ADDR		208
#define MQTT_PUBLISH_TOPIC_MAX_LEN	30
#define MQTT_SUBSCRIBE_TOPIC_ADDR	239
#define MQTT_SUBSCRIBE_TOPIC_MAX_LEN	30
#define MQTT_CLIENT_ID_ADDR			270
#define MQTT_CLIENT_ID_MAX_LEN		30
#define MQTT_UPDATE_INT_ADDR		301
#define MQTT_UPDATE_INT_LEN			sizeof(unsigned long)
#define MQTT_ENABLED_ADDR			309
#define MQTT_ENABLED_LEN			sizeof(uint8_t)
#define MQTT_PW_ADDR				317
#define MQTT_PW_MAX_LEN				60


#define CHECKSUM_ADDR				400
#define	CHECKSUM_LEN				4
#define	EEPROM_SIZE					450
char ssid[SSID_MAX_LEN+1];
char password[WIFI_PW_MAX_LEN+1];
char hostName[HOST_NAME_MAX_LEN+1];
char mqttServer[MQTT_SERVER_MAX_LEN+1];
char mqttPublishTopic[MQTT_PUBLISH_TOPIC_MAX_LEN+1];
char mqttSubscribeTopic[MQTT_SUBSCRIBE_TOPIC_MAX_LEN+1];
char mqttClientId[MQTT_CLIENT_ID_MAX_LEN+1];
char mqttPassword[MQTT_PW_MAX_LEN+1];

WiFiClient espClient;


void DataManager::init(){
	dev = DeviceControl::instance();
	lastWifiConnectTryTime = 0;
	scheduleRestart = false;

	//read from flash/blynk (update each other) or use default values
	EEPROM.begin(EEPROM_SIZE);

	//read SSID and password from flash
	for(uint8_t i = 0; i < SSID_MAX_LEN+1; i++){
		ssid[i] = EEPROM.read(SSID_ADDR+i);
	}
	for(uint8_t i = 0; i < WIFI_PW_MAX_LEN+1; i++){
		password[i] = EEPROM.read(WIFI_PW_ADDR+i);
	}
	for(uint8_t i = 0; i < HOST_NAME_MAX_LEN+1; i++){
		hostName[i] = EEPROM.read(HOST_NAME_ADDR+i);
	}
	uint32_t checksum = 0;
	for(uint8_t i = 0; i < CHECKSUM_LEN; i++){
		checksum |= ((uint32_t)EEPROM.read(CHECKSUM_ADDR+i))<<(8*i);
	}

	//read MQTT related data
	for(uint8_t i = 0; i < MQTT_SERVER_MAX_LEN+1; i++){
		mqttServer[i] = EEPROM.read(MQTT_SERVER_ADDR+i);
	}
	for(uint8_t i = 0; i < MQTT_PUBLISH_TOPIC_MAX_LEN+1; i++){
		mqttPublishTopic[i] = EEPROM.read(MQTT_PUBLISH_TOPIC_ADDR+i);
	}
	for(uint8_t i = 0; i < MQTT_SUBSCRIBE_TOPIC_MAX_LEN+1; i++){
		mqttSubscribeTopic[i] = EEPROM.read(MQTT_SUBSCRIBE_TOPIC_ADDR+i);
	}
	for(uint8_t i = 0; i < MQTT_CLIENT_ID_MAX_LEN+1; i++){
		mqttClientId[i] = EEPROM.read(MQTT_CLIENT_ID_ADDR+i);
	}
	for(uint8_t i = 0; i < MQTT_PW_MAX_LEN+1; i++){
		mqttPassword[i] = EEPROM.read(MQTT_PW_ADDR+i);
	}

	eepromRead((uint8_t*)&out1P, OUT_1_P_ADDR, OUT_1_P_LEN);
	eepromRead((uint8_t*)&out1I, OUT_1_I_ADDR, OUT_1_I_LEN);
	eepromRead((uint8_t*)&out1D, OUT_1_D_ADDR, OUT_1_D_LEN);
	eepromRead((uint8_t*)&mqttUpdateInterval, MQTT_UPDATE_INT_ADDR, MQTT_UPDATE_INT_LEN);
	eepromRead((uint8_t*)&mqttEnabled, MQTT_ENABLED_ADDR, MQTT_ENABLED_LEN);

	wifiManager = new WiFiManager();
	//enter wifi setup mode
	if(!dev->readButtonUp() && !dev->readButtonDown()){
		Serial.println("enter setup mode");
		WIFISetupMode();
	}
	//if wifi not intialized correctly, use default values
	else if(checksum != calculateWIFIChecksum()){
		//copy default SSID to EEPROM and to the ssid variable
		const char* default_ssid = DEFAULT_SSID;
		for(uint8_t i = 0; i < SSID_MAX_LEN+1; i++){
			EEPROM.write(SSID_ADDR + i, default_ssid[i]);
			ssid[i] = default_ssid[i];
			if(default_ssid[i] == '\0'){
				break;
			}
		}
		//copy default password to EEPROM and to the password variable
		const char* default_pw = DEFAULT_PASSWORD;
		for(uint8_t i = 0; i < WIFI_PW_MAX_LEN+1; i++){
			EEPROM.write(WIFI_PW_ADDR + i, default_pw[i]);
			password[i] = default_pw[i];
			if(default_pw[i] == '\0'){
				break;
			}
		}
		//copy default host name to EEPROM and to the hostName variable
		const char* default_host_name = DEFAULT_HOST_NAME;
		for(uint8_t i = 0; i < HOST_NAME_MAX_LEN+1; i++){
			EEPROM.write(HOST_NAME_ADDR + i, default_host_name[i]);
			hostName[i] = default_host_name[i];
			if(default_host_name[i] == '\0'){
				break;
			}
		}
		checksum = calculateWIFIChecksum();
		for(uint8_t i = 0; i < CHECKSUM_LEN; i++){
			EEPROM.write(CHECKSUM_ADDR+i, (uint8_t)(checksum>>(8*i)));
		}

		//default MQTT values
		//copy default MQTT server to EEPROM and to the mqttServer variable
		const char* default_val = DEFAULT_MQTT_SERVER;
		for(uint8_t i = 0; i < MQTT_SERVER_MAX_LEN+1; i++){
			EEPROM.write(MQTT_SERVER_ADDR + i, default_val[i]);
			mqttServer[i] = default_val[i];
			if(default_val[i] == '\0'){
				break;
			}
		}
		//copy default MQTT publish topic to EEPROM and to the mqttPublishTopic variable
		default_val = DEFAULT_MQTT_PUBLISH_TOPIC;
		for(uint8_t i = 0; i < MQTT_PUBLISH_TOPIC_MAX_LEN+1; i++){
			EEPROM.write(MQTT_PUBLISH_TOPIC_ADDR + i, default_val[i]);
			mqttPublishTopic[i] = default_val[i];
			if(default_val[i] == '\0'){
				break;
			}
		}
		//copy default MQTT subscribe topic to EEPROM and to the mqttSubscribeTopic variable
		default_val = DEFAULT_MQTT_SUBSCRIBE_TOPIC;
		for(uint8_t i = 0; i < MQTT_SUBSCRIBE_TOPIC_MAX_LEN+1; i++){
			EEPROM.write(MQTT_SUBSCRIBE_TOPIC_ADDR + i, default_val[i]);
			mqttSubscribeTopic[i] = default_val[i];
			if(default_val[i] == '\0'){
				break;
			}
		}
		//copy default MQTT client id to EEPROM and to the mqttClientId variable
		default_val = DEFAULT_MQTT_CLIENT_ID;
		for(uint8_t i = 0; i < MQTT_CLIENT_ID_MAX_LEN+1; i++){
			EEPROM.write(MQTT_CLIENT_ID_ADDR + i, default_val[i]);
			mqttClientId[i] = default_val[i];
			if(default_val[i] == '\0'){
				break;
			}
		}
		//copy default MQTT password to EEPROM and to the mqttPassword variable
		default_val = DEFAULT_MQTT_PASSWORD;
		for(uint8_t i = 0; i < MQTT_PW_MAX_LEN+1; i++){
			EEPROM.write(MQTT_PW_ADDR + i, default_val[i]);
			mqttPassword[i] = default_val[i];
			if(default_val[i] == '\0'){
				break;
			}
		}

		EEPROM.commit();
	}
	//if EEPROM not initialized yet, write default values
	if(isnan(out1P) || isnan(out1I) || isnan(out1D) || isnan(mqttUpdateInterval) ||
			out1P < MIN_PID_P || out1I < MIN_PID_I || out1D < MIN_PID_D ||
			out1P > MAX_PID_P || out1I > MAX_PID_I || out1D > MAX_PID_D ||
			mqttUpdateInterval < MIN_MQTT_UPDATE_INT){
		Serial.println("Writing default values");
		out1P = DEFAULT_PID_P;
		out1I = DEFAULT_PID_I;
		out1D = DEFAULT_PID_D;
		mqttUpdateInterval = DEFAULT_MQTT_UPDATE_INT;
		mqttEnabled = 0;
		eepromWrite((uint8_t*)&out1P, OUT_1_P_ADDR, OUT_1_P_LEN, false);
		eepromWrite((uint8_t*)&out1I, OUT_1_I_ADDR, OUT_1_I_LEN, false);
		eepromWrite((uint8_t*)&out1D, OUT_1_D_ADDR, OUT_1_D_LEN, false);
		eepromWrite((uint8_t*)&mqttUpdateInterval, MQTT_UPDATE_INT_ADDR, MQTT_UPDATE_INT_LEN, false);
		eepromWrite((uint8_t*)&mqttEnabled, MQTT_ENABLED_ADDR, MQTT_ENABLED_LEN, false);
		EEPROM.commit();
	}
	out1Setpoint = DEFAULT_OUT_1_SETPOINT;
	mqttSubscribeTrunc = String(MQTT_MASTER_STRING) + "/" + String(mqttClientId) + "/ "
			+ String(mqttSubscribeTopic) + "/";
	mqttPublishTrunc = String(MQTT_MASTER_STRING) + "/" + String(mqttClientId) + "/"
			+ String(mqttPublishTrunc) + "/";

	//WiFi.setHostname(hostName);
	WiFi.begin(ssid, password);
	startConfigPortal(false);
	pubSubClient = new PubSubClient(espClient);
	pubSubClient->setServer(mqttServer, 1883);
	pubSubClient->setCallback(pubSubCallback);
	lastWifiConnectTryTime = millis();
	delay(500);
}

/// standard update routine
void DataManager::update(){
	if(mqttEnabled && !WiFi.isConnected() && millis() >= lastWifiConnectTryTime + WIFI_CONNECT_INTERVAL){
		WiFi.begin(ssid, password);
		lastWifiConnectTryTime = millis();
	}
	wifiManager->process();
	static unsigned long lastMqttReconnectTime = 0;
	if(getMqttEnabled() && WiFi.isConnected() && !pubSubClient->connected()
			&& millis() >= lastMqttReconnectTime + 5000){
		pubSubReconnect();
		lastMqttReconnectTime = millis();
	}
	if(getMqttEnabled() && pubSubClient->connected()){
		pubSubClient->loop();
	}
}

double DataManager::getOut1Setpoint(){
	return out1Setpoint;
}

// saves a new target temperature for the boiler
// @param temp: new target temperature
// @param updateBlynk: updates the target temperature in blynk, if set to true and blynk is enabled
void DataManager::setOut1Setpoint(double setPoint, bool send){
	if(setPoint > MAX_OUT_1_SETPOINT)
		setPoint = MAX_OUT_1_SETPOINT;
	else if(setPoint < MIN_OUT_1_SETPOINT)
		setPoint = MIN_OUT_1_SETPOINT;
	if(out1Setpoint == setPoint){
		return;
	}

	out1Setpoint = setPoint;
	//update remote
	if(send && mqttEnabled && WiFi.isConnected() && pubSubClient->connected())
		pubSubClient->publish((mqttSubscribeTrunc+"setpoint").c_str(), String(setPoint).c_str());
}

double DataManager::getOut1P(){
	return out1P;
}

/// saves a new boiler controller P parameter
// @param p: the new parameter
// @param send: sends the new parameter via MQTT
void DataManager::setOut1P(double p, bool send){
	if(p < MIN_PID_P) p = MIN_PID_P;
	else if(p > MAX_PID_P) p = MAX_PID_P;
	if(out1P == p) return;
	out1P = p;
	eepromWrite((uint8_t*)&out1P, OUT_1_P_ADDR, OUT_1_P_LEN, true);
	//update remote
	if(send && mqttEnabled && WiFi.isConnected() && pubSubClient->connected())
		pubSubClient->publish((mqttSubscribeTrunc+"P").c_str(), String(p).c_str());
}

double DataManager::getOut1I(){
	return out1I;
}

/// saves a new boiler controller I parameter
// @param i: the new parametep
// @param send: sends the new parameter via MQTT
void DataManager::setOut1I(double i, bool send){
	if(i < MIN_PID_I) i = MIN_PID_I;
	else if(i > MAX_PID_I) i = MAX_PID_I;
	if(out1I == i) return;
	out1I = i;
	eepromWrite((uint8_t*)&out1I, OUT_1_I_ADDR, OUT_1_I_LEN, true);
	//update remote
	if(send && mqttEnabled && WiFi.isConnected() && pubSubClient->connected())
		pubSubClient->publish((mqttSubscribeTrunc+"I").c_str(), String(i).c_str());
}

double DataManager::getOut1D(){
	return out1D;
}

/// saves a new boiler controller D parameter
// @param d: the new parameter
// @param send: sends the new parameter via MQTT
void DataManager::setOut1D(double d, bool send){
	if(d < MIN_PID_D) d = MIN_PID_D;
	else if(d > MAX_PID_D) d = MAX_PID_D;
	if(out1D == d) return;
	out1D = d;
	eepromWrite((uint8_t*)&out1D, OUT_1_D_ADDR, OUT_1_D_LEN, true);
	//update remote
	if(send && mqttEnabled && WiFi.isConnected() && pubSubClient->connected())
		pubSubClient->publish((mqttSubscribeTrunc+"D").c_str(), String(d).c_str());
}


bool DataManager::getWifiConnected(){
	return WiFi.isConnected();
}

unsigned long DataManager::getMqttUpdateInterval(){
	return mqttUpdateInterval;
}

void DataManager::setMqttUpdateInterval(unsigned long interval, bool send){
	if(interval < MIN_MQTT_UPDATE_INT)
		interval = DEFAULT_MQTT_UPDATE_INT;
	if(mqttUpdateInterval == interval)
		return;
	mqttUpdateInterval = interval;
	if(send && mqttEnabled && WiFi.isConnected() && pubSubClient->connected())
		pubSubClient->publish((mqttSubscribeTrunc+"updateInterval").c_str(), String(interval).c_str());
}

void DataManager::mqttPublishTemp(float temp, int sensorNumber){
	if(sensorNumber < 1 || sensorNumber > 3)
		return;
	if(mqttEnabled && WiFi.isConnected() && pubSubClient->connected())
		pubSubClient->publish((mqttPublishTrunc+"temp"+sensorNumber).c_str(), String(temp).c_str());
}
bool DataManager::getMqttEnabled(){
	return mqttEnabled != 0;
}

void DataManager::setMqttEnabled(bool enabled){
	if((mqttEnabled != 0) != enabled){
		uint8_t tmp = enabled==0?0x00:0xFF;
		eepromWrite((uint8_t*)&tmp, MQTT_ENABLED_ADDR, MQTT_ENABLED_LEN, true);
	}
}

bool DataManager::getInternetConnected(){
	return WiFi.isConnected();
}

String DataManager::getIPAddress(){
	return WiFi.localIP().toString();
}


//////////////////////////////////////////////////////////////////
//																//
//				EEPROM functions								//
//																//
//////////////////////////////////////////////////////////////////


/// used to make sure valid data is read from EEPROM
uint32_t DataManager::calculateWIFIChecksum(){
	uint32_t res = 0;
	for(uint8_t i = 0, j = 0; i < SSID_MAX_LEN+1; i++, j = (j+1)%CHECKSUM_LEN){
		res ^= ((uint32_t)(ssid[i])<<(8*j));
		if(ssid[i] == '\0'){
			break;
		}
	}
	for(uint8_t i = 0, j = 0; i < WIFI_PW_MAX_LEN+1; i++, j = (j+1)%CHECKSUM_LEN){
		res ^= ((uint32_t)(password[i])<<(8*j));
		if(password[i] == '\0'){
			break;
		}
	}
	for(uint8_t i = 0, j = 0; i < HOST_NAME_MAX_LEN+1; i++, j = (j+1)%CHECKSUM_LEN){
		res ^= ((uint32_t)(hostName[i])<<(8*j));
		if(hostName[i] == '\0'){
			break;
		}
	}
	return res;
}

/// reads data from the EEPROM
// @param dst: memory address, where the data read is to be stored
// @param addr: EEPROM start address
// @param len: length of the data to be read in bytes
void DataManager::eepromRead(uint8_t *dst, int addr, int len){
	if(len <= 0){
		return;
	}
	for(int i = 0; i < len; i++){
		*(dst++) = EEPROM.read(addr++);
	}
}

/// writes data to the EEPROM
// @param src: memory address, where the data to be written is to be stored
// @param addr: EEPROM start address
// @param len: length of the data to be written in bytes
// @param commit: will finish the EEPROM write with a commit, if true.
//					Only performing such a commit will make the data being actually written
void DataManager::eepromWrite(uint8_t *src, int addr, int len, bool commit){
	if(len < 0){
		return;
	}
	for(int i = 0; i < len; i++){
		EEPROM.write(addr++, *(src++));
	}
	if(commit){
		EEPROM.commit();
	}
}

void DataManager::pubSubReconnect(){
	Serial.println("Attempting MQTT connection...");
	// Attempt to connect
	if (pubSubClient->connect(mqttClientId)) {
		Serial.println("MQTT client connected");
		// ... and resubscribe
		pubSubClient->subscribe((mqttSubscribeTrunc+"setpoint").c_str());
		pubSubClient->publish((mqttSubscribeTrunc+"setpoint").c_str(), String((int)getOut1Setpoint()).c_str());
	}
}

void DataManager::pubSubCallback(char* topic, byte* payload, unsigned int length){
	if(String(topic).equals(mqttSubscribeTrunc+"setpoint")){
		int setpoint = atoi((const char*)payload);
		setOut1Setpoint(setpoint, false);
	}
	else if(String(topic).equals(mqttSubscribeTrunc+"P")){
		int p = atoi((const char*)payload);
		setOut1P(p, false);
	}
	else if(String(topic).equals(mqttSubscribeTrunc+"I")){
		int i = atoi((const char*)payload);
		setOut1I(i, false);
	}
	else if(String(topic).equals(mqttSubscribeTrunc+"D")){
		int d = atoi((const char*)payload);
		setOut1D(d, false);
	}
}

WiFiManagerParameter *custom_p;
WiFiManagerParameter *custom_i;
WiFiManagerParameter *custom_d;
WiFiManagerParameter *custom_setpoint;
WiFiManagerParameter *custom_mqtt_server;
WiFiManagerParameter *custom_mqtt_password;
WiFiManagerParameter *custom_mqtt_publish_topic;
WiFiManagerParameter *custom_mqtt_subscribe_topic;
WiFiManagerParameter *custom_mqtt_id;
WiFiManagerParameter *custom_mqtt_update_int;
WiFiManagerParameter *custom_mqtt_enabled;

void DataManager::configPortalCallback(){
	String newSSID = wifiManager->getWiFiSSID(true);
	String newPW = wifiManager->getWiFiPass(true);
	if((newSSID && !newSSID.equals("")) || (newPW && !newPW.equals("")))
		setWIFICredentials(newSSID.c_str(), newPW.c_str(), DEFAULT_HOST_NAME);
}

void DataManager::saveParamsCallback(){
	Serial.println("configPortalCallback");
	String newP = custom_p->getValue();
	if(newP && !newP.equals(""))
		setOut1P(newP.toFloat(), true);
	String newI = custom_i->getValue();
	if(newI && !newI.equals(""))
		setOut1I(newI.toFloat(), true);
	String newD = custom_d->getValue();
	if(newD && !newD.equals(""))
		setOut1D(newD.toFloat(), true);
	String newSetpoint = custom_setpoint->getValue();
	if(newSetpoint && !newSetpoint.equals(""))
		setOut1Setpoint(newSetpoint.toInt(), true);
	//mqtt parameters
	String newUpdateInterval = custom_mqtt_update_int->getValue();
	if(newUpdateInterval && !newUpdateInterval.equals(""))
		setMqttUpdateInterval(newUpdateInterval.toInt(), true);
	String newMqttEnabled = custom_mqtt_enabled->getValue();
	if(newMqttEnabled && !newMqttEnabled.equals("")){
		setMqttEnabled(!newMqttEnabled.equals("0"));
	}
	String newMQTTServer = custom_mqtt_server->getValue();
	if(newMQTTServer && !newMQTTServer.equals("") && !newMQTTServer.equals(mqttServer)){
		const char* temp = newMQTTServer.c_str();
		for(uint8_t i = 0; i < MQTT_SERVER_MAX_LEN+1; i++){
			EEPROM.write(MQTT_SERVER_ADDR + i, temp[i]);
			if(temp[i] == '\0')
				break;
		}
		EEPROM.commit();
		scheduleRestart = true;
	}
	String newMQTTPassword = custom_mqtt_password->getValue();
	if(newMQTTPassword && !newMQTTPassword.equals(mqttPassword)){
		const char* temp = newMQTTPassword.c_str();
		for(uint8_t i = 0; i < MQTT_PW_MAX_LEN+1; i++){
			EEPROM.write(MQTT_PW_ADDR + i, temp[i]);
			if(temp[i] == '\0')
				break;
		}
		EEPROM.commit();
		scheduleRestart = true;
	}
	String newPublishTopic = custom_mqtt_publish_topic->getValue();
	if(newPublishTopic && !newPublishTopic.equals("") && !newPublishTopic.equals(mqttPublishTopic)){
		const char* temp = newPublishTopic.c_str();
		for(uint8_t i = 0; i < MQTT_PUBLISH_TOPIC_MAX_LEN+1; i++){
			EEPROM.write(MQTT_PUBLISH_TOPIC_ADDR + i, temp[i]);
			if(temp[i] == '\0')
				break;
		}
		EEPROM.commit();
		scheduleRestart = true;
	}
	String newSubscribeTopic = custom_mqtt_subscribe_topic->getValue();
	if(newSubscribeTopic && !newSubscribeTopic.equals("") && !newSubscribeTopic.equals(mqttSubscribeTopic)){
		const char* temp = newSubscribeTopic.c_str();
		for(uint8_t i = 0; i < MQTT_SUBSCRIBE_TOPIC_MAX_LEN+1; i++){
			EEPROM.write(MQTT_SUBSCRIBE_TOPIC_ADDR + i, temp[i]);
			if(temp[i] == '\0')
				break;
		}
		EEPROM.commit();
		scheduleRestart = true;
	}
	String newMQTTId = custom_mqtt_id->getValue();
	if(newMQTTId && !newMQTTId.equals("") && !newMQTTId.equals(mqttClientId)){
		const char* temp = newMQTTId.c_str();
		for(uint8_t i = 0; i < MQTT_CLIENT_ID_MAX_LEN+1; i++){
			EEPROM.write(MQTT_CLIENT_ID_ADDR + i, temp[i]);
			if(temp[i] == '\0')
				break;
		}
		EEPROM.commit();
		scheduleRestart = true;
	}
}

void DataManager::startConfigPortal(bool blocking){
//	wifiManager->setConfigPortalTimeout(PORTALTIMEOUT);
	wifiManager->setSaveConfigCallback(configPortalCallback);
	wifiManager->setSaveParamsCallback(saveParamsCallback);
	wifiManager->setBreakAfterConfig(true);
	wifiManager->setParamsPage(true);
	custom_p = new WiFiManagerParameter("out1P", "P", String(getOut1P()).c_str(), 8);
	custom_i = new WiFiManagerParameter("out1I", "I", String(getOut1I()).c_str(), 8);
	custom_d = new WiFiManagerParameter("out1D", "D", String(getOut1D()).c_str(), 8);
	custom_setpoint = new WiFiManagerParameter("out1Setpoint", "Setpoint", String(getOut1Setpoint()).c_str(), 8);
	custom_mqtt_update_int = new WiFiManagerParameter("mqttUpdateInt", "MQTT update interval", String(getMqttUpdateInterval()).c_str(), 8);
	custom_mqtt_enabled = new WiFiManagerParameter("mqttEnabled", "MQTT enabled (0=off, 1=on)", getMqttEnabled()?"1":"0", 1);
	custom_mqtt_server = new WiFiManagerParameter("mqttServer", "MQTT server", mqttServer, MQTT_SERVER_MAX_LEN);
	custom_mqtt_password = new WiFiManagerParameter("mqttPassword", "MQTT password", mqttPassword, MQTT_PW_MAX_LEN);
	custom_mqtt_publish_topic = new WiFiManagerParameter("mqttPublishTopic", "MQTT publish topic", mqttPublishTopic, MQTT_PUBLISH_TOPIC_MAX_LEN);
	custom_mqtt_subscribe_topic = new WiFiManagerParameter("mqttSubscribeTopic", "MQTT subscribe topic", mqttSubscribeTopic, MQTT_SUBSCRIBE_TOPIC_MAX_LEN);
	custom_mqtt_id = new WiFiManagerParameter("mqttId", "MQTT device ID", mqttClientId, MQTT_CLIENT_ID_MAX_LEN);
	wifiManager->addParameter(custom_p);
	wifiManager->addParameter(custom_i);
	wifiManager->addParameter(custom_d);
	wifiManager->addParameter(custom_setpoint);
	wifiManager->addParameter(custom_mqtt_enabled);
	wifiManager->addParameter(custom_mqtt_update_int);
	wifiManager->addParameter(custom_mqtt_server);
	wifiManager->addParameter(custom_mqtt_password);
	wifiManager->addParameter(custom_mqtt_publish_topic);
	wifiManager->addParameter(custom_mqtt_subscribe_topic);
	wifiManager->addParameter(custom_mqtt_id);
	if(blocking){
		wifiManager->startConfigPortal(DEFAULT_SSID);
		scheduleRestart = true;
	}
	else{
		wifiManager->setConfigPortalBlocking(false);
		wifiManager->startWebPortal();
	}
}

/**
 * Enters wifi setup mode: machine will create an access point with default SSID and password.
 * On the machine IP address (192.168.4.1) will be a webserver running to set up the new WIFI credentials
 * No regular operation will be possible in this mode
 */
void DataManager::WIFISetupMode(){
//	WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
//	IPAddress IP = WiFi.softAPIP();
//	Serial.print("AP IP address: ");
//	Serial.println(IP);
//	webserver_init();

	delay(500);
	DisplayManager *display = DisplayManager::instance();
	display->updateSetupMode();
	startConfigPortal(true);
	while(1){
		if(scheduleRestart){
			scheduleRestart = false;
			delay(1000);
			ESP.restart();
		}
		delay(100);
	}
}

String DataManager::setWIFICredentials(const char* newSSID, const char* newPassword, const char* newHostName){
	if(newSSID != NULL && strcmp(newSSID, "") != 0){
		for(uint8_t i = 0; i < SSID_MAX_LEN+1; i++){
			EEPROM.write(SSID_ADDR + i, newSSID[i]);
			ssid[i] = newSSID[i];
			if(newSSID[i] == '\0'){
				break;
			}
		}
	}
	if(newPassword != NULL && strcmp(newPassword, "") != 0){
		for(uint8_t i = 0; i < WIFI_PW_MAX_LEN+1; i++){
			EEPROM.write(WIFI_PW_ADDR + i, newPassword[i]);
			password[i] = newPassword[i];
			if(newPassword[i] == '\0'){
				break;
			}
		}
	}
	if(newHostName != NULL && strcmp(newHostName, "") != 0){
		for(uint8_t i = 0; i < HOST_NAME_MAX_LEN+1; i++){
			EEPROM.write(HOST_NAME_ADDR + i, newHostName[i]);
			hostName[i] = newHostName[i];
			if(newHostName[i] == '\0'){
				break;
			}
		}
	}
	uint32_t checksum = calculateWIFIChecksum();
	for(uint8_t i = 0; i < CHECKSUM_LEN; i++){
		EEPROM.write(CHECKSUM_ADDR+i, (uint8_t)(checksum>>(8*i)));
	}
	EEPROM.commit();
	scheduleRestart = true;
	return "Successfully set new WIFI credentials. You can change them again by restarting your machine while having both "
			"buttons pressed and the distribution switch set to manual distribution.";
}
