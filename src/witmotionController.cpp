//
//  witmotionController.cpp
//  Created by Qaotech Studio on 21/12/20.
//

#include "witmotionController.h"

witmotionController::witmotionController() {

}

witmotionController::~witmotionController() {

}


//----------------------------------------------------- setup.
void witmotionController::setup() {
    getDeviceIDs();
    isConnected = false;
    resetOrientation();
}

void witmotionController::update(){
    if(isConnected){
        ret = recvData(r_buf, 44);

        for (int i = 0; i < ret; i++) {
            parseData(r_buf[i]);
        }
    }
}

vector <string> witmotionController::getDeviceIDs(){
    deviceList = serial.getDeviceList();
    for(int i=0; i<deviceList.size(); i++) {
        deviceIDs.push_back(deviceList[i].getDeviceName().c_str());
    }
    return deviceIDs;
}


void witmotionController::connectDevice(int deviceID){
    isConnected = serial.setup(deviceID, BAUD);
    
    //flush the serial port once before we start
    serial.flush();
    bzero(r_buf, 1024);
}


void witmotionController::disconnectDevice() {
    if(isConnected) {
        serial.close();
    }
}

void witmotionController::resetOrientation(){
    for(int i=0; i< 3; i++){
        magnetic_field_shift[i] = 0;
        acceleration_shift[i] = 0;
        gyro_shift[i] = 0;
        angle_shift[i] = 0;
    }
}

void witmotionController::tare(){
    // angle tare
    float* swpAngle = getAngle();
    angle_shift[1] = angle_shift[1] - swpAngle[0];
    angle_shift[2] = angle_shift[2] - swpAngle[1];
    angle_shift[0] = angle_shift[0] - swpAngle[2];
    
    // acceleration tare
    float* swpAcceleration = getAcceleration();
    acceleration_shift[0] = acceleration_shift[0] - swpAcceleration[0];
    acceleration_shift[1] = acceleration_shift[1] - swpAcceleration[1];
    acceleration_shift[2] = acceleration_shift[2] - swpAcceleration[2];

    // gyro tare
    float* swpGyro = getGyro();
    gyro_shift[0] = gyro_shift[0] - swpGyro[0];
    gyro_shift[1] = gyro_shift[1] - swpGyro[1];
    gyro_shift[2] = gyro_shift[2] - swpGyro[2];
    
    // magnetic field tare
    float* swpMagneticField = getMagneticField();
    magnetic_field_shift[0] = magnetic_field_shift[0] - swpMagneticField[0];
    magnetic_field_shift[1] = magnetic_field_shift[1] - swpMagneticField[1];
    magnetic_field_shift[2] = magnetic_field_shift[2] - swpMagneticField[2];
}

float* witmotionController::getAcceleration() {
    float* calculated_acceleration = new float[3];
    
    calculated_acceleration[0] = acceleration[0] + acceleration_shift[0];
    calculated_acceleration[1] = acceleration[2] + acceleration_shift[1];
    calculated_acceleration[2] = acceleration[2] + acceleration_shift[2];
    
    return calculated_acceleration;
}

float* witmotionController::getGyro() {
    float* calculated_gyro = new float[3];
    
    calculated_gyro[0] = gyro[0] + gyro_shift[0];
    calculated_gyro[1] = gyro[2] + gyro_shift[1];
    calculated_gyro[2] = gyro[2] + gyro_shift[2];
    
    return calculated_gyro;
}

float* witmotionController::getAngle() {
    float* calculated_angle = new float[3];
        
    // mapping hardware x-y-z axis to openFrameworks x-y-z axis standarts
    calculated_angle[0] = angle[1] + angle_shift[1];
    calculated_angle[1] = angle[2] + angle_shift[2];
    calculated_angle[2] = angle[0] + angle_shift[0];
    
    return calculated_angle;
}

float* witmotionController::getMagneticField() {
    float* calculated_magnetic_field = new float[3];
    
    calculated_magnetic_field[0] = magnetic_field[0] + magnetic_field_shift[0];
    calculated_magnetic_field[1] = magnetic_field[2] + magnetic_field_shift[1];
    calculated_magnetic_field[2] = magnetic_field[2] + magnetic_field_shift[2];
    
    return calculated_magnetic_field;
}

int witmotionController::recvData(char *recv_buffer, int length) {
    long retLength = serial.readBytes(recv_buffer, length);
    int len = ofToInt(ofToString(retLength));
    return len;
}

void witmotionController::parseData(char chr) {
    static char chrBuf[100];
    static unsigned char chrCnt = 0;
    signed short sData[4];
    unsigned char i;
    char cTemp = 0;
    time_t now;
    chrBuf[chrCnt++] = chr;
    
    if (chrCnt < 11)
        return;
    for (i = 0; i < 10; i++)
    cTemp += chrBuf[i];
    if ((chrBuf[0] != 0x55) || ((chrBuf[1] & 0x50) != 0x50) || (cTemp != chrBuf[10])) {
        ofLog() << "Error:%x %x\r\n", chrBuf[0], chrBuf[1];
        memcpy(&chrBuf[0], &chrBuf[1], 10);
        chrCnt--;
        return;
    }
    
    memcpy(&sData[0], &chrBuf[2], 8);
    switch (chrBuf[1]) {
        case 0x51:
            for (i = 0; i < 3; i++)
            acceleration[i] = (float)sData[i] / 32768.0 * 16.0;
            time(&now);
            break;
        case 0x52:
            for (i = 0; i < 3; i++)
            gyro[i] = (float)sData[i] / 32768.0 * 2000.0;
            break;
        case 0x53:
            for (i = 0; i < 3; i++)
            angle[i] = (float)sData[i] / 32768.0 * 180.0;
            break;
        case 0x54:
            for (i = 0; i < 3; i++)
            magnetic_field[i] = (float)sData[i];
            break;
    }

    chrCnt = 0;
}
