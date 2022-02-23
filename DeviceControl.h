/*
 * DeviceControl.h
 *
 *  Created on: May 30, 2020
 *      Author: tsugua
 */

#ifndef DEVICECONTROL_H_
#define DEVICECONTROL_H_

#include "HW_config.h"
#include "Arduino.h"
#include "Button.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define SSR_PERIOD_TIME			5000
#define	BUTTON_DEPRELL_TIME		300
#define DIST_SWITCH_DEPRELL_TIME	100
#define	BUTTON_LONG_PRESS_TIME	3000
#define TEMP_UPDATE_INTERVAL	500

class DeviceControl {
public:
	static DeviceControl *instance(){
		if(!_instance)
			_instance = new DeviceControl();
		return _instance;
	}
	void update();

	double getPIDInputForOut1();

	void enableOut1(int level);
	void disableOut1();

	bool readButtonUp();
	bool readButtonDown();
	bool getButtonUpState();
	bool getButtonDownState();
	bool getButtonUpLongPressed(){return buttonUp->isLongPressed();}
	bool getButtonUpShortPressed(){return buttonUp->isShortPressed();}
	bool getButtonUpHeld(){return buttonUp->isHeld();}
	bool getButtonDownLongPressed(){return buttonDown->isLongPressed();}
	bool getButtonDownShortPressed(){return buttonDown->isShortPressed();}
	bool getButtonDownHeld(){return buttonDown->isHeld();}

	double getTemp1(){return temp1value;}
	double getTemp2(){return temp2value;}
	double getTemp3(){return temp3value;}
	bool getTemp1Connected(){return temp1Connected;}
	bool getTemp2Connected(){return temp2Connected;}
	bool getTemp3Connected(){return temp3Connected;}


private:
	static DeviceControl *_instance;
	DeviceControl();
	DeviceControl (const DeviceControl& );
	virtual ~DeviceControl();

	void init();
	void Out1On();
	void Out1Off();

	unsigned long Out1PeriodStartTime;
	int Out1Level;

	Button *buttonUp;
	Button *buttonDown;

	OneWire *temp1OneWire;
	OneWire *temp2OneWire;
	OneWire *temp3OneWire;
	DallasTemperature *temp1;
	DallasTemperature *temp2;
	DallasTemperature *temp3;
	bool temp1Connected;
	bool temp2Connected;
	bool temp3Connected;
	DeviceAddress temp1Addr;
	DeviceAddress temp2Addr;
	DeviceAddress temp3Addr;
	double temp1value;
	double temp2value;
	double temp3value;
};

#endif /* DEVICECONTROL_H_ */

