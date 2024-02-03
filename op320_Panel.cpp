#include <Arduino.h>
#include "op320_Panel.h"

//#define DEBUG_OP320
#ifdef DEBUG_OP320
void viewRxPack(uint8_t *_data, uint8_t _length){    
    static uint8_t countRxPack = 0;
    countRxPack++;
    Serial.print("принятый пакет: ");
    for (uint8_t i = 0; i < _length; i++){
        Serial.print(i);
        Serial.print("байт: ");
        Serial.print(_data[i]);
        Serial.print(", ");
    }
    Serial.println();
    Serial.print("счётчик пакетов: ");
    Serial.println(countRxPack);
    Serial.println("---------------------------");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
};

void viewTxPack(byte *_data, uint16_t _length){    
    static uint8_t countTxPack = 0;
    countTxPack++;
    Serial.print("отправленный пакет: ");
    for (uint8_t i = 0; i < _length; i++){
        Serial.print(i);
        Serial.print("байт: ");
        Serial.print(_data[i]);
        Serial.print(", ");
    }
    Serial.println();
    Serial.print("счётчик пакетов: ");
    Serial.println(countTxPack);
    Serial.println("---------------------------");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
};
#endif

/*!
* @param _port номер используемого порта @param _baudrate скорость обменна данными
*/
op320_Panel::op320_Panel(HardwareSerial* _port, uint32_t _baudRate){
    _port->begin(_baudRate);
    this->serial = _port;
    packedEndTime = ceilf(((3.5 * 10) / _baudRate));//расчитаем время конца пакета(округление до целого вверх)
    packedEndTime++;//плюс 1 мс, а то не принимает все байты    
};

/*!
* @param _port номер используемого порта @param _baudrate скорость обменна данными
* @param _config конфигурация обмена @param _rxPin пин приёма @param _txPin пин отправки
*/
op320_Panel::op320_Panel(HardwareSerial* _port, uint32_t _baudRate, uint32_t _config, uint8_t _rxPin, uint8_t _txPin){
    _port->begin(_baudRate, _config, _rxPin, _txPin);
    this->serial = _port;
    packedEndTime = ceilf(((3.5 * 10) / _baudRate));//расчитаем время конца пакета(округление до целого вверх)
    packedEndTime++;//плюс 1 мс, а то не принимает все байты
};

/*!
* @param _slaveId адрес модбас слейва
*/
void op320_Panel::begin(uint8_t _slaveId){
    slaveId = _slaveId;
    bufTx[0] = slaveId;//от какого слейва ответ
    for (uint8_t i = 0; i < AMOUNT_ADR_REG; i++)
    {
        mbData[i] = 0;
    }
           
}
bool op320_Panel::getStateConnect(void)
{
    return flagStateConnect;
};

void op320_Panel::func1(uint8_t _reg)
{
    uint8_t lengthPack = 0;//длина передаваемого пакета
    uint16_t crcSum = 0;//для контрольной суммы
    uint8_t amountOut = bufRx[NUMB_BYTEL_AMOUNT_REG];//кол-во выходов которые нужно прочитать
    uint8_t amountByteNext = 1;
    bufTx[1] = READ_COILS;
    bufTx[NUMB_BYTE_NEXT_REPLY_TO_MASTER] = 1;
    bufTx[NUMB_ONE_BYTE_DATA_FUNC1] = mbData[_reg]>>8;//особености OP320, так как читает только по одному входу    
    crcSum = getCrc(bufTx,4);
    bufTx[4] = crcSum;
    bufTx[5] = crcSum>>8;
    for (byte i = 0; i < 6; i++){//отправляем ответ мастеру
        serial->write(bufTx[i]);
    }    
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, 6);//отправляем отладочную информацию
    #endif
}

void op320_Panel::func2(uint8_t _reg)
{
    uint8_t lengthPack = 0;//длина передаваемого пакета
    uint16_t crcSum = 0;//для контрольной суммы
    uint8_t amountOut = bufRx[NUMB_BYTEL_AMOUNT_REG];//кол-во выходов которые нужно прочитать
    uint8_t amountByteNext = 1;
    bufTx[1] = READ_DISCRET_INPUTS;
    bufTx[NUMB_BYTE_NEXT_REPLY_TO_MASTER] = 1;
    bufTx[NUMB_ONE_BYTE_DATA_FUNC1] = mbData[_reg];//особености OP320, так как читает только по одному выходу
    crcSum = getCrc(bufTx,4);
    bufTx[4] = crcSum;
    bufTx[5] = crcSum>>8;
    for (byte i = 0; i < 6; i++){//отправляем ответ мастеру
        serial->write(bufTx[i]);
    }    
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, 6);//отправляем отладочную информацию
    #endif


}

