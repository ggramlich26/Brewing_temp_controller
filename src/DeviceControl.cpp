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
	Serial.println("initialize buttons");
	Serial.flush();
	pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
	pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
	Serial.println("initialize buttons done");
	Serial.flush();
	pinMode(OUT1_PIN, OUTPUT);
	buttonUp = new Button([](){return DeviceControl::instance()->readButtonUp();});
	buttonDown = new Button([](){return DeviceControl::instance()->readButtonDown();});

    Out1Off();

    Serial.println("init oneWire");
	Serial.flush();
	temp1OneWire = new OneWire(TEMP1_PIN);
	temp2OneWire = new OneWire(TEMP2_PIN);
	temp3OneWire = new OneWire(TEMP3_PIN);
	temp1 = new DallasTemperature(temp1OneWire);
	temp2 = new DallasTemperature(temp2OneWire);
	temp3 = new DallasTemperature(temp3OneWire);
	temp1value = 0;
	temp2value = 0;
	temp3value = 0;
	temp1ConsecutiveErrors = 0;
	temp2ConsecutiveErrors = 0;
	temp3ConsecutiveErrors = 0;
	Serial.println("begin oneWire1");
	temp1->begin();
	Serial.println("begin oneWire2");
	temp2->begin();
	Serial.println("begin oneWire3");
	temp3->begin();
	temp1Connected = false;
	temp2Connected = false;
	temp3Connected = false;
	connectTemperatureSensors();
}

/// update routine, that should be called regularly. Some values are only updated through this routine.
void DeviceControl::update(){
	//update SSR
	if(Out1Level == 0){
		Out1Off();
	}
	// completely on for a little below 100 as well to avoid short relay clicks
	// with 60s SSR_PERIOD_TIME this yields a minimum off time of 5s
	else if(Out1Level >= 92){
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
			double temp = temp1->getTempC(temp1Addr);
			if(temp < -20 || temp > 150){
				temp1ConsecutiveErrors++;
				// try again in 100ms
				lastTempUpdateTime -= (TEMP_UPDATE_INTERVAL - 100);
				if(temp1ConsecutiveErrors >= 10){
					temp1Connected = false;
				}
			}
			else{
				temp1value = temp;
				temp1ConsecutiveErrors = 0;
			}
			temp1->requestTemperaturesByAddress(temp1Addr);
		}
		if(temp2Connected){
			double temp = temp2->getTempC(temp2Addr);
			if(temp < -20 || temp > 150){
				temp2ConsecutiveErrors++;
				if(temp2ConsecutiveErrors >= 10){
					temp2Connected = false;
				}
			}
			else{
				temp2value = temp;
				temp2ConsecutiveErrors = 0;
			}
			temp2->requestTemperaturesByAddress(temp2Addr);
		}
		if(temp3Connected){
			double temp = temp2->getTempC(temp3Addr);
			if(temp < -20 || temp > 150){
				temp3ConsecutiveErrors++;
				if(temp3ConsecutiveErrors >= 10){
					temp3Connected = false;
				}
			}
			else{
				temp3value = temp;
				temp3ConsecutiveErrors = 0;
			}
			temp3->requestTemperaturesByAddress(temp3Addr);
		}
		connectTemperatureSensors();
	}
}

void DeviceControl::connectTemperatureSensors(){
	if(!temp1Connected){
		temp1->begin();
		if(temp1->getAddress(temp1Addr, 0)){
			temp1->setResolution(temp1Addr, TEMP_RESOLUTION);
			temp1Connected = true;
			temp1->setWaitForConversion(false);
			temp1ConsecutiveErrors = 0;
			temp1->requestTemperaturesByAddress(temp1Addr);
		}
	}
	if(!temp2Connected){
		temp2->begin();
		if(temp2->getAddress(temp2Addr, 0)){
			temp2->setResolution(temp2Addr, TEMP_RESOLUTION);
			temp2Connected = true;
			temp2->setWaitForConversion(false);
			temp2ConsecutiveErrors = 0;
			temp2->requestTemperaturesByAddress(temp2Addr);
		}
	}
	if(!temp3Connected){
		temp3->begin();
		if(temp3->getAddress(temp3Addr, 0)){
			temp3->setResolution(temp3Addr, TEMP_RESOLUTION);
			temp3Connected = true;
			temp3->setWaitForConversion(false);
			temp3ConsecutiveErrors = 0;
			temp3->requestTemperaturesByAddress(temp3Addr);
		}
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

