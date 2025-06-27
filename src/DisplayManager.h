/*
 * DisplayManager.h
 *
 *  Created on: 28.10.2021
 *      Author: ag4716
 */

#ifndef DISPLAYMANAGER_H_
#define DISPLAYMANAGER_H_
#include <Wire.h>
#include "SSD1306Wire.h"
#include "DataManager.h"
#include "DeviceControl.h"

#define		MAX_UPDATE_INTERVAL	500
#define		DISP_FONT	ArialMT_Plain_16
#define		LINE_0_ORIG			0
#define		LINE_1_ORIG			16
#define		LINE_2_ORIG			32
#define		LINE_3_ORIG			48

class DisplayManager {
public:
	static DisplayManager *instance(){
		if(!_instance)
			_instance = new DisplayManager();
		return _instance;
	}
	void init(int sda, int scl);
	void update();
	void setPowerLevel(double level);
	void updateSetupMode();
	void valuesChanged();
private:
	static DisplayManager *_instance;
	DisplayManager();
	DisplayManager (const DisplayManager& );
	virtual ~DisplayManager();
	SSD1306Wire *display;
	void displayIP();
	void displayTemps();
	void displayPower();
	void displayTriangle(int line);
	void displayMenu();
	DeviceControl *dev;
	double powerLevel;

	bool updateDisplay = false;
	unsigned long lastUpdateTime = 0;
};

#endif /* DISPLAYMANAGER_H_ */
