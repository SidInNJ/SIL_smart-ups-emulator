//  Helper Functions

#include "HandyHelpers.h"
#include <string.h>


uint16_t HandyHelpers::anaFilter_ms(uint8_t AnaInChan, uint16_t ms) // 100ms: 6 full-waves of AC power at 60Hz, 5 at 50Hz.
{
    uint32_t readingsSum = 0;
    uint32_t readCount = 0;
    uint32_t startTime = millis();

    while ((millis() - startTime) > ms)
    {
        readingsSum += analogRead(AnaInChan);
        readCount++;
    }

    if (readCount)      // Avoid possible div by 0 fault
        return readingsSum / readCount;
    else
        return analogRead(AnaInChan);
}

//Protos:
int compare_int16(const void *cmp1, const void *cmp2);
//
// 29 is good value for numSamples as it takes the time of a full-wave of AC power.
uint16_t HandyHelpers::anaFilter_Mid(uint8_t AnaInChan, uint8_t numSamples)
{
    #define MAX_ALLOWABLE_SAMPLES 60

    if (numSamples > MAX_ALLOWABLE_SAMPLES)
    {
        numSamples = MAX_ALLOWABLE_SAMPLES;
    }
    uint16_t readingsSorted[MAX_ALLOWABLE_SAMPLES];

    m_serialPtr->flush();
    //uint32_t startTime = millis();

#define DO_MINSAMPLECHECK false //DOYET DEBUG

#if DO_MINSAMPLECHECK
    if (numSamples > MAX_ALLOWABLE_SAMPLES)
    {
        m_serialPtr->println(F("Programming ERROR: anaFilter_Mid(): Requested more samples than array can hold."));
    }

    if (numSamples <3)
    {
        m_serialPtr->println(F("Programming Warning: anaFilter_Mid(): Requested less than 3 samples; increased to 3."));
        numSamples = 3;;
    }
#endif

    readingsSorted[0] = analogRead(AnaInChan);  // 1 dummy read for settling

    for (uint8_t i=0; i<numSamples; i++)
    {
        readingsSorted[i] = analogRead(AnaInChan);
        if (numSamples>1)
        {
            delayMicroseconds(425);
        }
    }

    //uint32_t SampleDuration = millis()-startTime;
    
    #define DEBUG_DISP_PRESORT_SENSE_ARRAY false
    //#define DEBUG_DISP_PRESORT_SENSE_ARRAY true
    #if DEBUG_DISP_PRESORT_SENSE_ARRAY
    m_serialPtr->print(F("Readings: "));

        for (uint8_t i=0; i<numSamples; i++)
        {
            m_serialPtr->print(readingsSorted[i]);  m_serialPtr->print(F(", "));
        }
        m_serialPtr->println(F(""));
    #endif

    // qsort - last parameter is a function pointer to the sort function
    qsort(readingsSorted, numSamples, sizeof(uint16_t), compare_int16);

    //uint32_t DoneSortTime = millis();

    //m_serialPtr->println("Filter Sample time: " + String(SampleDuration));
    //m_serialPtr->println("FilterTotal time: " + String(DoneSortTime-startTime));

    #define DEBUG_DISP_SENSE_ARRAY false
    //#define DEBUG_DISP_SENSE_ARRAY true     // DEBUG
    #if DEBUG_DISP_SENSE_ARRAY
    m_serialPtr->print(F("Readings: "));

        for (uint8_t i=0; i<numSamples; i++)
        {
            m_serialPtr->print(readingsSorted[i]);
            m_serialPtr->print(F(", "));
        }
        m_serialPtr->println(F(""));
    #endif

    return readingsSorted[(numSamples/2)];   // Return the middle of the sorted samples
}

#define NO_SOFTWARE_SERIAL_LCD      // Turns off #include <SoftwareserialPtr->h> in HandyHelpers.h
#ifndef NO_SOFTWARE_SERIAL_LCD

// Limit length of string being printed to the LCD to a maxLen by cutting off the beginning, blank fill any shortcoming.
void SoftSerial_LCD::print_maxLenPlusBlanks(const char *s, uint8_t maxLen)
{
    uint8_t lenRFID = strlen(s);
    if (lenRFID > maxLen)
    {
        s += lenRFID-maxLen;    // Too long, cut off the beginning.
    }

    this->print(s);

    if (lenRFID < maxLen)
    {
        for (uint8_t i = maxLen-lenRFID; i; i--)
        {
            this->print(F(" "));
        }
    }
}
#endif //#ifndef NO_SOFTWARE_SERIAL_LCD

// qsort requires a comparison function
int compare_int16(const void *cmp1, const void *cmp2)
{
  // Need to cast the void * to int *
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  // The comparison
  //return a > b ? -1 : (a < b ? 1 : 0);
  // A simpler, probably faster way:
  return a - b;
}

