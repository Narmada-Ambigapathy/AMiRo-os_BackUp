#include "userthread.hpp"
#include <chprintf.h>
#include <ctime>
#include <hal.h>
#include <complex>
#include <vector>
#include <iostream>
#include <cmath>
#include "global.hpp"


using namespace amiro;

using std::size_t;
using std::vector;
using std::complex;
using std::exp;


extern Global global;

// For storing complex values of nth roots
// of unity we use complex<double>
typedef std::complex<float> cd;

UserThread::UserThread() :
    chibios_rt::BaseStaticThread<USER_THREAD_STACK_SIZE>()
{
    maxFtValue = 0;
    maxFtIndex = 0;
}

UserThread::~UserThread()
{
}

void UserThread::sleepForSec(int inSeconds)
{
    for(int i = 0; i < inSeconds; i++)
    {
        chprintf((BaseSequentialStream*) &global.sercanmux1,"Sleep : %d\n", i);
        this->sleep(MS2ST(1000));
    }
}


void UserThread::microphoneInput()
{
    // input start

    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nCycle %d : Input started\n", cycleNumber);

    i2sStart(&I2SD2, &global.i2scfg);
    this->sleep(MS2ST(100));
    i2sStartExchange(&I2SD2);

    this->sleep(MS2ST(500));

    i2sStopExchange(&I2SD2);
    this->sleep(MS2ST(100));
    i2sStop(&I2SD2);

    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nCycle %d : Input ended\n", cycleNumber);
    cycleNumber++;

    // input end
}

void UserThread::adjustData(std::vector<std::complex<float> > &outFftInput)
{
    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nAdjusting Data start\n");

    for(int i = 1,k=0; i < I2S_BUF_SIZE; i = i+2,k++)
    {
        uint32_t raw = global.i2s_rx_buf[i];
        int16_t data = (raw & 0xFFFF);
        if(data == 0){
            data = (raw >> 16) & 0xFFFF;
        }

        // "data" is what you want aka the correct data
        // put this value to a vector of complex<float>
        // complex<flaot> _example = 1.0;
        // output : (1.0, 0.0)
        outFftInput.at(k) = static_cast<float>(data);
        // chprintf((BaseSequentialStream*)&global.sercanmux1,"%d,%d\n", i, data);
        // dataVec.push_back(data);


    }
    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nAdjusting Data end\n");


    // serialize input data
//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nData Start: \n");
//    for(int i = 0; i < outFftInput.size(); i++)
//    {
//        chprintf((BaseSequentialStream*)&global.sercanmux1,"%f,", outFftInput.at(i).real());
//    }
//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nData End: \n");
}


vector<complex<float> > UserThread::computeDft(const vector<complex<float> > &input, const int inFtRange) {

    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nDFT Start\n");

    vector<complex<float> > output;
    complex<float> i(0.0,1.0);

//    // not related to FT
//    float max = 0;

    size_t n = input.size();
    // replace n(from the outer loop) with the any number; if you want fft result up to that range
    for (size_t k = 0; k < inFtRange; k++) {  // For each output element
        complex<float> sum(0.0, 0.0);
        for (size_t t = 0; t < n; t++) {  // For each input element
            complex<float> angle = 2 * M_PI * t * k / n;
            angle = angle * i;
            sum += input[t] * exp(-angle);
        }
        output.push_back(sum);

//        // not related to FT
//        if(std::abs(sum) > max)
//        {
//            max = maxFtValue = abs(sum);
//            maxFtIndex = k;
//        }
    }

//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nmaxFtIndex : %d, maxFtValue : %f\n", maxFtIndex, maxFtValue);
    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nDFT Finished\n");

    return output;
}

void UserThread::printFftResult(const std::vector<std::complex<float> > &inFftInput,
                    const std::vector<std::complex<float> > &inFftOutput)
{
    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nIndex,Data,Frequency,Real,Imaginary,Abs\n");
    float frequency = 0;
    int fftInputSize = inFftInput.size();
    float fftInputSizeF = static_cast<float>(inFftInput.size());
    for(int i = 0; i < fftInputSize; i++)
    {
        frequency = 32000.0 / fftInputSizeF * static_cast<float>(i);

        if(i < inFftOutput.size())
        {
            chprintf((BaseSequentialStream*)&global.sercanmux1,"%d,%f,%f,%f,%f,%f\n",
                     i, inFftInput.at(i).real(), frequency, inFftOutput.at(i).real(), inFftOutput.at(i).imag(), std::abs(inFftOutput.at(i)));
        }
        else
        {
            chprintf((BaseSequentialStream*)&global.sercanmux1,"%d,%f,%f,0,0,0\n",
                     i, inFftInput.at(i).real(), frequency);
        }

    }

//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nSerialized FFT Output Start\n");
//    int fftOutputSize = inFftOutput.size();
//    for(int i = 0; i < fftOutputSize; i++)
//    {
//        chprintf((BaseSequentialStream*)&global.sercanmux1,"%f,",std::abs(inFftOutput.at(i)));
//    }
//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nSerialized FFT Output End\n");
}

float UserThread::ftThreshold(const std::vector<std::complex<float> > &inFftOutput)
{
    float sum = 0;
    int size = inFftOutput.size();

    // hard coded for now
    if(size > 16)
    {
        size = 16;
    }

    for(int i = 0; i < size; i++)
    {
        sum += std::abs(inFftOutput.at(i));
    }

    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nAverage : %f\n", (sum / static_cast<float>(size)));

    return (sum / static_cast<float>(size));
}

float UserThread::ftThreshold2(const std::vector<std::complex<float> > &inFftOutput)
{
    size_t size = inFftOutput.size();
    float max = 0;
    for(int i = 0; i < size; i++)
    {
        if(abs(inFftOutput.at(i)) > max)
        {
            max = maxFtValue = abs(inFftOutput.at(i));
            maxFtIndex = i;
        }
    }

    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nmaxFtIndex : %d, maxFtValue : %f\n", maxFtIndex, maxFtValue);

    return maxFtIndex;
}

void UserThread::lightOnlyTheHightestFreq(const std::vector<std::complex<float> > &inFftOutput, int inFfRange)
{
    // shows only one light, the one with the highest frequency
    float maxIndex = ftThreshold2(inFftOutput);
    for(int i = 0; i < (inFfRange + 1); i++)
    {
        if(i == maxIndex)
        {
            chprintf((BaseSequentialStream*) &global.sercanmux1, "\nLight i : %d, RED\n", i);
            global.robot.setLightColor(i, Color::RED);
        }
        else
        {
            global.robot.setLightColor(i, Color::BLACK);
        }
    }
}

