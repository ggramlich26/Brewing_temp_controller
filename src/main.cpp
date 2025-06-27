#include "Arduino.h"
#include "DeviceControl.h"
#include "DataManager.h"
#include "DisplayManager.h"
#include <PID_v1.h>


//////////////////////////////////////////////////////////////////////////////////////////
//										MQTT											//
//	Subscribed:																			//
//				setpoint																//
//				updateInterval															//
//				P																		//
//				I																		//
//				D																		//
//	Publish:																			//
//				temp1																	//
//				temp2																	//
//				temp3																	//
//				setpoint																//
//				updateInterval															//
//				P																		//
//				I																		//
//				D																		//
//																						//
//////////////////////////////////////////////////////////////////////////////////////////

DeviceControl* dev = NULL;
DisplayManager *displayManager = NULL;

//PID
double pid_input = 0;
double pid_output = 0;
double pid_setpoint = 25;
PID *pid = NULL;

void setup()
{
	Serial.begin(9600);
	Serial.println("Serial started");
	dev = DeviceControl::instance();
	displayManager = DisplayManager::instance();
	displayManager->init(DISPLAY_SDA, DISPLAY_SCL);
	DataManager::init();
	pid_input = dev->getPIDInputForOut1();
	pid = new PID(&pid_input, &pid_output, &pid_setpoint, DataManager::getOut1P(),
			DataManager::getOut1I(), DataManager::getOut1D(), DIRECT);
	pid->SetOutputLimits(0, 100);
	pid->SetMode(AUTOMATIC);
}

void loop()
{
	dev->update();
	DataManager::update();
	displayManager->update();
	//todo: display, temp sensor


	//update setpoint from buttons
	if(dev->getButtonUpShortPressed()){
		DataManager::setOut1Setpoint(DataManager::getOut1Setpoint()+1, true);
		displayManager->valuesChanged();
	}
	if(dev->getButtonDownShortPressed()){
		DataManager::setOut1Setpoint(DataManager::getOut1Setpoint()-1, true);
		displayManager->valuesChanged();
	}
	if(dev->getButtonUpHeld()){
		static unsigned long lastChangeTime=0;
		if(millis() >= lastChangeTime + 500){
			lastChangeTime = millis();
			DataManager::setOut1Setpoint(DataManager::getOut1Setpoint()+5, true);
			displayManager->valuesChanged();
		}
	}
	if(dev->getButtonDownHeld()){
		static unsigned long lastChangeTime=0;
		if(millis() >= lastChangeTime + 500){
			lastChangeTime = millis();
			DataManager::setOut1Setpoint(DataManager::getOut1Setpoint()-5, true);
			displayManager->valuesChanged();
		}
	}

	//update PID
	pid_input = dev->getPIDInputForOut1();
	pid_setpoint = DataManager::getOut1Setpoint();
	pid->SetTunings(DataManager::getOut1P(), DataManager::getOut1I(),
			DataManager::getOut1D());
	pid->Compute();
	// turn off heater if connection to temperature sensor lost
	if(!dev->getTemp1Connected()){
		pid_output = 0;
	}
	dev->enableOut1(pid_output);
	displayManager->setPowerLevel(pid_output/100.0);

	//transmit temperature to MQTT server
	static unsigned long lastTempTransmitTime = 0;
	if(millis()>lastTempTransmitTime+DataManager::getMqttUpdateInterval()){
		lastTempTransmitTime = millis();
		if(dev->getTemp1Connected())
			DataManager::mqttPublishTemp(dev->getTemp1(), 1);
		if(dev->getTemp2Connected())
			DataManager::mqttPublishTemp(dev->getTemp2(), 2);
		if(dev->getTemp3Connected())
			DataManager::mqttPublishTemp(dev->getTemp3(), 3);
		DataManager::mqttPublishSensorError(!dev->getTemp1Connected(), 1);
		DataManager::mqttPublishSensorError(!dev->getTemp2Connected(), 2);
		DataManager::mqttPublishSensorError(!dev->getTemp3Connected(), 3);
	}
}