// Change 16 bit var
bool HandyHelpers::updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, uint16_t &varLocn_16, const char *varName, bool isHex, uint16_t minVal)
{
    uint8_t temp;
    return updateFromUserInput_8or16(userIn, indexUserIn, inByte, maxAllowableValue, false, temp, varLocn_16, varName, isHex, minVal);
}

//bool HandyHelpers::updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, UsersActive &varLocn_UQ, const char *varName)
//{
//    uint8_t temp;
//    return updateFromUserInput_8or16(userIn, indexUserIn, inByte, maxAllowableValue, sizeof(varLocn_UQ)==1, temp, (uint16_t &)varLocn_UQ, varName);
//}

// Change 8 bit var
bool HandyHelpers::updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, uint8_t &varLocn_8, const char *varName, bool isHex, uint8_t minVal)
{
    uint16_t temp;
    if ((maxAllowableValue <=255) && (minVal<maxAllowableValue))
    {
        return updateFromUserInput_8or16(userIn, indexUserIn, inByte, maxAllowableValue, true, varLocn_8, temp, varName, isHex, (uint16_t)minVal);
    }
    else
    {
        if (maxAllowableValue > 255)
        {
            m_serialPtr->print(F("PROGRAMMING ERROR: Max value for updateFromUserInput too large for uint_8 variable. Provided max was: "));
            m_serialPtr->println(String(maxAllowableValue));
        }
        if (minVal>=maxAllowableValue)
        {
            m_serialPtr->print(F("PROGRAMMING ERROR: Max value <= Min Value for updateFromUserInput. Provided min was: "));
            m_serialPtr->println(String(minVal));
        }
        return false;
    }
}

// signed 16 bit in6
bool HandyHelpers::updateFromUserInput(char *userIn, uint8_t& indexUserIn, int& inByte, int16_t maxAllowableValue, int16_t& varLocn_16, const char *varName, bool isHex, int16_t minVal)
{
    bool rc = false;
    uint8_t base = (isHex ? 16 : 10);
    if ((inByte == ASCII_CR) || (inByte == ASCII_LF))
    {
        int16_t val = strtol(&userIn[1], 0, base);
        if (val > maxAllowableValue)
        {
            val = 0;
            m_serialPtr->print(F("Error: Invalid "));
            m_serialPtr->print(varName);
            m_serialPtr->print(F(" value! Max is "));
            m_serialPtr->print(maxAllowableValue);
            m_serialPtr->println(F(".\n"));
        }
        else if (val < minVal)
        {
            val = 0;
            m_serialPtr->print(F("Error: Invalid \""));
            m_serialPtr->print(varName);
            m_serialPtr->print(F("\" value! Min is "));
            m_serialPtr->print(minVal);
            m_serialPtr->println(F(".\n"));
        }
        else
        {
            m_serialPtr->print(F("Old value was: "));
            if (isHex)
            {
                m_serialPtr->print(F("0x"));
            }

            m_serialPtr->println(String(varLocn_16, base));
            varLocn_16 = val;

            m_serialPtr->print(F("New "));
            m_serialPtr->print(varName);
            m_serialPtr->print(F(" set to: "));
            if (isHex)
            {
                m_serialPtr->print(F("0x"));
            }
            m_serialPtr->println(val, base);
            m_serialPtr->println(F("Don't forget to (S)ave the new value!"));
            rc = true;
        }
        indexUserIn = 0;
    }
    return rc;
}

bool HandyHelpers::updateFromUserInput_8or16(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, bool is8bit, uint8_t &varLocn_8, uint16_t &varLocn_16, const char *varName, bool isHex, uint16_t minVal)
{
    bool rc = false;
    uint8_t base = (isHex ? 16 : 10);
    if ((inByte == ASCII_CR) || (inByte == ASCII_LF))
    {
        uint16_t val = strtol(&userIn[1], 0, base);
        if (val > maxAllowableValue)
        {
            val = 0;
            m_serialPtr->print(F("Error: Invalid "));
            m_serialPtr->print(varName);
            m_serialPtr->print(F(" value! Max is "));
            m_serialPtr->print(maxAllowableValue);
            m_serialPtr->println(F(".\n"));
        }
        else if (val < minVal)
        {
            val = 0;
            m_serialPtr->print(F("Error: Invalid \""));
            m_serialPtr->print(varName);
            m_serialPtr->print(F("\" value! Min is "));
            m_serialPtr->print(minVal);
            m_serialPtr->println(F(".\n"));
        }
        else
        {
            m_serialPtr->print(F("Old value was: "));
            if(isHex)
            {
                m_serialPtr->print(F("0x"));
            }

            if (is8bit)
            {
                m_serialPtr->println(String(varLocn_8, base));
                varLocn_8 = val;
            } 
            else
            {
                m_serialPtr->println(String(varLocn_16, base));
                varLocn_16 = val;
            }

            m_serialPtr->print(F("New "));
            m_serialPtr->print(varName );
            m_serialPtr->print(F(" set to: "));
            if(isHex)
            {
                m_serialPtr->print(F("0x"));
            }
            m_serialPtr->println(val, base);
            m_serialPtr->println(F("Don't forget to (S)ave the new value!"));
            rc = true;
        }
        indexUserIn = 0;
    }
    return rc;
}