void op320_Panel::func5(uint8_t _reg)
{
    mbData[_reg] = (bufRx[NUMB_BYTEH_DATA_FUNC6] << 8) | bufRx[NUMB_BYTEL_DATA_FUNC6];//сшиваем байты 
    adrRegWrite = _reg;    
    for (byte i = 0; i < AMOUNT_BYTE_FUNC6; i++)
    {//отправляем ответ мастеру то же что и приняли
        serial->write(bufRx[i]);
    }    
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, AMOUNT_BYTE_FUNC6);//отправляем отладочную информацию
    #endif 

};

void op320_Panel::func3(uint8_t _reg){
    uint8_t lengthPack = 0;//длина передаваемого пакета
    uint16_t crcSum = 0;//для контрольной суммы
    uint8_t amountReg = bufRx[NUMB_BYTEL_AMOUNT_REG];
    lengthPack = 5 + (amountReg*2);//длина отправляемого ответа
    bufTx[NUMB_BYTE_CODE_FUNC] = READ_HOLDING_REGISTERS;//номер функции
    bufTx[NUMB_BYTE_NEXT_REPLY_TO_MASTER] = amountReg * 2;//кол-во байт далее
    for (uint8_t i = 0; i < amountReg; i++){//заносим в пакет на отправку запрашиваемые данные
        bufTx[NUMB_ONE_BYTE_DATA_FUNC3+(i*2)] = mbData[_reg]>>8;//старший байт запрашиваемых данных
        bufTx[NUMB_ONE_BYTE_DATA_FUNC3+1+(i*2)] = mbData[_reg];//младший байт запрашиваемых данных
    }        
    crcSum = getCrc(bufTx,lengthPack-2);//считаем контрольную сумму
    bufTx[lengthPack-1] = crcSum>>8;//старший байт контрольной суммы
    bufTx[lengthPack-2] = crcSum;//младший байт контрольной суммы
    for (byte i = 0; i < lengthPack; i++){//отправляем ответ мастеру
        serial->write(bufTx[i]);
    }
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, lengthPack);//отправляем отладочную информацию
    #endif 
}

void op320_Panel::func4(uint8_t _reg)
{
    uint8_t lengthPack = 0;//длина передаваемого пакета
    uint16_t crcSum = 0;//для контрольной суммы
    uint8_t amountReg = bufRx[NUMB_BYTEL_AMOUNT_REG];
    lengthPack = 5 + (amountReg*2);//длина отправляемого ответа
    bufTx[NUMB_BYTE_CODE_FUNC] = READ_INPUT_REGISTERS;//номер функции
    bufTx[NUMB_BYTE_NEXT_REPLY_TO_MASTER] = amountReg * 2;//кол-во байт далее
    for (uint8_t i = 0; i < amountReg; i++){//заносим в пакет на отправку запрашиваемые данные
        bufTx[NUMB_ONE_BYTE_DATA_FUNC3+(i*2)] = mbData[_reg]>>8;//старший байт запрашиваемых данных
        bufTx[NUMB_ONE_BYTE_DATA_FUNC3+1+(i*2)] = mbData[_reg];//младший байт запрашиваемых данных
    }
    crcSum = getCrc(bufTx,lengthPack-2);//считаем контрольную сумму
    bufTx[lengthPack-1] = crcSum>>8;//старший байт контрольной суммы
    bufTx[lengthPack-2] = crcSum;//младший байт контрольной суммы
    for (byte i = 0; i < lengthPack; i++){//отправляем ответ мастеру
        serial->write(bufTx[i]);
    }
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, lengthPack);//отправляем отладочную информацию
    #endif

};

void op320_Panel::func6(uint8_t _reg){
    mbData[_reg] = (bufRx[NUMB_BYTEH_DATA_FUNC6] << 8) | bufRx[NUMB_BYTEL_DATA_FUNC6];//сшиваем байты 
    adrRegWrite = _reg;        
    for (byte i = 0; i < AMOUNT_BYTE_FUNC6; i++){//отправляем ответ мастеру то же что и приняли
        serial->write(bufRx[i]);
    }    
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, AMOUNT_BYTE_FUNC6);//отправляем отладочную информацию
    #endif 
};

