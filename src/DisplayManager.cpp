/*
 * DisplayManager.cpp
 *
 *  Created on: 28.10.2021
 *      Author: ag4716
 */

#include "DisplayManager.h"
#include "WIFI_config.h"
#include <cstddef>

DisplayManager* DisplayManager::_instance;

DisplayManager::DisplayManager() {
	display = NULL;
}

DisplayManager::~DisplayManager() {
	if(display != NULL)
		delete(display);
}

void DisplayManager::init(int sda, int scl){
	display = new SSD1306Wire(0x3C, sda, scl);
	display->init();
	display->flipScreenVertically();
	display->clear();
	display->setFont(DISP_FONT);
	dev = DeviceControl::instance();
}

void DisplayManager::update(){
	if(updateDisplay || millis() > lastUpdateTime + MAX_UPDATE_INTERVAL){
		lastUpdateTime = millis();
		display->clear();
		displayIP();
		displayTemps();
		displayPower();
		display->display();
	}
}

void DisplayManager::setPowerLevel(double level){
	if(level < 0)
		level = 0;
	if(level > 1)
		level = 1;
	if(level!= powerLevel)
		updateDisplay = true;
	powerLevel = level;
}

void DisplayManager::updateSetupMode(){
	if(updateDisplay || millis() > lastUpdateTime + MAX_UPDATE_INTERVAL){
		Serial.println("update display setup mode");
		lastUpdateTime = millis();
		display->clear();
		display->setTextAlignment(TEXT_ALIGN_LEFT);
		display->drawString(0, LINE_0_ORIG, "Setup Mode:");
		display->drawString(0, LINE_1_ORIG, DEFAULT_SSID);
		display->drawString(0, LINE_2_ORIG, "192.168.4.1");
		display->display();
	}
}
void DisplayManager::valuesChanged(){
	updateDisplay = true;
}

void DisplayManager::displayTemps(){
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0, LINE_1_ORIG, String("Set: "));
	display->drawString(0, LINE_2_ORIG, String("T1: "));
	display->drawString(0, LINE_3_ORIG, String("T2: "));
	int temp = dev->getTemp1();
	display->drawString(30, LINE_1_ORIG, String((int)(DataManager::getOut1Setpoint())) + " �C");
	display->drawString(30, LINE_2_ORIG, dev->getTemp1Connected()?(String(temp) + " �C"):"NC");
	temp = dev->getTemp2();
	display->drawString(30, LINE_3_ORIG, dev->getTemp2Connected()?(String(temp) + " �C"):"NC");
}

void DisplayManager::displayPower(){
	int width = 20;
	int height = 40;
	int x_bottom = 127-width/2;
	display->setColor(WHITE);
	display->drawTriangle(x_bottom, 63, x_bottom-width/2, 63-height, x_bottom+width/2, 63-height);
	int height_level = height*powerLevel;
	if(height_level>0){
		int width_level = (double)width/height*height_level;
		display->fillTriangle(x_bottom, 63, x_bottom-width_level/2, 63-height_level,
				x_bottom+width_level/2, 63-height_level);
	}
}

void DisplayManager::displayTriangle(int line){
	if(line < 0 || line > 3)
		return;
	display->fillTriangle(0,line*16+5, 0,line*16+11, 6,line*16+8);
}

void DisplayManager::displayIP(){
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	if(DataManager::getInternetConnected())
		display->drawString(127, LINE_0_ORIG, DataManager::getIPAddress());
	else
		display->drawString(127, LINE_0_ORIG, "disconnected");
}