// Change bool var
bool HandyHelpers::updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, bool &varLocn_bool, const char *varName)
{
    bool rc = false;

    maxAllowableValue = maxAllowableValue;  // squelch "unused" warning

    if ((inByte == ASCII_CR) || (inByte == ASCII_LF))
    {
        uint16_t val = atoi(&userIn[1]);
        //if (val > maxAllowableValue)
        if (val > 1)
        {
            val = 0;
            m_serialPtr->print(F("Error: Invalid "));
            m_serialPtr->print(varName);
            m_serialPtr->println(F(" value! Max is 1 (for a bool).\n"));
        }
        else
        {
            m_serialPtr->print(F("Old value was: "));
            m_serialPtr->println(String(varLocn_bool));

            varLocn_bool = val;
            
            m_serialPtr->print(F("New "));
            m_serialPtr->print(varName );
            m_serialPtr->print(F(" set to: "));
            m_serialPtr->println(varLocn_bool);
            m_serialPtr->println(F("Don't forget to (S)ave the new value!"));
            rc = true;
        }
        indexUserIn = 0;
    }
    return rc;
}

//    bool goodValue = MH.updateIndex_16bit(userIn, indexUserIn, inByte, 4, 1023, slotNum, newValue, "Car present ambient light threshold");
bool HandyHelpers::updateIndex_16bit(char *userIn, uint8_t &indexUserIn, int &inByte, uint8_t maxIndex, uint16_t maxAllowableValue, uint8_t &memNum_8, uint16_t &varLocn_16, const char *varName)
{
    bool rc = false;
    if ((inByte == ASCII_CR) || (inByte == ASCII_LF))
    {        
        char *field = strtok(&userIn[1], ",.");
        uint16_t newIndex = atoi(field);
        if (newIndex > maxIndex)
        {
            newIndex = 0;
            m_serialPtr->print(F("Error: Invalid member number provided! Max is "));
            m_serialPtr->println(maxIndex);
        }
        else
        {
            field = strtok(NULL, ",");
            uint16_t val = atoi(field);
            if (val > maxAllowableValue)
            {
                val = 0;
                m_serialPtr->print(F("Error: Invalid value! Max is "));
                m_serialPtr->print(maxAllowableValue);
                m_serialPtr->print(F(".\n"));
            }
            else
            {
                memNum_8 = newIndex;    // Caller will set actual values in place
    
                m_serialPtr->println(String(val));
                varLocn_16 = val;
    
                m_serialPtr->print(F("Member "));
                m_serialPtr->print(newIndex );
                m_serialPtr->print(F(" of "));
                m_serialPtr->print(varName );
                m_serialPtr->println(F(" set to: "));
                m_serialPtr->print(val);
                m_serialPtr->println(F("Don't forget to (S)ave the new value!"));
                rc = true;
            }
        }
        indexUserIn = 0;
    }
    return rc;
}

// Convert IP Address string into 32 bit IP address. 
// Optional valStrOffset is normally 1 unless it is a 2 char command ("EG192.168.1.1" for Ethernet Gateway)
void HandyHelpers::UserIPAddressEntry(char userIn[MAX_MENU_CHARS], uint8_t &indexUserIn, char cmdChar, IPAddress &destIpAddr, uint32_t &eeAddressStorage, uint8_t valStrOffset)
{
    if ((indexUserIn>=2) && (indexUserIn<MAX_MENU_CHARS))
    {
        userIn[indexUserIn] = 0;  // Null out the CR at the end of the string for sake of .fromString()
        
        IPAddress ipTemp;
        bool isGood = ipTemp.fromString(&userIn[valStrOffset]);
        if (isGood == false)
        {
            m_serialPtr->print(F("Bad IP format. Proper usage: "));
            m_serialPtr->print(cmdChar);
            m_serialPtr->println(F("10.0.40.242"));
            m_serialPtr->print(F("Your entry: " ));
            m_serialPtr->println(&userIn[0]);
        }
        else
        {
            m_serialPtr->print(F("Previous IP Address: "));
            printParsedNumberEndian(destIpAddr, 4, '.');
            //destIpAddr.printTo(SERIALPORT);
            destIpAddr.fromString(&userIn[valStrOffset]);     // Convert from string to IP address
            m_serialPtr->print(F("\nNew IP Address     : "));
            printParsedNumberEndian(destIpAddr, 4, '.');
            //destIpAddr.printTo(SERIALPORT);
            m_serialPtr->println(F("\nMust Save and power-cycle/reset (#) to take effect."));
            eeAddressStorage = destIpAddr;    // Get 32 bit IP address
        }
    }
    indexUserIn = 0;                        
}

