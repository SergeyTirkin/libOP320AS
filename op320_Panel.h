#ifndef OP320_PANEL_H
#define OP320_PANEL_H
#include "defines.h"

#define AMOUNT_FUNC 7//количество поддерживаемых функций
#define AMOUNT_BYTE_RX 64
#define NOT_CHANGE_REG -1//значение адреса регистра которое значит что небыло его изменния
#define TIME_NOT_CONNECT 3000//допустимое время в мс отсутствия связи с панелью

/***********ВЗЯТО ИЗ СПЕЦИФИКАЦИИ НА ПРОТОКОЛ MODBAS RTU************************************************/
#define READ_COILS 0x01//только чтение дискретных выходов(в десятичной 1)                               *
#define READ_DISCRET_INPUTS 0x02//только чтение дискретных входов(в десятичной 2)                       *
#define READ_HOLDING_REGISTERS 0x03//чтение аналоговых выходов либо внутренних значений(в десятичной 3) *
#define READ_INPUT_REGISTERS 0x04//только чтение аналоговых входов(в десятичной 4)                      *
#define WRITE_SINGLE_COIL 0x05//запись одного дискретного выхода (в десятичной 5)                       *
#define WRITE_SINGLE_REGISTER 0x06//запись одного аналогового выхода(в десятичной 6)                    *
#define WRITE_MULTIPLE_REGISTERS 0x10//запись нескольких аналоговых выходов(в десятичной 16)
#define NUMB_BYTE_CODE_FUNC 1//номер байта в пакете для кода фукнкции                                   *
#define NUMB_BYTEH_ADR_REG 2//номер байта в пакете для старшего байта адреса регистра                   *
#define NUMB_BYTEL_ADR_REG 3//номер байта в пакете для младшего байта адреса регистра                   *
#define NUMB_BYTEH_AMOUNT_REG 4//старший номер байта количества регистров                               *
#define NUMB_BYTEL_AMOUNT_REG 5//младший номер байта количества регистров                               *
#define NUMB_BYTEH_DATA_FUNC6 4//старший номер байта записываемых данных в функции6                     *
#define NUMB_BYTEL_DATA_FUNC6 5//младший номер байта записываемых данных в функции6                     *
#define NUMB_BYTEH1_DATA_FUNC15 7//старший номер байта первого регистра записываемых данных в функции15 *
#define NUMB_BYTEL1_DATA_FUNC15 8//младший номер байта первого регистра записываемых данных в функции15 *
#define NUMB_BYTEH2_DATA_FUNC15 9//старший номер байта второго регистра записываемых данных в функции15 *
#define NUMB_BYTEL2_DATA_FUNC15 10//младший номер байта второго регистра записываемых данных в функции15*
#define NUMB_BYTE_NEXT_REPLY_TO_MASTER 2//количество байт далее в ответе для мастера                    *
#define NUMB_BYTE_NEXT_REQUEST_FROM_MASTER 6//номер байта для количества байт далее в запросе от мастера*
#define NUMB_ONE_BYTE_DATA_FUNC3 3 //номер первого байта полезных данных в функции 3                    *
#define NUMB_ONE_BYTE_DATA_FUNC2 3 //номер первого байта полезных данных в функции 2                    *
#define NUMB_ONE_BYTE_DATA_FUNC1 3 //номер первого байта полезных данных в функции 1                    *
#define AMOUNT_BYTE_FUNC6 8//кол-во байт в ответе на функцию 6                                          *
#define AMOUNT_BYTE_FUNC10 8//кол-во байт в ответе на функцию 10

/*******************************************************************************************************/
class op320_Panel{
    private:
    Stream* serial;//для последовательного интерфейса, пока незнаю зачем, но без этого неработает.
    uint8_t slaveId = 1;//адрес модбас слейва (по-умолчанию 1)
    uint8_t packedEndTime = 0;//время конца пакета в мс
    uint32_t timeOkByte = 0;//сюда пишем время когда приходит байт
    uint8_t bufRx[AMOUNT_BYTE_RX];//буфер для приёма данных
    uint8_t bufTx[AMOUNT_BYTE_RX];//буфер для отправки данных
    uint16_t getCrc(uint8_t *_buf, uint16_t _length);
    const uint8_t codeFunc[AMOUNT_FUNC] = {READ_COILS,READ_DISCRET_INPUTS,READ_HOLDING_REGISTERS,READ_INPUT_REGISTERS,
                                           WRITE_SINGLE_COIL,WRITE_SINGLE_REGISTER,WRITE_MULTIPLE_REGISTERS};//массив поддерживаемых функций модбас    
    void packagePanel(uint8_t *_rxData, uint8_t _length);
    void replyMaster(uint8_t _codeFunc, uint8_t _adrReg);//отвечаем мастеру
    void func1(uint8_t _reg);
    void func2(uint8_t _reg);
    void func3(uint8_t _reg);
    void func4(uint8_t _reg);
    void func5(uint8_t _reg);    
    void func6(uint8_t _reg);
    void func15(uint8_t _reg);
    uint16_t mbData[AMOUNT_ADR_REG];//регистры модбас
    int16_t adrRegWrite = NOT_CHANGE_REG;//больше 0 если произошло изменение данных с панели, адрес регистра который был изменён    
    uint32_t timeConnect = 0;
    bool flagStateConnect = false;

    public:
    float qwer = 0;
    op320_Panel(HardwareSerial* _port, uint32_t _baudRate);
    op320_Panel(HardwareSerial* _port, uint32_t _baudRate, uint32_t _config, uint8_t _rxPin, uint8_t _txPin);
    void begin(uint8_t _slaveId);
    bool getStateConnect(void);
    bool task(void);
    void setValFloatReg(float value, uint16_t adrReg);
    void setValUintReg(uint16_t value, uint16_t adrReg);
    void setValIntReg(int16_t value, uint16_t adrReg);
    float getValFloatReg(uint16_t adrReg);
    uint16_t getValReg(uint16_t adrReg);    
    void setAdrRegWrite(int16_t adrRegWrite);    
    int16_t getAdrRegWrite(void);    
};
#endif