void UserThread::lightTillHighestFrequency(const std::vector<std::complex<float> > &inFftOutput, int inFfRange)
{
    // shows light till the highest frequency
    float maxIndex = ftThreshold2(inFftOutput);
    for(int i = 0; i < (inFfRange + 1); i++)
    {
        if(i <= maxIndex)
        {
            chprintf((BaseSequentialStream*) &global.sercanmux1, "\nLight i : %d, RED\n", i);
            global.robot.setLightColor(i, Color::RED);
        }
        else
        {
            global.robot.setLightColor(i, Color::BLACK);
        }
    }
}

void UserThread::motorControl(std::vector<std::complex<float>> &outFftInput)
{
    // shows light till the highest frequency
    float maxIndex = ftThreshold2(outFftInput);

    int baseSpeed = 10000000 * 4;
    int motorSpeed = maxIndex * baseSpeed;
    global.robot.setTargetSpeed(motorSpeed, -motorSpeed);
}

void UserThread::manualDftIncomplete()
{
//    // DFT Manual
//    // Example code for serialized vector
//    vector<complex<double>> fftOutput;
//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nDataVec\n");
//    for(int i = 0; i < dataVec.size(); i++)
//    {
//        // chprintf((BaseSequentialStream*)&global.sercanmux1,"%d\n", dataVec.at(i));

//        double fft_val_real = 0.0;
//        double fft_val_imag = 0.0;
//        for(int j = 0; j < dataVec.size(); j++)
//        {
//            complex<double> fft_val = std::polar(1.0, -2 * M_PI * i * j / dataVec.size()) * dataVec.at(i);
//            fft_val_real += real(fft_val);
//            fft_val_imag += imag(fft_val);
//        }
//        complex<double> temp = {fft_val_real, fft_val_imag};
//        fftOutput.push_back(temp);

//    }
//    chprintf((BaseSequentialStream*)&global.sercanmux1,"\nDataVec\n");
}


void UserThread::ftSpecifications(int &outFtRange, int &outNecessaryBufferSize)
{
    // the lowest frequncy range we want to measure
    int low = 0;
    // the highest frequncy range we want to measure
    int high = 800;
    int lightNumber = 8;

    // or we can say how accurate we want our frequncy calculation will be
    int stepSize = (high - low) / lightNumber;
    outNecessaryBufferSize = 32000 / stepSize;

    vector<int> frequencyList;
    frequencyList.resize(outNecessaryBufferSize);

    for(int i = 0; i < outNecessaryBufferSize; i++)
    {
        frequencyList.at(i) = 32000.0 / static_cast<float> (outNecessaryBufferSize) * static_cast<float> (i);

        if(frequencyList.at(i) < high)
        {
            outFtRange = i;
        }

    }
}