void HandyHelpers::printParsedBytes(uint8_t *byteArray, uint8_t numFields, char delim, uint8_t base)
{
    int8_t i=0;
    for (; i<(numFields-1); i++)
    {
        m_serialPtr->print(String(byteArray[i], base) + delim);
    }

    m_serialPtr->print(String(byteArray[i], base));  
}

void HandyHelpers::printParsedNumber(uint32_t num, uint8_t numFields, char delim)
{
    if (numFields>4)
    {
        m_serialPtr->println(F("ERROR: Bad numFields #."));
    }

    for (int8_t i=3; i; i--)
    {
        if (numFields>i)
        {
            uint32_t numField = num >> (i*8);
            m_serialPtr->print(String(numField) + delim);
            num -= numField << (i*8);
        }
    }

    m_serialPtr->print(String(num));  
}

void HandyHelpers::printParsedNumberEndian(uint32_t num, uint8_t numFields, char delim)
{
    if (numFields>4)
    {
        m_serialPtr->println(F("ERROR: Bad numFields #."));
    }

    for (int8_t i=0; i<(numFields-1); i++)
    {
            m_serialPtr->print(String(num & 0xFF) + delim);
            num = num >> 8;
    }

    m_serialPtr->print(String(num));  
}

// Divide by 10 and print with tenths: 123 prints as "12.3". Good for storing # accurate to 10ths by multiplying it by 10.
void HandyHelpers::printDivBy10(uint16_t num)
{
    uint16_t numDiv10 = num / 10;
    uint16_t unitsOnly = num - (numDiv10 * 10);

    m_serialPtr->print(numDiv10);  
    m_serialPtr->print(F("."));  
    m_serialPtr->print(unitsOnly);  
}

void HandyHelpers::printNumPadBlanks(uint16_t num, uint8_t numDigits) // Pad on left with blanks to take up numDigits characters.
{
    uint16_t t = num;

    if (t==0)
    {
        numDigits--;
    }
    else
    {
        for (; numDigits && t; numDigits--)
        {
            t /= 10;
        }
    }

    for (; numDigits; numDigits--)
    {
        m_serialPtr->print(F(" "));  
    }
    m_serialPtr->print(num);  
}


bool HandyHelpers::updateFromUserInputWFields(char userIn[MAX_MENU_CHARS], uint8_t &indexUserIn, char inByte, uint32_t &eeAddressStorage, const char *varName, uint8_t numFields, char delim)
{
    bool rc = false;
    uint32_t priorVal = eeAddressStorage;

    if ((inByte == ASCII_CR) || (inByte == ASCII_LF))
    {
        if ((indexUserIn>=1) && (indexUserIn<MAX_MENU_CHARS))
        {
            if ((userIn[indexUserIn - 1] == ASCII_CR) || (userIn[indexUserIn - 1] == ASCII_LF))
            {
            userIn[indexUserIn-1] = 0;  // Null out the CR at the end of the string for sake of .fromString()
            }

            bool isGood = uint32FromStringOfFields(&userIn[1], eeAddressStorage, numFields);
            if (isGood == false)
            {
                m_serialPtr->print(F("Bad Number format. Proper usage: "));
                m_serialPtr->print(inByte);
                printParsedNumber(0x0122, numFields, delim);
                m_serialPtr->print(F("\nYour entry: " ));
                m_serialPtr->println(&userIn[0]);
            }
            else
            {
                m_serialPtr->print(F("Previous "));  m_serialPtr->print(varName); m_serialPtr->print(F(": "));
                printParsedNumber(priorVal, numFields, delim);
                m_serialPtr->print(F("\nNew: "));
                printParsedNumber(eeAddressStorage, numFields, delim);
                m_serialPtr->println(F("\nDon't forget to (S)ave the new value!"));
                rc = true;
            }
        }
        indexUserIn = 0;                        
    }
    return rc;
}