void op320_Panel::func15(uint8_t _reg){
    // mbData[_reg] = ((bufRx[NUMB_BYTEH1_DATA_FUNC15] << 8) | bufRx[NUMB_BYTEL1_DATA_FUNC15]);//сшиваем байты
    // mbData[_reg+1] = ((bufRx[NUMB_BYTEH2_DATA_FUNC15] << 8) | bufRx[NUMB_BYTEL2_DATA_FUNC15]);   
    mbData[_reg] = ((bufRx[NUMB_BYTEH2_DATA_FUNC15] << 8) | bufRx[NUMB_BYTEL2_DATA_FUNC15]);//сшиваем байты
    mbData[_reg+1] = ((bufRx[NUMB_BYTEH1_DATA_FUNC15] << 8) | bufRx[NUMB_BYTEL1_DATA_FUNC15]);
    adrRegWrite = _reg;    
    bufTx[1] = WRITE_MULTIPLE_REGISTERS;
    bufTx[2] = 0;
    bufTx[3] = _reg;
    bufTx[4] = 0;
    bufTx[5] = 2;
    uint16_t crcSum = getCrc(bufTx,6);
    bufTx[6] = crcSum;//младший байт контрольной суммы
    bufTx[7] = crcSum >> 8;//старший байт контрольной суммы
    for (byte i = 0; i < AMOUNT_BYTE_FUNC10; i++){//отправляем ответ мастеру
        serial->write(bufTx[i]);
    }    
    #ifdef DEBUG_OP320
        viewTxPack(bufTx, AMOUNT_BYTE_FUNC10);//отправляем отладочную информацию
    #endif 
};

void op320_Panel::replyMaster(uint8_t _codeFunc, uint8_t _adrReg){        
    switch (_codeFunc){
    case READ_COILS:
        func1(_adrReg);
        timeConnect = millis();//запоминаем время ответа
        break;
    case READ_DISCRET_INPUTS:
        func2(_adrReg);
        timeConnect = millis();//запоминаем время ответа
        break;
    case READ_HOLDING_REGISTERS:
        func3(_adrReg);
        timeConnect = millis();//запоминаем время ответа
        break;
    case READ_INPUT_REGISTERS:
        func4(_adrReg);
        timeConnect = millis();//запоминаем время ответа
        break;
    case WRITE_SINGLE_COIL:
        func5(_adrReg);
        timeConnect = millis();//запоминаем время ответа
        break;
     case WRITE_SINGLE_REGISTER:
        func6(_adrReg);
        timeConnect = millis();//запоминаем время ответа        
        break;
    case WRITE_MULTIPLE_REGISTERS:
        func15(_adrReg);
        timeConnect = millis();//запоминаем время ответа        
        break;                           
    default:
        break;
    }           
};//отвечаем мастеру

uint16_t op320_Panel::getCrc(byte *_buf, uint16_t _length){
    byte j = 0;
	uint16_t crc = 0xFFFF;
	while (_length--){
        crc ^= *_buf++;
	    for(j = 0; j < 8; j++){
            if(crc & 0x01){
                crc = (crc >> 1) ^ 0xA001;                
	        }else{
                crc = crc >> 1;
		    }
	    }
    }
    return crc;
};

void op320_Panel::packagePanel(uint8_t *_rxData, uint8_t _length){
    uint8_t numbFunc = 0, adrReg = 0, crcL = 0, crcH = 0;
    uint16_t crcSum = 0;//для контрольной суммы
    for (uint8_t i = 0; i < AMOUNT_FUNC; i++){
        if(_rxData[1] == codeFunc[i]) numbFunc = codeFunc[i];//ищем номер функции
    }
    if(numbFunc){//если есть поддерживаемая функция то работаем дальше
       for (uint8_t i = 0; i < AMOUNT_ADR_REG; i++){
           if(_rxData[NUMB_BYTEL_ADR_REG] == i) adrReg = i;//ищем номер регистра
       }       
    }else return;
    if(adrReg>-1 && adrReg < AMOUNT_ADR_REG){//если есть номер подерживаемого регистра то работаем дальше
        crcSum = getCrc(_rxData, _length-2);//считаем контрольную сумму
        crcL = crcSum;//младший байт контрольной суммы
        crcH = (crcSum >> 8);//старший байт контрольной суммы
        if(crcL != _rxData[_length-2]) return;//сравниваем байты
        if(crcH != _rxData[_length-1]) return;//контрольной суммы         
    }else return;
    replyMaster(numbFunc,adrReg);//отправляем ответ мастеру
};//обрабатываем и проверяем пакет от мастера