void UserThread::mockDataFftInput(std::vector<std::complex<float>> &outFftInput)
{
    // std::vector<int16_t> dataVec = {-1653,-1653,-1652,-1652,-1651,-1649,-1649,-1647,-1647,-1646,-1645,-1646,-1648,-1646,-1646,-1646,-1644,-1644,-1644,-1644,-1645,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1653,-1653,-1654,-1655,-1656,-1658,-1658,-1658,-1659,-1660,-1661,-1661,-1662,-1663,-1663,-1665,-1665,-1665,-1667,-1668,-1667,-1667,-1669,-1670,-1671,-1672,-1674,-1674,-1676,-1678,-1680,-1681,-1681,-1681,-1682,-1684,-1685,-1686,-1687,-1688,-1689,-1690,-1691,-1691,-1690,-1691,-1691,-1691,-1692,-1693,-1693,-1694,-1695,-1695,-1694,-1695,-1694,-1694,-1696,-1695,-1694,-1695,-1696,-1696,-1695,-1695,-1695,-1694,-1693,-1692,-1692,-1691,-1691,-1690,-1689,-1687,-1686,-1685,-1684,-1684,-1684,-1682,-1681,-1680,-1679,-1677,-1676,-1676,-1676,-1675,-1675,-1674,-1671,-1669,-1668,-1667,-1666,-1665,-1664,-1664,-1665,-1663,-1662,-1661,-1660,-1659,-1659,-1659,-1658,-1658,-1658,-1657,-1655,-1655,-1655,-1655,-1654,-1652,-1651,-1650,-1650,-1650,-1649,-1648,-1647,-1646,-1645,-1644,-1644,-1644,-1644,-1644,-1645,-1645,-1644,-1644,-1643,-1643,-1644,-1645,-1646,-1647,-1650,-1652,-1652,-1651,-1652,-1653,-1654,-1656,-1656,-1657,-1657,-1658,-1660,-1662,-1662,-1662,-1663,-1663,-1663,-1664,-1665,-1665,-1666,-1669,-1670,-1670,-1671,-1672,-1673,-1673,-1673,-1675,-1676,-1678,-1681,-1681,-1681,-1683,-1684,-1685,-1686,-1687,-1686,-1687,-1688,-1690,-1690,-1690,-1691,-1691,-1690,-1691,-1693,-1692,-1692,-1694,-1695,-1695,-1697,-1697,-1696,-1697,-1697,-1697,-1697,-1696,-1697,-1697,-1696,-1696,-1697,-1695,-1694,-1694,-1692,-1691,-1691,-1691,-1690,-1689,-1688,-1687,-1685,-1684,-1684,-1682,-1681,-1681,-1680,-1679,-1679,-1679,-1678,-1676,-1675,-1674,-1672,-1670,-1670,-1669,-1668,-1668,-1666,-1666,-1665,-1664,-1662,-1660,-1660,-1660,-1660,-1659,-1659,-1660,-1659,-1657,-1656,-1656,-1654,-1652,-1653,-1653,-1651,-1650,-1651,-1651,-1649,-1649,-1648,-1646,-1645,-1645,-1644,-1644,-1645,-1645,-1644,-1645,-1645,-1645,-1644,-1644,-1646,-1647,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1654,-1654,-1654,-1655,-1656,-1658,-1659,-1660,-1660,-1661,-1660,-1661,-1662,-1664,-1665,-1666,-1667,-1667,-1668,-1668,-1669,-1670,-1670,-1671,-1672,-1673,-1675,-1677,-1679,-1680,-1680,-1681,-1682,-1683,-1684,-1686,-1686,-1687,-1688,-1688,-1688,-1689,-1689,-1690,-1690,-1690,-1690,-1690,-1691,-1693,-1694,-1694,-1693,-1693,-1693,-1693,-1693,-1694,-1695,-1695,-1695,-1694,-1694,-1693,-1693,-1692,-1691,-1691,-1691,-1690,-1690,-1690,-1688,-1687,-1685,-1684,-1683,-1683,-1682,-1682,-1681,-1679,-1678,-1678,-1677,-1676,-1675,-1674,-1672,-1671,-1671,-1668,-1668,-1667,-1665,-1665,-1664,-1663,-1662,-1661,-1660,-1660,-1659,-1658,-1658,-1658,-1656,-1655,-1656,-1655,-1655,-1654,-1652,-1651,-1650,-1651,-1650,-1649,-1648,-1648,-1647,-1646,-1646,-1646,-1644,-1643,-1645,-1644,-1643,-1644,-1645,-1644,-1643,-1643,-1644,-1644,-1644,-1645,-1646,-1647,-1647,-1649,-1650,-1650,-1651,-1651,-1651,-1651,-1653,-1655,-1656,-1656,-1658,-1659,-1659,-1660,-1660,-1661,-1662,-1662,-1663,-1663,-1664,-1666,-1666,-1668,-1669,-1669,-1670,-1670,-1671,-1672,-1674,-1675,-1676,-1678,-1679,-1680,-1682,-1682,-1683,-1683,-1683,-1684,-1685,-1686,-1686,-1688,-1689,-1689,-1689,-1689,-1691,-1691,-1691,-1691,-1691,-1692,-1692,-1693,-1693,-1693,-1693,-1693,-1692,-1691,-1692,-1692,-1691,-1690,-1691,-1691,-1690,-1690,-1690,-1689,-1688,-1686,-1684,-1684,-1684,-1683,-1683,-1682,-1680,-1679,-1677,-1676,-1677,-1675,-1673,-1674,-1674,-1672,-1671,-1670,-1668,-1668,-1667,-1664,-1663,-1662,-1661,-1661,-1661,-1662,-1661,-1658,-1657,-1656,-1656,-1655,-1654,-1655,-1654,-1653,-1653,-1652,-1652,-1651,-1650,-1649,-1648,-1647,-1647,-1647,-1647,-1646,-1645,-1644,-1643,-1642,-1642,-1641,-1640,-1640,-1640,-1641,-1642,-1642,-1642,-1642,-1642,-1643,-1643,-1644,-1645,-1646,-1647,-1648,-1649,-1650,-1651,-1653,-1654,-1654,-1654,-1656,-1656,-1657,-1657,-1658,-1659,-1660,-1661,-1661,-1662,-1663,-1665,-1666,-1666,-1667,-1668,-1669,-1670,-1671,-1672,-1673,-1675,-1677,-1679,-1680,-1682,-1683,-1683,-1684,-1685,-1685,-1686,-1687,-1688,-1688,-1689,-1689,-1690,-1691,-1691,-1692,-1692,-1692,-1692,-1693,-1694,-1693,-1695,-1696,-1695,-1695,-1695,-1695,-1694,-1694,-1695,-1695,-1695,-1695,-1694,-1693,-1693,-1692,-1692,-1691,-1689,-1688,-1688,-1687,-1686,-1687,-1686,-1684,-1683,-1682,-1680,-1679,-1679,-1678,-1678,-1677,-1677,-1676,-1675,-1673,-1671,-1669,-1668,-1667,-1666,-1666,-1666,-1666,-1665,-1664,-1664,-1663,-1660,-1658,-1659,-1658,-1656,-1657,-1658,-1656,-1656,-1655,-1654,-1653,-1652,-1650,-1649,-1648,-1649,-1648,-1646,-1647,-1646,-1645,-1645,-1646,-1645,-1643,-1642,-1642,-1642,-1641,-1641,-1642,-1643,-1643,-1644,-1645,-1647,-1647,-1646,-1647,-1648,-1648,-1650,-1651,-1652,-1654,-1655,-1655,-1656,-1657,-1658,-1658,-1659,-1658,-1658,-1660,-1661,-1663,-1665,-1665,-1666,-1667,-1668,-1668,-1670,-1670,-1670,-1671,-1673,-1674,-1676,-1678,-1680,-1680,-1681,-1682,-1683,-1683,-1684,-1686,-1688,-1688,-1688,-1689,-1689,-1691,-1691,-1691,-1690,-1691,-1692,-1692,-1694,-1695,-1695,-1695,-1696,-1697,-1696,-1696,-1696,-1697,-1696,-1695,-1695,-1694,-1694,-1695,-1694,-1694,-1693,-1692,-1691,-1690,-1690,-1690,-1689,-1688,-1687,-1687,-1685,-1684,-1684,-1683,-1681,-1680,-1680,-1680,-1678,-1677,-1677,-1675,-1675,-1674,-1673,-1671,-1669,-1670,-1667,-1665,-1665,-1665,-1664,-1663,-1663,-1663,-1661,-1661,-1660,-1658,-1658,-1658,-1656,-1656,-1655,-1655,-1655,-1654,-1653,-1653,-1652,-1651,-1650,-1648,-1647,-1647,-1647,-1647,-1647,-1646,-1646,-1645,-1644,-1644,-1643,-1643,-1644,-1644,-1645,-1644,-1644,-1644,-1646,-1646,-1647,-1649,-1651,-1651,-1652,-1653,-1654,-1654,-1655,-1656,-1657,-1658,-1659,-1661,-1662,-1661,-1661,-1661,-1661,-1663,-1665,-1666,-1668,-1668,-1670,-1671,-1672,-1672,-1673,-1673,-1672,-1673,-1675,-1676,-1678,-1680,-1681,-1681,-1682,-1684,-1686,-1688,-1687,-1689,-1690,-1691,-1691,-1690,-1690,-1690,-1691,-1691,-1691,-1693,-1694,-1695,-1695,-1695,-1695,-1696,-1696,-1697,-1697,-1697,-1695,-1694,-1694,-1694,-1694,-1694,-1693,-1692,-1691,-1690,-1690,-1689,-1688,-1688,-1688,-1688,-1687,-1687,-1686,-1685,-1684,-1681,-1679,-1678,-1677,-1678,-1677,-1677,-1676,-1675,-1673,-1671,-1667,-1665,-1665};
//        chprintf((BaseSequentialStream*)&global.sercanmux1,"int fftInput\n");
    // vector<complex<float>> outFftInput;
    // vector<complex<float>> outFftInput = {-1653,-1653,-1652,-1652,-1651,-1649,-1649,-1647,-1647,-1646,-1645,-1646,-1648,-1646,-1646,-1646,-1644,-1644,-1644,-1644,-1645,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1653,-1653,-1654,-1655,-1656,-1658,-1658,-1658,-1659,-1660,-1661,-1661,-1662,-1663,-1663,-1665,-1665,-1665,-1667,-1668,-1667,-1667,-1669,-1670,-1671,-1672,-1674,-1674,-1676,-1678,-1680,-1681,-1681,-1681,-1682,-1684,-1685,-1686,-1687,-1688,-1689,-1690,-1691,-1691,-1690,-1691,-1691,-1691,-1692,-1693,-1693,-1694,-1695,-1695,-1694,-1695,-1694,-1694,-1696,-1695,-1694,-1695,-1696,-1696,-1695,-1695,-1695,-1694,-1693,-1692,-1692,-1691,-1691,-1690,-1689,-1687,-1686,-1685,-1684,-1684,-1684,-1682,-1681,-1680,-1679,-1677,-1676,-1676,-1676,-1675,-1675,-1674,-1671,-1669,-1668,-1667,-1666,-1665,-1664,-1664,-1665,-1663,-1662,-1661,-1660,-1659,-1659,-1659,-1658,-1658,-1658,-1657,-1655,-1655,-1655,-1655,-1654,-1652,-1651,-1650,-1650,-1650,-1649,-1648,-1647,-1646,-1645,-1644,-1644,-1644,-1644,-1644,-1645,-1645,-1644,-1644,-1643,-1643,-1644,-1645,-1646,-1647,-1650,-1652,-1652,-1651,-1652,-1653,-1654,-1656,-1656,-1657,-1657,-1658,-1660,-1662,-1662,-1662,-1663,-1663,-1663,-1664,-1665,-1665,-1666,-1669,-1670,-1670,-1671,-1672,-1673,-1673,-1673,-1675,-1676,-1678,-1681,-1681,-1681,-1683,-1684,-1685,-1686,-1687,-1686,-1687,-1688,-1690,-1690,-1690,-1691,-1691,-1690,-1691,-1693,-1692,-1692,-1694,-1695,-1695,-1697,-1697,-1696,-1697,-1697,-1697,-1697,-1696,-1697,-1697,-1696,-1696,-1697,-1695,-1694,-1694,-1692,-1691,-1691,-1691,-1690,-1689,-1688,-1687,-1685,-1684,-1684,-1682,-1681,-1681,-1680,-1679,-1679,-1679,-1678,-1676,-1675,-1674,-1672,-1670,-1670,-1669,-1668,-1668,-1666,-1666,-1665,-1664,-1662,-1660,-1660,-1660,-1660,-1659,-1659,-1660,-1659,-1657,-1656,-1656,-1654,-1652,-1653,-1653,-1651,-1650,-1651,-1651,-1649,-1649,-1648,-1646,-1645,-1645,-1644,-1644,-1645,-1645,-1644,-1645,-1645,-1645,-1644,-1644,-1646,-1647,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1654,-1654,-1654,-1655,-1656,-1658,-1659,-1660,-1660,-1661,-1660,-1661,-1662,-1664,-1665,-1666,-1667,-1667,-1668,-1668,-1669,-1670,-1670,-1671,-1672,-1673,-1675,-1677,-1679,-1680,-1680,-1681,-1682,-1683,-1684,-1686,-1686,-1687,-1688,-1688,-1688,-1689,-1689,-1690,-1690,-1690,-1690,-1690,-1691,-1693,-1694,-1694,-1693,-1693,-1693,-1693,-1693,-1694,-1695,-1695,-1695,-1694,-1694,-1693,-1693,-1692,-1691,-1691,-1691,-1690,-1690,-1690,-1688,-1687,-1685,-1684,-1683,-1683,-1682,-1682,-1681,-1679,-1678,-1678,-1677,-1676,-1675,-1674,-1672,-1671,-1671,-1668,-1668,-1667,-1665,-1665,-1664,-1663,-1662,-1661,-1660,-1660,-1659,-1658,-1658,-1658,-1656,-1655,-1656,-1655,-1655,-1654,-1652,-1651,-1650,-1651,-1650,-1649,-1648,-1648,-1647,-1646,-1646,-1646,-1644,-1643,-1645,-1644,-1643,-1644,-1645,-1644,-1643,-1643,-1644,-1644,-1644,-1645,-1646,-1647,-1647,-1649,-1650,-1650,-1651,-1651,-1651,-1651,-1653,-1655,-1656,-1656,-1658,-1659,-1659,-1660,-1660,-1661,-1662,-1662,-1663,-1663,-1664,-1666,-1666,-1668,-1669,-1669,-1670,-1670,-1671,-1672,-1674,-1675,-1676,-1678,-1679,-1680,-1682,-1682,-1683,-1683,-1683,-1684,-1685,-1686,-1686,-1688,-1689,-1689,-1689,-1689,-1691,-1691,-1691,-1691,-1691,-1692,-1692,-1693,-1693,-1693,-1693,-1693,-1692,-1691,-1692,-1692,-1691,-1690,-1691,-1691,-1690,-1690,-1690,-1689,-1688,-1686,-1684,-1684,-1684,-1683,-1683,-1682,-1680,-1679,-1677,-1676,-1677,-1675,-1673,-1674,-1674,-1672,-1671,-1670,-1668,-1668,-1667,-1664,-1663,-1662,-1661,-1661,-1661,-1662,-1661,-1658,-1657,-1656,-1656,-1655,-1654,-1655,-1654,-1653,-1653,-1652,-1652,-1651,-1650,-1649,-1648,-1647,-1647,-1647,-1647,-1646,-1645,-1644,-1643,-1642,-1642,-1641,-1640,-1640,-1640,-1641,-1642,-1642,-1642,-1642,-1642,-1643,-1643,-1644,-1645,-1646,-1647,-1648,-1649,-1650,-1651,-1653,-1654,-1654,-1654,-1656,-1656,-1657,-1657,-1658,-1659,-1660,-1661,-1661,-1662,-1663,-1665,-1666,-1666,-1667,-1668,-1669,-1670,-1671,-1672,-1673,-1675,-1677,-1679,-1680,-1682,-1683,-1683,-1684,-1685,-1685,-1686,-1687,-1688,-1688,-1689,-1689,-1690,-1691,-1691,-1692,-1692,-1692,-1692,-1693,-1694,-1693,-1695,-1696,-1695,-1695,-1695,-1695,-1694,-1694,-1695,-1695,-1695,-1695,-1694,-1693,-1693,-1692,-1692,-1691,-1689,-1688,-1688,-1687,-1686,-1687,-1686,-1684,-1683,-1682,-1680,-1679,-1679,-1678,-1678,-1677,-1677,-1676,-1675,-1673,-1671,-1669,-1668,-1667,-1666,-1666,-1666,-1666,-1665,-1664,-1664,-1663,-1660,-1658,-1659,-1658,-1656,-1657,-1658,-1656,-1656,-1655,-1654,-1653,-1652,-1650,-1649,-1648,-1649,-1648,-1646,-1647,-1646,-1645,-1645,-1646,-1645,-1643,-1642,-1642,-1642,-1641,-1641,-1642,-1643,-1643,-1644,-1645,-1647,-1647,-1646,-1647,-1648,-1648,-1650,-1651,-1652,-1654,-1655,-1655,-1656,-1657,-1658,-1658,-1659,-1658,-1658,-1660,-1661,-1663,-1665,-1665,-1666,-1667,-1668,-1668,-1670,-1670,-1670,-1671,-1673,-1674,-1676,-1678,-1680,-1680,-1681,-1682,-1683,-1683,-1684,-1686,-1688,-1688,-1688,-1689,-1689,-1691,-1691,-1691,-1690,-1691,-1692,-1692,-1694,-1695,-1695,-1695,-1696,-1697,-1696,-1696,-1696,-1697,-1696,-1695,-1695,-1694,-1694,-1695,-1694,-1694,-1693,-1692,-1691,-1690,-1690,-1690,-1689,-1688,-1687,-1687,-1685,-1684,-1684,-1683,-1681,-1680,-1680,-1680,-1678,-1677,-1677,-1675,-1675,-1674,-1673,-1671,-1669,-1670,-1667,-1665,-1665,-1665,-1664,-1663,-1663,-1663,-1661,-1661,-1660,-1658,-1658,-1658,-1656,-1656,-1655,-1655,-1655,-1654,-1653,-1653,-1652,-1651,-1650,-1648,-1647,-1647,-1647,-1647,-1647,-1646,-1646,-1645,-1644,-1644,-1643,-1643,-1644,-1644,-1645,-1644,-1644,-1644,-1646,-1646,-1647,-1649,-1651,-1651,-1652,-1653,-1654,-1654,-1655,-1656,-1657,-1658,-1659,-1661,-1662,-1661,-1661,-1661,-1661,-1663,-1665,-1666,-1668,-1668,-1670,-1671,-1672,-1672,-1673,-1673,-1672,-1673,-1675,-1676,-1678,-1680,-1681,-1681,-1682,-1684,-1686,-1688,-1687,-1689,-1690,-1691,-1691,-1690,-1690,-1690,-1691,-1691,-1691,-1693,-1694,-1695,-1695,-1695,-1695,-1696,-1696,-1697,-1697,-1697,-1695,-1694,-1694,-1694,-1694,-1694,-1693,-1692,-1691,-1690,-1690,-1689,-1688,-1688,-1688,-1688,-1687,-1687,-1686,-1685,-1684,-1681,-1679,-1678,-1677,-1678,-1677,-1677,-1676,-1675,-1673,-1671,-1667,-1665,-1665};
    // vector<complex<float>> outFftInput = {-1653,-1653,-1652,-1652,-1651,-1649,-1649,-1647,-1647,-1646,-1645,-1646,-1648,-1646,-1646,-1646,-1644,-1644,-1644,-1644,-1645,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1653,-1653,-1654,-1655,-1656,-1658,-1658,-1658,-1659,-1660,-1661,-1661,-1662,-1663,-1663,-1665,-1665,-1665,-1667,-1668,-1667,-1667,-1669,-1670,-1671,-1672,-1674,-1674,-1676,-1678,-1680,-1681,-1681,-1681,-1682,-1684,-1685,-1686,-1687,-1688,-1689,-1690,-1691,-1691,-1690,-1691,-1691,-1691,-1692,-1693,-1693,-1694,-1695,-1695,-1694,-1695,-1694,-1694,-1696,-1695,-1694,-1695,-1696,-1696,-1695,-1695,-1695,-1694,-1693,-1692,-1692,-1691,-1691,-1690,-1689,-1687,-1686,-1685,-1684,-1684,-1684,-1682,-1681,-1680,-1679,-1677,-1676,-1676,-1676,-1675,-1675,-1674,-1671,-1669,-1668,-1667,-1666,-1665,-1664,-1664,-1665,-1663,-1662,-1661,-1660,-1659,-1659,-1659,-1658,-1658,-1658,-1657,-1655,-1655,-1655,-1655,-1654,-1652,-1651,-1650,-1650,-1650,-1649,-1648,-1647,-1646,-1645,-1644,-1644,-1644,-1644,-1644,-1645,-1645,-1644,-1644,-1643,-1643,-1644,-1645,-1646,-1647,-1650,-1652,-1652,-1651,-1652,-1653,-1654,-1656,-1656,-1657,-1657,-1658,-1660,-1662,-1662,-1662,-1663,-1663,-1663,-1664,-1665,-1665,-1666,-1669,-1670,-1670,-1671,-1672,-1673,-1673,-1673,-1675,-1676,-1678,-1681,-1681,-1681,-1683,-1684,-1685,-1686,-1687,-1686,-1687,-1688,-1690,-1690,-1690,-1691,-1691,-1690,-1691,-1693,-1692,-1692,-1694,-1695,-1695,-1697,-1697,-1696,-1697,-1697,-1697,-1697,-1696,-1697,-1697,-1696,-1696,-1697,-1695,-1694,-1694,-1692,-1691,-1691,-1691,-1690,-1689,-1688,-1687,-1685,-1684,-1684,-1682,-1681,-1681,-1680,-1679,-1679,-1679,-1678,-1676,-1675,-1674,-1672,-1670,-1670,-1669,-1668,-1668,-1666,-1666,-1665,-1664,-1662,-1660,-1660,-1660,-1660,-1659,-1659,-1660,-1659,-1657,-1656,-1656,-1654,-1652,-1653,-1653,-1651,-1650,-1651,-1651,-1649,-1649,-1648,-1646,-1645,-1645,-1644,-1644,-1645,-1645,-1644,-1645,-1645,-1645,-1644,-1644,-1646,-1647,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1654,-1654,-1654,-1655,-1656,-1658,-1659,-1660,-1660,-1661,-1660,-1661,-1662,-1664,-1665,-1666,-1667,-1667,-1668,-1668,-1669,-1670,-1670,-1671,-1672,-1673,-1675,-1677,-1679,-1680,-1680,-1681,-1682,-1683,-1684,-1686,-1686,-1687,-1688,-1688,-1688,-1689,-1689,-1690,-1690,-1690,-1690,-1690,-1691,-1693,-1694,-1694,-1693,-1693,-1693,-1693,-1693,-1694,-1695,-1695,-1695,-1694,-1694,-1693,-1693,-1692,-1691,-1691,-1691,-1690,-1690,-1690,-1688,-1687,-1685,-1684,-1683,-1683,-1682,-1682,-1681,-1679,-1678,-1678,-1677,-1676,-1675,-1674,-1672,-1671,-1671,-1668,-1668,-1667,-1665,-1665,-1664,-1663,-1662,-1661,-1660,-1660,-1659,-1658,-1658,-1658,-1656,-1655,-1656,-1655,-1655,-1654,-1652,-1651,-1650,-1651,-1650,-1649,-1648,-1648,-1647,-1646,-1646,-1646,-1644,-1643,-1645,-1644,-1643,-1644,-1645,-1644,-1643,-1643,-1644,-1644,-1644,-1645,-1646,-1647,-1647,-1649,-1650,-1650,-1651,-1651,-1651,-1651,-1653,-1655,-1656,-1656,-1658,-1659,-1659,-1660,-1660,-1661,-1662,-1662,-1663,-1663,-1664,-1666,-1666,-1668,-1669,-1669,-1670,-1670,-1671,-1672,-1674,-1675,-1676,-1678};
    // vector<complex<float>> outFftInput = {-1653,-1653,-1652,-1652,-1651,-1649,-1649,-1647,-1647,-1646,-1645,-1646,-1648,-1646,-1646,-1646,-1644,-1644,-1644,-1644,-1645,-1647,-1648,-1649,-1649,-1650,-1651,-1652,-1653,-1653,-1653,-1654,-1655,-1656,-1658,-1658,-1658,-1659,-1660,-1661,-1661,-1662,-1663,-1663,-1665,-1665,-1665,-1667,-1668,-1667,-1667,-1669,-1670,-1671,-1672,-1674,-1674,-1676,-1678,-1680,-1681,-1681,-1681,-1682,-1684,-1685,-1686,-1687,-1688,-1689,-1690,-1691,-1691,-1690,-1691,-1691,-1691,-1692,-1693,-1693,-1694,-1695,-1695,-1694,-1695,-1694,-1694,-1696,-1695,-1694,-1695,-1696,-1696,-1695,-1695,-1695,-1694,-1693,-1692,-1692,-1691,-1691,-1690,-1689,-1687,-1686,-1685,-1684,-1684,-1684,-1682,-1681,-1680,-1679,-1677,-1676,-1676,-1676,-1675,-1675,-1674,-1671,-1669,-1668,-1667,-1666,-1665,-1664,-1664,-1665,-1663,-1662,-1661,-1660,-1659,-1659,-1659,-1658,-1658,-1658,-1657,-1655,-1655,-1655,-1655,-1654,-1652,-1651,-1650,-1650,-1650,-1649,-1648,-1647,-1646,-1645,-1644,-1644,-1644,-1644,-1644,-1645,-1645,-1644,-1644,-1643,-1643,-1644,-1645,-1646,-1647,-1650,-1652,-1652,-1651,-1652,-1653,-1654,-1656,-1656,-1657,-1657,-1658,-1660,-1662,-1662,-1662,-1663,-1663,-1663,-1664,-1665,-1665,-1666,-1669,-1670,-1670,-1671,-1672,-1673,-1673,-1673,-1675,-1676};
    // vector<complex<float>> outFftInput = { 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0 };
}