// Translate string of comma/dot/space delimited numbers into a uint32_t (4 byte word)
// Return true if OK. Supply char string, destination of number, number of delimited fields
bool HandyHelpers::uint32FromStringOfFields(const char *numString, uint32_t &destNum, uint8_t numFields, bool showErrors)
{
    uint16_t acc = 0; // Accumulator
    int8_t desiredDots = numFields-1;
    union
    {
        uint8_t bytes[4];  // Up to fields (bytes)
        uint32_t dword;
    } resultNum;

    resultNum.dword = 0;

    if (desiredDots < 0)
    {
        if (showErrors)
        {
        m_serialPtr->println(F("ERROR: Programming - bad # of fields specified to uint32FromString()"));
        }
        return false;
    }

    int8_t dots = 4-numFields;

    while (*numString)
    {
        char c = *numString++;
        if (c >= '0' && c <= '9')
        {
            acc = acc * 10 + (c - '0');
            if (acc > 255)
            {
                // Value out of [0..255] range
                if (showErrors)
                {
                m_serialPtr->print(F("Error: Number field out of range. Should be 0-255, is: "));
                m_serialPtr->println(acc);
                }
                return false;
            }
        }
        else if ((c == '.') || (c == ',') || (c == ' '))
        {
            if (dots == 4)
            {
                // Too many dots/commas/spaces (there must be # supplied)
                if (showErrors)
                {
                m_serialPtr->println(F("Error: Too many number fields!"));
                }
                return false;
            }
            resultNum.bytes[3-dots++] = acc;
            //m_serialPtr->println("This byte: " + String(resultNum.bytes[3-(dots-1)]) + ", dots=" + String(dots));            
            acc = 0;
        }
        else
        {
            // Invalid char
            if (showErrors)
            {
            m_serialPtr->println(F("Error: Bad Character. Should be 0-9, comma, period or space."));
            }
            return false;
        }
    }

    if (dots != 3)
    {
        // Too few dots (there must be desiredDots dots)
        if (showErrors)
        {
            m_serialPtr->print(F("Error: Too few number fields! Fields seen:"));
            m_serialPtr->println(String(dots));
        }
        return false;
    }
    resultNum.bytes[0] = acc;
    destNum = resultNum.dword;
    return true;
}

// Convert string of numbers into an array of ui32_t's. Returns the number of converted numbers.
uint8_t HandyHelpers::uint32sFromStringOfFields(const char *numString, uint32_t destNum[MAX_NUMS2CONVERT], bool showErrors)
{
    uint8_t numsConverted = 0;

    uint16_t acc = 0; // Accumulator
    //union
    //{
    //    uint8_t bytes[4];  // Up to fields (bytes)
    //    uint32_t dword;
    //} resultNum;

    for (uint8_t i = 0; i < MAX_NUMS2CONVERT; i++)
    {
        destNum[i] = 0;
    }

    while (*numString)
    {
        char c = *numString++;
        if (c >= '0' && c <= '9')
        {
            acc = acc * 10 + (c - '0');

            c = *numString; // look ahead
            if ((c == '.') || (c == ',') || (c == ' ') || (c == 0x00))
            {
                if (numsConverted >= MAX_NUMS2CONVERT)
                {
                    m_serialPtr->print(F("Error: Too many number fields! "));
                    return 0;
                }
                destNum[numsConverted++] = acc;
                //m_serialPtr->println("This num: " + String(acc) + ");
                acc = 0;
                numString++;    // Skip over the delimiter
                if (c == 0x00)
                    break;      // If end of string, we're done
            }
        }
        else
        {
            // Invalid char
            if (showErrors)
            {
            m_serialPtr->println(F("Error: Bad Character. Should be 0-9, comma, period or space."));
            }
            return false;
        }
    }

    return numsConverted;
}

// Convert an unsigned 16-bit integer to a string. Please provide a character array large enough for the result! 6 chars long should do it.
void toString(char str[], uint16_t num)
{
    uint8_t  i, len = 0;
    uint16_t rem, n;
 
    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}

// Convert an unsigned 16-bit integer to a string. Please provide a character array large enough for the result! 5 chars long should do it.
void toHexString(char str[], uint16_t num)
{
    uint8_t  i, len = 0;
    uint16_t rem, n;
 
    n = num;
    while (n != 0)
    {
        len++;
        n /= 16;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 16;
        num = num / 16;
        if (rem <10)
            str[len - (i + 1)] = rem + '0';
        else
            str[len - (i + 1)] = rem - 10 + 'A';
    }
    str[len] = '\0';
}



#if (false)

