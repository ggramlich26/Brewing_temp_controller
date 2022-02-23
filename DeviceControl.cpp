/*
 * DeviceControl.cpp
 *
 *  Created on: Aug 23, 2020
 *      Author: tsugua
 */

#include "DeviceControl.h"

DeviceControl* DeviceControl::_instance = NULL;


DeviceControl::DeviceControl() {
	init();
}

DeviceControl::~DeviceControl() {
}

/// initializes all pins and interrupts etc. necessary
void DeviceControl::init(){
	Out1PeriodStartTime = 0;
	Out1Level = 0;
	pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
	pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
	pinMode(OUT1_PIN, OUTPUT);
	buttonUp = new Button([](){return DeviceControl::instance()->readButtonUp();});
	buttonDown = new Button([](){return DeviceControl::instance()->readButtonDown();});

    Out1Off();

    Serial.println("init oneWire");
	temp1OneWire = new OneWire(TEMP1_PIN);
	temp2OneWire = new OneWire(TEMP2_PIN);
	temp3OneWire = new OneWire(TEMP3_PIN);
	temp1 = new DallasTemperature(temp1OneWire);
	temp2 = new DallasTemperature(temp2OneWire);
	temp3 = new DallasTemperature(temp3OneWire);
	temp1value = 9999;
	temp2value = 9999;
	temp3value = 9999;
	Serial.println("begin oneWire1");
	temp1->begin();
	if(temp1->getAddress(temp1Addr, 0)){
		temp1->setResolution(temp1Addr, TEMP_RESOLUTION);
		temp1Connected = true;
		temp1->setWaitForConversion(false);
	}
	else{
		temp1Connected = false;
	}
	Serial.println("begin oneWire2");
	temp2->begin();
	if(temp2->getAddress(temp2Addr, 0)){
		temp2->setResolution(temp2Addr, TEMP_RESOLUTION);
		temp2Connected = true;
		temp2->setWaitForConversion(false);
	}
	else{
		temp2Connected = false;
	}
	Serial.println("begin oneWire3");
	temp3->begin();
	if(temp3->getAddress(temp3Addr, 0)){
		temp3->setResolution(temp3Addr, TEMP_RESOLUTION);
		temp3Connected = true;
		temp3->setWaitForConversion(false);
	}
	else{
		temp3Connected = false;
	}
}

/// update routine, that should be called regularly. Some values are only updated through this routine.
void DeviceControl::update(){
	//update SSR
	if(Out1Level == 0){
		Out1Off();
	}
	else if(Out1Level == 100){
		Out1On();
	}
	else{
		if(millis() >= Out1PeriodStartTime + SSR_PERIOD_TIME){
			Out1PeriodStartTime = millis();
			Out1On();
		}
		else if(millis() < Out1PeriodStartTime + Out1Level/100.0*SSR_PERIOD_TIME){
			Out1On();
		}
		else{
			Out1Off();
		}
	}
	//update buttons
	buttonUp->update();
	buttonDown->update();

	//update temperature
	static unsigned long lastTempUpdateTime = 0;
	if(millis() >= lastTempUpdateTime + TEMP_UPDATE_INTERVAL){
		lastTempUpdateTime = millis();
		if(temp1Connected){
			temp1value = temp1->getTempC(temp1Addr);
			temp1->requestTemperaturesByAddress(temp1Addr);
		}
		if(temp2Connected)
			temp2value = temp2->getTempC(temp2Addr);
		if(temp3Connected)
			temp3value = temp3->getTempC(temp3Addr);
	}
}

double DeviceControl::getPIDInputForOut1(){
	return getTemp1();
}

/// enables the boiler heater
// @param level: heater level between 0 (off) and 100 (full power)
void DeviceControl::enableOut1(int level){
	if(level < 0) level = 0;
	else if (level > 100) level = 100;
	Out1Level = level;
}

void DeviceControl::disableOut1(){
	Out1Level = 0;
	Out1Off();
}

bool DeviceControl::readButtonUp(){
	return digitalRead(BUTTON_UP_PIN);
}

bool DeviceControl::readButtonDown(){
	return digitalRead(BUTTON_DOWN_PIN);
}

/***
 * returns the debounced button 1 state
 */
bool DeviceControl::getButtonUpState(){
	return buttonUp->isPressed();
}

/***
 * returns the debounced button 2 state
 */
bool DeviceControl::getButtonDownState(){
	return buttonDown->isPressed();
}


/*** "Stupid" function for triggering the boiler heater SSR
 * @param send: if true and boiler heater had not been turned on before,
 * 				MCP23017 will be updated instantly
 */
void DeviceControl::Out1On(){
#ifdef OUT1_LOW_LEVEL_TRIGGER
	digitalWrite(OUT1_PIN, LOW);
#else
	digitalWrite(OUT1_PIN, HIGH);
#endif
}

/*** "Stupid" function for triggering the boiler heater SSR
 * @param send: if true and boiler heater had been turned on before,
 * 				MCP23017 will be updated instantly
 */
void DeviceControl::Out1Off(){
#ifdef BOILER_HEATER_LOW_LEVEL_TRIGGER
	digitalWrite(OUT1_PIN, HIGH);
#else
	digitalWrite(OUT1_PIN, LOW);
#endif
}