// Recursive function of FFT
std::vector<cd> fft(std::vector<cd>& a)
{
    int n = a.size();

    // if input contains just one element
    if (n == 1)
        return std::vector<cd>(1, a[0]);

    // For storing n complex nth roots of unity
    std::vector<cd> w(n);
    for (int i = 0; i < n; i++) {
        double alpha = 2 * M_PI * i / n;
        w[i] = cd(cos(alpha), sin(alpha));
    }

    std::vector<cd> A0(n / 2), A1(n / 2);
    for (int i = 0; i < n / 2; i++) {

        // even indexed coefficients
        A0[i] = a[i * 2];

        // odd indexed coefficients
        A1[i] = a[i * 2 + 1];
    }

    // Recursive call for even indexed coefficients
    std::vector<cd> y0 = fft(A0);

    // Recursive call for odd indexed coefficients
    std::vector<cd> y1 = fft(A1);

    // for storing values of y0, y1, y2, ..., yn-1.
    std::vector<cd> y(n);

    for (int k = 0; k < n / 2; k++) {
        y[k] = y0[k] + w[k] * y1[k];
        y[k + n / 2] = y0[k] - w[k] * y1[k];
    }
    return y;
}


void UserThread::lightOffAll()
{
    for(int i = 0; i < 8; i++)
    {
        global.robot.setLightColor(i, Color::BLACK);
    }
}