// Reduce color values so sum does not exceed maxIntensity
uint32_t HandyHelpers::reduceToMaxIntensity(uint32_t proposedColor, uint16_t maxIntensity)
{
    uint32_t newColor = proposedColor;
    uint16_t r = (newColor >> 16) & 0xFF;
    uint16_t g = (newColor >>  8) & 0xFF;
    uint16_t b = newColor & 0xFF;

    uint16_t colorSum = r + g + b;

    if (colorSum > maxIntensity)
    {
        float proportion = (float)maxIntensity / (float)colorSum;
        r = (float)r * proportion;
        g = (float)g * proportion;
        b = (float)b * proportion;

        newColor = ((uint32_t)r << 16) + (g << 8) + b;

        m_serialPtr->print(F("Reduced color intensity from ("));
        printParsedNumber(proposedColor);
        m_serialPtr->print(F(") to ("));
        printParsedNumber(newColor);
        m_serialPtr->println(F(")"));
    }
    else
    {
        //SERIALPORT_PRINT(F("Color not too bright. Sum: "));
        //SERIALPORT_PRINT(colorSum);
        //SERIALPORT_PRINT(F(", New Color:  "));
        //MH.printParsedNumber(newColor);
        //SERIALPORT_PRINT(F(", R: ")); SERIALPORT_PRINT(r);
        //SERIALPORT_PRINT(F(", G: ")); SERIALPORT_PRINT(g);
        //SERIALPORT_PRINT(F(", B: ")); SERIALPORT_PRINT(b);
        //SERIALPORT_PRINTLN();
    }
    return newColor;
}

// Return  index of table member closest to supplied value
uint8_t HandyHelpers::valueToTableIndex(int32_t val, int16_t table[], uint8_t tblCount)  
{
    uint8_t i = 0;
    int32_t diff;
    int32_t lastDiff = ABS((int32_t)table[0] - val);
    for (i = 1; i < tblCount; i++)
    {
        diff = ABS((int32_t)table[i] - val);
        if (lastDiff < diff)
        {
            i--;    // Last diff was better, stop with that one.
            return i;
        }
        lastDiff = diff;
    }
    return i-1;
}


uint32_t disolveDot(uint32_t colorOld, uint32_t colorNew, uint16_t ratioNew256)
{
    #define RESOLUTION_MULTI 256
    if (ratioNew256 > RESOLUTION_MULTI)
    {
        ratioNew256 = RESOLUTION_MULTI;
        SERIALPORT_PRINTLN(F("Programming Warning: ratioNew256 > 256."));
    }
    unsigned long multOld = RESOLUTION_MULTI - ratioNew256;
    unsigned long multNew = ratioNew256;

    unsigned long whtOld = (colorOld >> 24) & 0xFF;
    unsigned long redOld = (colorOld >> 16) & 0xFF;
    unsigned long grnOld = (colorOld >>  8) & 0xFF;
    unsigned long bluOld = colorOld & 0xFF;

    unsigned long whtNew = (colorNew >> 24) & 0xFF;
    unsigned long redNew = (colorNew >> 16) & 0xFF;
    unsigned long grnNew = (colorNew >>  8) & 0xFF;
    unsigned long bluNew = colorNew & 0xFF;

    unsigned long whtDis = (whtOld * multOld + whtNew * multNew) / RESOLUTION_MULTI;
    unsigned long redDis = (redOld * multOld + redNew * multNew) / RESOLUTION_MULTI;
    unsigned long grnDis = (grnOld * multOld + grnNew * multNew) / RESOLUTION_MULTI; 
    unsigned long bluDis = (bluOld * multOld + bluNew * multNew) / RESOLUTION_MULTI;
       
    return(((whtDis & 0xFF) << 24) + ((redDis & 0xFF) << 16) + ((grnDis & 0xFF) <<8) + (bluDis & 0xFF));

}


uint32_t disolveColor(uint32_t colorOld, uint32_t colorNew, unsigned long startTime, unsigned long dissolveDuration)
{
//#define GRADUATIONS DUR_DONE
#define GRADUATIONS dissolveDuration
    unsigned long timeNow = millis();
    unsigned long timeDiff = timeNow - startTime;

    if ((timeDiff >=dissolveDuration) || (dissolveDuration == 0))
    {
        return colorNew;
    }

    unsigned long multOld = ((dissolveDuration - timeDiff) * GRADUATIONS)/dissolveDuration;
    unsigned long multNew = ((dissolveDuration - (dissolveDuration - timeDiff)) * GRADUATIONS)/dissolveDuration;

    unsigned long whtOld = (colorOld >> 24) & 0xFF;
    unsigned long redOld = (colorOld >> 16) & 0xFF;
    unsigned long grnOld = (colorOld >>  8) & 0xFF;
    unsigned long bluOld = colorOld & 0xFF;

    unsigned long whtNew = (colorNew >> 24) & 0xFF; 
    unsigned long redNew = (colorNew >> 16) & 0xFF; 
    unsigned long grnNew = (colorNew >>  8) & 0xFF; 
    unsigned long bluNew = colorNew & 0xFF;

    unsigned long whtDis = (whtOld * multOld + whtNew * multNew) / GRADUATIONS;
    unsigned long redDis = (redOld * multOld + redNew * multNew) / GRADUATIONS;
    unsigned long grnDis = (grnOld * multOld + grnNew * multNew) / GRADUATIONS; 
    unsigned long bluDis = (bluOld * multOld + bluNew * multNew) / GRADUATIONS;

    return(((whtDis & 0xFF) << 24) + ((redDis & 0xFF) << 16) + ((grnDis & 0xFF) <<8) + (bluDis & 0xFF));
}