/// @brief поток для обменна данными с панелью
/// @return true - наличие связи с панелью, false - отсутствие связи с панелью,
bool op320_Panel::task(void){
    if(serial->available() > 0){
        uint8_t countByte = 0;
        bufRx[countByte] = serial->read();
        if(bufRx[countByte] == slaveId){
            countByte++;
            timeOkByte = millis();//запоминаем время принятия первого байта
            do{
                if(serial->available() > 0 ){
                    timeOkByte = millis();                
                    bufRx[countByte] = serial->read();
                    countByte++;
                    if(countByte == AMOUNT_BYTE_RX) break;
                }
            } while (millis() - timeOkByte < packedEndTime);//крутим цикл пока не поймём что наступило время конца пакета
        }else uint8_t emptyByte = serial->read();//читаем пустой байт даже если обращение не к нам, что-бы очистить буфер приёма
        if(countByte > 1){
            packagePanel(bufRx,countByte);//если байтов больше чем 1 то обрабатываем пакет от мастера
            #ifdef DEBUG_OP320
            viewRxPack(bufRx, countByte);//отправляем отладочную информацию            
            #endif
        }  
    }
    if(millis() - timeConnect > TIME_NOT_CONNECT)
    {
        flagStateConnect = false;
        return false;
    }
    flagStateConnect = true;
    return true;
}

/// @brief записать значение float в регистры модбас
/// @param value значение которое надо записать
/// @param adrReg адрес регистра модбас куда будем записывать
void op320_Panel::setValFloatReg(float value, uint16_t adrReg)
{
    union uint8map
    {
        float _float;
        uint8_t _uint8_t[sizeof(float)];
    };uint8map uint8map;
  uint8map._float = value;
    
  mbData[adrReg+1] = (uint8map._uint8_t[0] | uint8map._uint8_t[1] << 8);//порядок байт ABCD
  mbData[adrReg] = (uint8map._uint8_t[2] | uint8map._uint8_t[3] << 8);
  
//   mbData[1] = (uint8map._uint8_t[2] | uint8map._uint8_t[3] << 8);//порядок байт CDAB
//   mbData[0] = (uint8map._uint8_t[0] | uint8map._uint8_t[1] << 8);
}

/// @brief установить значение в регистр
/// @param value новое значение
/// @param adrReg адрес регистра модбас
void op320_Panel::setValUintReg(uint16_t value, uint16_t adrReg)
{
    mbData[adrReg] = value;
}

/// @brief получить значение регистра
/// @param adrReg адрес регистра куда надо установить значение
/// @return значение float из uint16_t
float op320_Panel::getValFloatReg(uint16_t adrReg)
{
    union uint8map
    {
        float _float;
        uint8_t _uint8_t[sizeof(float)];
    };uint8map uint8map; 
  
    // uint8map._uint8_t[0] = mbData[adrReg];//порядок байт ABCD
    // uint8map._uint8_t[1] = mbData[adrReg] >> 8;
    // uint8map._uint8_t[2] = mbData[adrReg+1];
    // uint8map._uint8_t[3] = mbData[adrReg+1] >> 8;
  
    uint8map._uint8_t[2] = mbData[adrReg];//порядок байт CDAB
    uint8map._uint8_t[3] = mbData[adrReg] >> 8;
    uint8map._uint8_t[0] = mbData[adrReg+1];
    uint8map._uint8_t[1] = mbData[adrReg+1] >> 8;
  
    return uint8map._float;
}

uint16_t op320_Panel::getValReg(uint16_t adrReg)
{
    return mbData[adrReg];
}

/// @brief установить адрес изменяемого регистра
/// @param adrRegWrite значение регистра, установить в 0 или < 0 чтобы не вызывать внешнее событие
void op320_Panel::setAdrRegWrite(int16_t adrRegWrite)
{
    this->adrRegWrite = adrRegWrite;
}

/// @brief получить адрес регистра который был изменен, если значение > 0, значит регистр был изменен
/// @return адрес регистра который был изменен, после вызова выставить в 0 или -1
int16_t op320_Panel::getAdrRegWrite(void)
{
    return adrRegWrite;
}