msg_t
UserThread::main()
{
    chprintf((BaseSequentialStream*) &global.sercanmux1, "\nIn main()\n");
    chprintf((BaseSequentialStream*) &global.sercanmux1, "\nBefore Sleep 15\n");
    sleepForSec(15);
    chprintf((BaseSequentialStream*) &global.sercanmux1, "\nAfter Sleep 15\n");


    // since we are using the data from one channel, the final input from the microphone
    // which we get in i2s_rx_buf; will be half of that
    // let's say i2s_rx_buff size is 2000
    // so one channel of data will be about 1000
    // we are using only one channel, so we are doing ft with 1000
    // should use this when taking input from microphone
    // it's a vector of complex nubmers
    vector<complex<float>> dftInput;
    dftInput.resize(I2S_BUF_SIZE/2);


    chprintf((BaseSequentialStream*) &global.sercanmux1, "Light Off All\n");
    lightOffAll();

    i2sInit();


    cycleNumber = 0;



    while (!this->shouldTerminate())
    {
        int sleepBeforeRecording = 0;
        chprintf((BaseSequentialStream*) &global.sercanmux1, "\nRecording starting in %d\n", sleepBeforeRecording);
        sleepForSec(sleepBeforeRecording);

        microphoneInput();
        adjustData(dftInput);



        // for how many values of the data we acutally need to calculate the FT
        // we don't care about values that represents frequncy that is more that 800 for example
        int ftRange = 0;
        // to calculate FT of the first ftRange(e.g first 10 values) values, how big the dataset shuld be
        int necessaryBufferSize = 0;
        ftSpecifications(ftRange, necessaryBufferSize);

        vector<complex<float>> dftInputNecessary(dftInput.begin(), dftInput.begin() + necessaryBufferSize);


        vector<complex<float>> dftOutput = computeDft(dftInputNecessary, ftRange + 1);



        // adjust the data for the first ft result index
        dftOutput.at(0) = 0;
        // printFftResult(fftInputTruncated, fftOutput);

//        vector<float> fftOutputAbsolute = {0,1135.58654,1358.63610,4564.55175,3124.11181,1135.00305,690.08038,435.53344,359.72653,303.64587,512.12915,160.51707,199.17561,210.07049,245.58178,138.29951,131.66738,178.47999,99.00965,116.28580,96.63494,103.13957,95.05598,81.16582,78.25346,85.53370,64.28251,41.18144,89.08612,74.96069,84.18582,63.13232,77.08586,75.73348,73.70004,60.91404,51.86021,45.84040,38.94847,27.97813,65.98235,34.58185,46.01984,58.05112,64.51342,108.54753,24.54235,27.73278,76.02766,63.02331,11.38394,8.79509,43.17379,46.83200,46.77915,25.55215,15.86886,60.95971,67.68765,4.54389,32.70319,58.50863,43.78678,56.82464,32.05156,47.65341,53.74621,20.65540,62.29689,52.31645,21.01792,23.66131,53.86732,19.74291,9.58077,31.65172,29.82717,26.68946,37.73799,29.71418,34.79859,23.04648,37.73220,34.42855,24.47649,27.43594,24.66086,19.07671,24.67285,32.92256,9.99378,24.76587,46.68445,4.30438,19.52642,29.74738,27.75021,21.74477,11.74843,34.97548,32.71369,25.63313,29.12622,29.29659,22.71945,4.23972,30.31164,18.86420,16.81499,22.66192,15.08509,15.05417,34.83185,22.93190,31.59764,25.60832,19.69113,32.13681,5.64386,25.26600,11.17863,19.10688,26.75739,24.35874,36.91413,14.45388,15.59971,18.00393,2.64098,15.14873,10.62753,14.11282,20.98804,26.55543,11.22896,39.34543,36.89253,25.48628,38.08795,22.22427,35.33192,9.15398,15.15412,17.99653,24.43990,29.87902,12.95891,2.29028,12.32517,10.03769,25.70044,15.48093,30.33620,40.20777,25.73522,6.43233,15.75259,26.99877,24.68230,11.39602,11.72649,6.92379,18.67016,27.45854,28.68862,27.74070,8.02009,11.99758,5.14344,22.07382,16.48616,20.98086,9.52464,12.19233,10.26001,16.03654,14.80008,15.93072,13.32250,18.54465,7.37183,24.03082,16.02125,19.05282,17.15705,14.98963,20.22596,18.46969,11.44010,9.48563,3.84351,8.75838,17.42191,17.98804,20.01135,19.48734,14.28802,13.76718,17.70730,23.20265,23.49524,19.35447,8.40192,13.95838,17.06327,21.97187,15.80750,16.25995,20.93070,15.51583,21.22384,16.78016,20.25436,6.56640,17.44149,14.41351,19.22742,12.27899,16.68981,8.26856,18.07604,15.09798,15.98129,27.52808,17.00525,20.22955,19.78676,8.34591,7.76133,17.68037,14.78392,14.33714,16.52656,9.31404,23.24482,14.43461,5.13880,8.29393,8.45559,15.49298,14.44519,15.91646,11.09406,10.91911,10.05277,12.64548,14.81782,8.49898,6.82637,16.99791,20.50204,6.64691,8.45424,14.45053,14.05911,9.22116,11.03659,10.92480,16.62648,15.05360,15.48065,9.10398,8.46188,5.51433,14.91740,22.69008,9.62279,16.19551,14.30736,15.11512,17.44508,7.68922,8.15136,20.09874,20.49987,17.27722,28.62774,13.92529,15.53731,16.74232,8.80691,16.28188,12.37586,19.19651,14.54891,17.35512,6.63714,20.26824,16.62825,21.68381,15.69805,21.35045,16.13405,15.72231,23.65569,17.15987,14.02875,7.22036,17.31218,23.96635,22.98090,16.89306,13.70776,13.41512,17.72670,20.97247,16.97541,17.17542,8.29173,4.08567,9.61117,11.42740,18.75764,20.65382,14.96527,17.56876,19.58419,15.59718,23.28360,7.22472,18.28973,13.39337,16.89610,14.28893,15.52311,10.22240,12.06157,9.17924,19.90617,16.49221,22.29700,4.85800,11.57049,8.15477,27.46483,28.40716,27.56348,18.07001,6.93226,11.32075,11.41290,23.60334,27.95220,15.43354,6.16956,25.86453,40.23092,29.90480,19.25693,24.15211,12.93441,12.80164,2.77849,13.03530,30.53557,24.10795,16.64621,14.89914,9.74286,33.80143,22.15656,37.63497,25.64249,39.88339,39.04744,11.16534,26.85547,20.08391,14.29353,10.67641,17.25583,2.77230,18.00082,15.85026,13.80373,37.01491,23.62702,26.34373,18.66083,11.74245,25.21574,7.29103,32.29884,16.31185,26.40189,32.98777,23.02452,34.82846,14.71004,15.27859,21.85267,15.63563,19.82508,30.47277,3.86649,22.82616,30.77114,29.82299,25.50796,32.96940,35.51892,12.42663,21.75348,27.61502,28.98136,18.76648,3.77303,47.59134,26.24151,7.04554,33.10654,24.92718,18.50324,24.48365,27.54336,24.53269,33.42235,38.21719,22.71203,33.62369,29.82418,33.68056,27.15046,29.66362,31.01936,9.91645,22.31679,53.58171,23.60783,21.00846,52.15563,63.64894,21.35130,53.26783,47.44976,24.99580,57.07971,43.96496,58.12771,34.10749,4.70391,66.62628,60.55607,15.89721,26.18955,47.68880,44.82159,41.63823,9.15083,11.50395,62.93771,75.78726,22.60961,26.25051,108.69568,65.21578,58.53554,45.59320,34.32519,66.38587,28.20651,34.38099,46.55830,52.55696,60.79694,74.09638,77.15715,77.82113,62.58272,82.60779,74.71032,89.40252,41.54298,64.64408,85.11981,77.98589,80.61627,95.44821,102.90898,96.52040,116.61956,98.18675,178.82618,131.27717,138.53892,245.31646,210.17016,191.58605,161.94012,511.77206,304.78277,361.95156,435.50210,689.67474,1135.26049,3124.25830,4563.63671,1358.02453,1135.86486};
//        vector<float> frequncyList = {0,64.1283,128.257,192.385,256.513,320.641,384.77,448.898,513.026,577.154,641.283,705.411,769.539,833.667,897.796,961.924,1026.05,1090.18,1154.31,1218.44,1282.57,1346.69,1410.82,1474.95,1539.08,1603.21,1667.33,1731.46,1795.59,1859.72,1923.85,1987.98,2052.1,2116.23,2180.36,2244.49,2308.62,2372.75,2436.87,2501,2565.13,2629.26,2693.39,2757.52,2821.64,2885.77,2949.9,3014.03,3078.16,3142.28,3206.41,3270.54,3334.67,3398.8,3462.93,3527.05,3591.18,3655.31,3719.44,3783.57,3847.7,3911.82,3975.95,4040.08,4104.21,4168.34,4232.46,4296.59,4360.72,4424.85,4488.98,4553.11,4617.23,4681.36,4745.49,4809.62,4873.75,4937.88,5002,5066.13,5130.26,5194.39,5258.52,5322.65,5386.77,5450.9,5515.03,5579.16,5643.29,5707.42,5771.54,5835.67,5899.8,5963.93,6028.06,6092.18,6156.31,6220.44,6284.57,6348.7,6412.83,6476.95,6541.08,6605.21,6669.34,6733.47,6797.6,6861.72,6925.85,6989.98,7054.11,7118.24,7182.36,7246.49,7310.62,7374.75,7438.88,7503.01,7567.13,7631.26,7695.39,7759.52,7823.65,7887.78,7951.9,8016.03,8080.16,8144.29,8208.42,8272.54,8336.67,8400.8,8464.93,8529.06,8593.19,8657.31,8721.44,8785.57,8849.7,8913.83,8977.96,9042.08,9106.21,9170.34,9234.47,9298.6,9362.73,9426.85,9490.98,9555.11,9619.24,9683.37,9747.5,9811.62,9875.75,9939.88,10004,10068.1,10132.3,10196.4,10260.5,10324.6,10388.8,10452.9,10517,10581.2,10645.3,10709.4,10773.5,10837.7,10901.8,10965.9,11030.1,11094.2,11158.3,11222.4,11286.6,11350.7,11414.8,11479,11543.1,11607.2,11671.3,11735.5,11799.6,11863.7,11927.9,11992,12056.1,12120.2,12184.4,12248.5,12312.6,12376.8,12440.9,12505,12569.1,12633.3,12697.4,12761.5,12825.7,12889.8,12953.9,13018,13082.2,13146.3,13210.4,13274.5,13338.7,13402.8,13466.9,13531.1,13595.2,13659.3,13723.4,13787.6,13851.7,13915.8,13980,14044.1,14108.2,14172.3,14236.5,14300.6,14364.7,14428.9,14493,14557.1,14621.2,14685.4,14749.5,14813.6,14877.8,14941.9,15006,15070.1,15134.3,15198.4,15262.5,15326.7,15390.8,15454.9,15519,15583.2,15647.3,15711.4,15775.6,15839.7,15903.8,15967.9,16032.1,16096.2,16160.3,16224.4,16288.6,16352.7,16416.8,16481,16545.1,16609.2,16673.3,16737.5,16801.6,16865.7,16929.9,16994,17058.1,17122.2,17186.4,17250.5,17314.6,17378.8,17442.9,17507,17571.1,17635.3,17699.4,17763.5,17827.7,17891.8,17955.9,18020,18084.2,18148.3,18212.4,18276.6,18340.7,18404.8,18468.9,18533.1,18597.2,18661.3,18725.5,18789.6,18853.7,18917.8,18982,19046.1,19110.2,19174.3,19238.5,19302.6,19366.7,19430.9,19495,19559.1,19623.2,19687.4,19751.5,19815.6,19879.8,19943.9,20008,20072.1,20136.3,20200.4,20264.5,20328.7,20392.8,20456.9,20521,20585.2,20649.3,20713.4,20777.6,20841.7,20905.8,20969.9,21034.1,21098.2,21162.3,21226.5,21290.6,21354.7,21418.8,21483,21547.1,21611.2,21675.4,21739.5,21803.6,21867.7,21931.9,21996,22060.1,22124.2,22188.4,22252.5,22316.6,22380.8,22444.9,22509,22573.1,22637.3,22701.4,22765.5,22829.7,22893.8,22957.9,23022,23086.2,23150.3,23214.4,23278.6,23342.7,23406.8,23470.9,23535.1,23599.2,23663.3,23727.5,23791.6,23855.7,23919.8,23984,24048.1,24112.2,24176.4,24240.5,24304.6,24368.7,24432.9,24497,24561.1,24625.2,24689.4,24753.5,24817.6,24881.8,24945.9,25010,25074.1,25138.3,25202.4,25266.5,25330.7,25394.8,25458.9,25523,25587.2,25651.3,25715.4,25779.6,25843.7,25907.8,25971.9,26036.1,26100.2,26164.3,26228.5,26292.6,26356.7,26420.8,26485,26549.1,26613.2,26677.4,26741.5,26805.6,26869.7,26933.9,26998,27062.1,27126.3,27190.4,27254.5,27318.6,27382.8,27446.9,27511,27575.2,27639.3,27703.4,27767.5,27831.7,27895.8,27959.9,28024,28088.2,28152.3,28216.4,28280.6,28344.7,28408.8,28472.9,28537.1,28601.2,28665.3,28729.5,28793.6,28857.7,28921.8,28986,29050.1,29114.2,29178.4,29242.5,29306.6,29370.7,29434.9,29499,29563.1,29627.3,29691.4,29755.5,29819.6,29883.8,29947.9,30012,30076.2,30140.3,30204.4,30268.5,30332.7,30396.8,30460.9,30525.1,30589.2,30653.3,30717.4,30781.6,30845.7,30909.8,30973.9,31038.1,31102.2,31166.3,31230.5,31294.6,31358.7,31422.8,31487,31551.1,31615.2,31679.4,31743.5,31807.6,31871.7,31935.9};

//        // shuld take the average value of fourier transform result(abs values) upto 16 but no more than that
//        float threshold = ftThreshold(fftOutput);
//        // here we want to go upto <900 values and the the hightes value under 900 is at index 15
//        for(int i = 0; i < (ftRange + 1); i++)
//        {
//            int lightIndex = floor(frequencyList.at(i) / 100);

//            if(std::abs(fftOutput.at(i)) > threshold)
//            {
//                chprintf((BaseSequentialStream*) &global.sercanmux1, "\nLight index i : %d, RED\n", lightIndex);
//                global.robot.setLightColor(lightIndex, Color::RED);
//            }
//            else
//            {
//                chprintf((BaseSequentialStream*) &global.sercanmux1, "\nLight index i : %d, BLACK\n", lightIndex);
//                global.robot.setLightColor(lightIndex, Color::BLACK);
//            }

//        }


        lightTillHighestFrequency(dftOutput, ftRange);

        motorControl(dftOutput);

    }

    chprintf((BaseSequentialStream*) &global.sercanmux1, "After While\n");

    return RDY_OK;
}