// Disolve (change the value) a 16 bit number over time (convert to 32bit on the way in)
uint16_t disolve16BitNum(uint32_t numOld, uint32_t numTarget, unsigned long startTime, unsigned long dissolveDuration)
{
    unsigned long timeNow = millis();
    unsigned long timeDiff = timeNow - startTime;

    if ((timeDiff >=dissolveDuration) || (dissolveDuration == 0))
    {
        return (uint16_t)numTarget;
    }

    unsigned long multOld = dissolveDuration - timeDiff;
    unsigned long multNew = timeDiff;

    return ((numOld * multOld) + (numTarget * multNew)) / dissolveDuration;
}

uint32_t Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) 
{
    return ((uint32_t)white << 24) | ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
}


// Convert a linear lighting "curve" to a squared lighting curve
// Take the square root of the linear: 0 stays 0, max value stays max value, curve in between
uint16_t squaredCurve(uint16_t newValLinear)
{
    float newValSquared = (float)newValLinear / (float)SQRT_MAX_PWM_VALUE; // map it to be square-able (max is square root of max PWM value)
    newValSquared *= newValSquared;   // Squared function as it looks more linear to the eye (real 25% looks like 50% to eye)

    return (uint16_t) newValSquared;
}

#endif // false

/* This function places the current value of the heap and stack pointers in the
 * variables. You can call it from any place in your code and save the data for
 * outputting or displaying later. This allows you to check at different parts of
 * your program flow.
 * The stack pointer starts at the top of RAM and grows downwards. The heap pointer
 * starts just above the static variables etc. and grows upwards. SP should always
 * be larger than HP or you'll be in big trouble! The smaller the gap, the more
 * careful you need to be. Julian Gall 6-Feb-2009.
 */

int check_mem(bool doPrint) 
{
    //#ifdef ARDUINO_ARCH_SAMD || TEENSYDUINO
    #if defined(ARDUINO_ARCH_SAMD) || defined(TEENSYDUINO)
        return 0;
    #else
        uint8_t * heapptr, * stackptr;
        stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
        heapptr = stackptr;                     // save value of heap pointer
        free(stackptr);      // free up the memory again (sets stackptr to 0)
        stackptr =  (uint8_t *)(SP);           // save value of stack pointer
    
        if (doPrint)
        {
            SERIALPORT_PRINT(F("\n Avail RAM: "));
            SERIALPORT_PRINT((long)(stackptr-heapptr));
            SERIALPORT_PRINT(F(", Stack="));
            SERIALPORT_PRINT((long)stackptr);
            SERIALPORT_PRINT(F(", Heap="));
            SERIALPORT_PRINTLN((long)heapptr);
        }
    
        return(int) ((long)(stackptr-heapptr));
    #endif
}


//#if defined(__AVR_ATmega2560__) || defined(TEENSYDUINO)
//    #define EEPROM_END_ADDR 0x1000  // 4k (actual size of EEPROM)
//#elif defined(__AVR_ATmega328P__)
//    #define EEPROM_END_ADDR 0x400   // 1k (actual size of EEPROM)
//#endif

// Below based on: https://forum.arduino.cc/index.php?topic=183225.
// Intent is to dump exhibit EEPROMs at/before installs, be able to
// restore EEPROM if the arduino needs to be replaced.
void dumpEEProm(Stream *serialPtr)
{
    uint8_t i, c;
    uint16_t addr = 0;

    serialPtr->print(F("\r\n$$-EEPROM CONTENTS:"));

    do
    { 
        bool ofInterest = false;
        for (i=0; i<16; ++i)
        {
            if(EEPROM.read(addr+i) != 0xFF)
                ofInterest = true;
        }

        if ((addr==0) || (ofInterest))   // Always print first line of data, rest only if non FF
        {
    
            /* ------ Print the address padded with spaces ------ */
            serialPtr->print(F("\r\n$ "));    // Start lines with "$ " 
            if (addr<0x1000)
            {
                serialPtr->print(F("0"));
            }
            if (addr<0x100)
            {
                serialPtr->print(F("0"));
            }
            if (addr<0x10)
            {
                serialPtr->print(F("0"));
            }
            serialPtr->print (addr, HEX);
            serialPtr->print F((" "));
            /*------- Print the Hex Values in groups of 16 ---------*/
            for (i=0; i<16; ++i)
            {
                c=EEPROM.read(addr);
                if (c < 0x10)
                {
                    serialPtr->print(F("0"));
                }
                serialPtr->print (c, HEX);
                if (i<15)
                {
                    serialPtr->print(F(" "));
                }
                ++addr;
            }
        }
        else
        {
            addr += 16;
        }
    } while (addr < EEPROM_END_ADDR);   // EEPROM_END_ADDR is the size of the EEPROM (end+1)

    serialPtr->print(F("\r\n$$-END EEPROM\r\n\r\n"));

}


// Compare EEProm with present config. Return true if different
bool isConfigEEPromMismatch(uint16_t addr, uint8_t *configPtr, uint16_t len)
{
    bool rc = false;
    for (uint16_t i = 0; i < len; ++i)
    {
        if (EEPROM.read(addr + i) != configPtr[i])
        {
            rc = true; // mismatch
            break;
        }
    }
    return rc;
}



// Use for qsort() comparisons of uint16_t's. ie. qsort(arOfEdges, numZones, sizeof(arOfEdges[0]), uint16Compare);
int uint16Compare(const void *arg1, const void *arg2)
{
    uint16_t *a = (uint16_t *)arg1;  // cast to pointers to integers
    uint16_t *b = (uint16_t *)arg2;

    return *a - *b;     // The compare, returns a neg, 0, or a positive
}  

void clearEEPromToFFs(void)
{
    uint8_t fillArray[256];
    #define fillArraySize (sizeof(fillArray)/sizeof(fillArray[0]))


    for (uint16_t i = 0; i < fillArraySize; i++)
    {
        fillArray[i] = 0xFF;
    }

    for (uint16_t i = 0; i < (EEPROM_END_ADDR/fillArraySize); i++)
    {
        EEPROM.put(i*fillArraySize, fillArray);     // Write block of 0xFFs to EEPROM
        delay(100);
    }

    SERIALPORT_PRINT(F("fillArraySize: "));
    SERIALPORT_PRINT(fillArraySize);
    SERIALPORT_PRINT(F(", EESize: "));
    SERIALPORT_PRINT(EEPROM_END_ADDR);
    SERIALPORT_PRINT(F(", fillArray[0]: "));
    SERIALPORT_PRINT(fillArray[0]);
    SERIALPORT_PRINT(F(", fillArray[1]: "));
    SERIALPORT_PRINT(fillArray[1]);
    SERIALPORT_PRINT(F(", EESize/fillArraySize: "));
    SERIALPORT_PRINT(EEPROM_END_ADDR/fillArraySize);
    SERIALPORT_PRINTLN();
}


AverageRecent::AverageRecent(uint8_t listLen)         // Pass # of readings to average
{
    if (listLen < 32)   // mathmatically could handle up to 127
    {
        m_listLen = listLen;
    }
    else
        m_listLen = 32;

    m_listLenSoFar = 0;
    m_list = (uint16_t*) malloc((m_listLen+1) * sizeof(uint16_t));        
    if (m_list == NULL)
    {
        SERIALPORT_PRINTLN("PROGRAMMING ERROR: malloc() failed in AverageRecent(). Halting...");
        while (true)
            ;
    }
}

/*
            uint16_t readSum = 0;
            for (uint8_t i = 0; i < (MAXREADS - 1); i++)
            {
                m_lastReads_R[i] = m_lastReads_R[i + 1];
                readSum += m_lastReads_R[i];

            }
            m_lastReads_R[MAXREADS - 1] = range;
            readSum += range;
            readSum /= MAXREADS;

            //SERIALPORT_PRINT("Range: ");
            //SERIALPORT_PRINT(range);
            //SERIALPORT_PRINT(", Ave: ");
            //SERIALPORT_PRINTLN(readSum);
*/

uint16_t AverageRecent::aveRecent(uint16_t lastRead)  // Pass most recent reading, returns ave of last few readings
{
    uint16_t readSum = 0;

    for (int8_t i = 0; i < (m_listLenSoFar - 1); i++)
    {
        m_list[i] = m_list[i + 1];
        readSum += m_list[i];

    }
    if (m_listLenSoFar < m_listLen)
    {
        m_listLenSoFar++;
    }
    m_list[m_listLenSoFar - 1] = lastRead;
    readSum += lastRead;
    readSum /= m_listLenSoFar;

    //SERIALPORT_PRINT("Range: ");
    //SERIALPORT_PRINT(range);
    //SERIALPORT_PRINT(", Ave: ");
    //SERIALPORT_PRINTLN(readSum);

    return readSum; 
}

