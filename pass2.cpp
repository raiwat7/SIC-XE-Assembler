/**Pass 2 of the Two Pass SIC-XE Assembler. It declares pass1.cpp file as a header file .**/
#include "pass1.cpp"

using namespace std;

ifstream intermediateFile;
ofstream errorFile, objectFile, ListingFile;

ofstream printTab;
string writeString;

bool isComment;
string label, opcode, operand, comment;
string operand1, operand2;

int lineNumber, blockNumber, address, startAddress;
string objectCode, writeData, currentRecord, modificationRecord = "M^", endRecord, write_R_Data, write_D_Data, currentSectionName = "DEFAULT";

int programCounter, baseRegisterValue, currentTextRecordLength;
bool nobase;

string readTillTab(string data, int &index)
{
  string tempBuffer = "";

  while (index < data.length() && data[index] != '\t')
  {
    tempBuffer += data[index];
    index++;
  }
  index++;
  if (tempBuffer == " ")
  {
    tempBuffer = "-1";
  }
  return tempBuffer;
}
bool readIntermediateFile(ifstream &readFile, bool &isComment, int &lineNumber, int &address, int &blockNumber, string &label, string &opcode, string &operand, string &comment)
{
  string fileLine = "";
  string tempBuffer = "";
  bool tempStatus;
  int index = 0;
  if (!getline(readFile, fileLine))
  {
    return false;
  }
  lineNumber = stoi(readTillTab(fileLine, index));

  isComment = (fileLine[index] == '.') ? true : false;
  if (isComment)
  {
    readFirstNonWhiteSpace(fileLine, index, tempStatus, comment, true);
    return true;
  }
  address = stringHexToInt(readTillTab(fileLine, index));
  tempBuffer = readTillTab(fileLine, index);
  if (tempBuffer == " ")
  {
    blockNumber = -1;
  }
  else
  {
    blockNumber = stoi(tempBuffer);
  }
  label = readTillTab(fileLine, index);
  if (label == "-1")
  {
    label = " ";
  }
  opcode = readTillTab(fileLine, index);
  if (opcode == "BYTE")
  {
    readByteOperand(fileLine, index, tempStatus, operand);
  }
  else
  {
    operand = readTillTab(fileLine, index);
    if (operand == "-1")
    {
      operand = " ";
    }
  }
  readFirstNonWhiteSpace(fileLine, index, tempStatus, comment, true);
  return true;
}

string createObjectCodeFormat34()
{
  string objectCode;
  int halfBytes;
  halfBytes = (getFlagFormat(opcode) == '+') ? 5 : 3;

  if (getFlagFormat(operand) == '#')
  {
    if (operand.substr(operand.length() - 2, 2) == ",X")
    {
      writeData = "Line: " + to_string(lineNumber) + " Index based addressing not supported with Indirect addressing";
      writeToFile(errorFile, writeData);
      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
      objectCode += (halfBytes == 5) ? "100000" : "0000";
      return objectCode;
    }

    string tempOperand = operand.substr(1, operand.length() - 1);
    if (if_all_num(tempOperand) || ((SYMTAB[tempOperand].exists == 'y') && (SYMTAB[tempOperand].relative == 0)))
    {
      int immediateValue;

      if (if_all_num(tempOperand))
      {
        immediateValue = stoi(tempOperand);
      }
      else
      {
        immediateValue = stringHexToInt(SYMTAB[tempOperand].address);
      }

      if (immediateValue >= (1 << 4 * halfBytes))
      {
        writeData = "Line: " + to_string(lineNumber) + " Immediate value exceeds format limit";
        writeToFile(errorFile, writeData);
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += (halfBytes == 5) ? "100000" : "0000";
      }
      else
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += (halfBytes == 5) ? '1' : '0';
        objectCode += intToStringHex(immediateValue, halfBytes);
      }
      return objectCode;
    }
    else if (SYMTAB[tempOperand].exists == 'n')
    {

      if (halfBytes == 3)
      {
        writeData += " : Symbol doesn't exists. Found " + tempOperand;
        writeToFile(errorFile, writeData);
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += (halfBytes == 5) ? "100000" : "0000";
        return objectCode;
      }
      if (SYMTAB[tempOperand].exists == 'y')
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += "100000";

        modificationRecord += "M^" + intToStringHex(address + 1, 6) + '^';
        modificationRecord += "05+";
        modificationRecord += '\n';

        return objectCode;
      }
    }
    else
    {
      int operandAddress = stringHexToInt(SYMTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);

      if (halfBytes == 5)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += '1';
        objectCode += intToStringHex(operandAddress, halfBytes);

        modificationRecord += "M^" + intToStringHex(address + 1, 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objectCode;
      }
      programCounter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
      programCounter += (halfBytes == 5) ? 4 : 3;
      int relativeAddress = operandAddress - programCounter;

      if (relativeAddress >= (-2048) && relativeAddress <= 2047)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += '2';
        objectCode += intToStringHex(relativeAddress, halfBytes);
        return objectCode;
      }

      if (!nobase)
      {
        relativeAddress = operandAddress - baseRegisterValue;
        if (relativeAddress >= 0 && relativeAddress <= 4095)
        {
          objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
          objectCode += '4';
          objectCode += intToStringHex(relativeAddress, halfBytes);
          return objectCode;
        }
      }

      if (operandAddress <= 4095)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 1, 2);
        objectCode += '0';
        objectCode += intToStringHex(operandAddress, halfBytes);

        modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objectCode;
      }
    }
  }
  else if (getFlagFormat(operand) == '@')
  {
    string tempOperand = operand.substr(1, operand.length() - 1);
    if (tempOperand.substr(tempOperand.length() - 2, 2) == ",X" || SYMTAB[tempOperand].exists == 'n')
    {

      writeData = "Line " + to_string(lineNumber);
      writeData += (SYMTAB[tempOperand].exists == 'n') ? ": Symbol doesn't exists" : " Index based addressing not supported with Indirect addressing";
      writeToFile(errorFile, writeData);
      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 2, 2);
      objectCode += (halfBytes == 5) ? "100000" : "0000";
      return objectCode;
    }
    int operandAddress = stringHexToInt(SYMTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);
    programCounter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    programCounter += (halfBytes == 5) ? 4 : 3;

    if (halfBytes == 3)
    {
      int relativeAddress = operandAddress - programCounter;
      if (relativeAddress >= (-2048) && relativeAddress <= 2047)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 2, 2);
        objectCode += '2';
        objectCode += intToStringHex(relativeAddress, halfBytes);
        return objectCode;
      }

      if (!nobase)
      {
        relativeAddress = operandAddress - baseRegisterValue;
        if (relativeAddress >= 0 && relativeAddress <= 4095)
        {
          objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 2, 2);
          objectCode += '4';
          objectCode += intToStringHex(relativeAddress, halfBytes);
          return objectCode;
        }
      }

      if (operandAddress <= 4095)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 2, 2);
        objectCode += '0';
        objectCode += intToStringHex(operandAddress, halfBytes);

        modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objectCode;
      }
    }
    else
    {
      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 2, 2);
      objectCode += '1';
      objectCode += intToStringHex(operandAddress, halfBytes);

      modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      modificationRecord += (halfBytes == 5) ? "05" : "03";
      modificationRecord += '\n';

      return objectCode;
    }

    writeData = "Line: " + to_string(lineNumber);
    writeData += "Cannot fit into program counter based or base register based addressing.";
    writeToFile(errorFile, writeData);
    objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 2, 2);
    objectCode += (halfBytes == 5) ? "100000" : "0000";
    return objectCode;
  }
  else if (getFlagFormat(operand) == '=')
  {
    string tempOperand = operand.substr(1, operand.length() - 1);

    if (tempOperand == "*")
    {
      tempOperand = "X'" + intToStringHex(address, 6) + "'";

      modificationRecord += "M^" + intToStringHex(stringHexToInt(LITTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].startAddress), 6) + '^';
      modificationRecord += intToStringHex(6, 2);
      modificationRecord += '\n';
    }

    if (LITTAB[tempOperand].exists == 'n')
    {
      writeData = "Line " + to_string(lineNumber) + " : Symbol doesn't exists. Found " + tempOperand;
      writeToFile(errorFile, writeData);

      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
      objectCode += (halfBytes == 5) ? "000" : "0";
      objectCode += "000";
      return objectCode;
    }

    int operandAddress = stringHexToInt(LITTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].startAddress);
    programCounter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    programCounter += (halfBytes == 5) ? 4 : 3;

    if (halfBytes == 3)
    {
      int relativeAddress = operandAddress - programCounter;
      if (relativeAddress >= (-2048) && relativeAddress <= 2047)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
        objectCode += '2';
        objectCode += intToStringHex(relativeAddress, halfBytes);
        return objectCode;
      }

      if (!nobase)
      {
        relativeAddress = operandAddress - baseRegisterValue;
        if (relativeAddress >= 0 && relativeAddress <= 4095)
        {
          objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
          objectCode += '4';
          objectCode += intToStringHex(relativeAddress, halfBytes);
          return objectCode;
        }
      }

      if (operandAddress <= 4095)
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
        objectCode += '0';
        objectCode += intToStringHex(operandAddress, halfBytes);

        modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objectCode;
      }
    }
    else
    {
      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
      objectCode += '1';
      objectCode += intToStringHex(operandAddress, halfBytes);

      modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      modificationRecord += (halfBytes == 5) ? "05" : "03";
      modificationRecord += '\n';

      return objectCode;
    }

    writeData = "Line: " + to_string(lineNumber);
    writeData += "Cannot fit into program counter based or base register based addressing.";
    writeToFile(errorFile, writeData);
    objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
    objectCode += (halfBytes == 5) ? "100" : "0";
    objectCode += "000";

    return objectCode;
  }
  else
  {
    int XBPE = 0;
    string tempOperand = operand;
    if (operand.substr(operand.length() - 2, 2) == ",X")
    {
      tempOperand = operand.substr(0, operand.length() - 2);
      XBPE = 8;
    }

    if (SYMTAB[tempOperand].exists == 'n')
    {
      writeData = "Line " + to_string(lineNumber) + " : Symbol doesn't exists. Found " + tempOperand;
      writeToFile(errorFile, writeData);

      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
      objectCode += (halfBytes == 5) ? (intToStringHex(XBPE + 1, 1) + "00") : intToStringHex(XBPE, 1);
      objectCode += "000";
      return objectCode;
    }
    else
    {

      int operandAddress = stringHexToInt(SYMTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);
      programCounter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
      programCounter += (halfBytes == 5) ? 4 : 3;

      if (halfBytes == 3)
      {
        int relativeAddress = operandAddress - programCounter;
        if (relativeAddress >= (-2048) && relativeAddress <= 2047)
        {
          objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
          objectCode += intToStringHex(XBPE + 2, 1);
          objectCode += intToStringHex(relativeAddress, halfBytes);
          return objectCode;
        }

        if (!nobase)
        {
          relativeAddress = operandAddress - baseRegisterValue;
          if (relativeAddress >= 0 && relativeAddress <= 4095)
          {
            objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
            objectCode += intToStringHex(XBPE + 4, 1);
            objectCode += intToStringHex(relativeAddress, halfBytes);
            return objectCode;
          }
        }

        if (operandAddress <= 4095)
        {
          objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
          objectCode += intToStringHex(XBPE, 1);
          objectCode += intToStringHex(operandAddress, halfBytes);

          modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
          modificationRecord += (halfBytes == 5) ? "05" : "03";
          modificationRecord += '\n';

          return objectCode;
        }
      }
      else
      {
        objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
        objectCode += intToStringHex(XBPE + 1, 1);
        objectCode += intToStringHex(operandAddress, halfBytes);

        modificationRecord += "M^" + intToStringHex(address + 1 + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objectCode;
      }

      writeData = "Line: " + to_string(lineNumber);
      writeData += "Cannot fit into program counter based or base register based addressing.";
      writeToFile(errorFile, writeData);
      objectCode = intToStringHex(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode) + 3, 2);
      objectCode += (halfBytes == 5) ? (intToStringHex(XBPE + 1, 1) + "00") : intToStringHex(XBPE, 1);
      objectCode += "000";

      return objectCode;
    }
  }
}

void writeTextRecord(bool lastRecord = false)
{
  if (lastRecord)
  {
    if (currentRecord.length() > 0)
    { // Write last text record
      writeData = intToStringHex(currentRecord.length() / 2, 2) + '^' + currentRecord;
      writeToFile(objectFile, writeData);
      currentRecord = "";
    }
    return;
  }
  if (objectCode != "")
  {
    if (currentRecord.length() == 0)
    {
      writeData = "T^" + intToStringHex(address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      writeToFile(objectFile, writeData, false);
    }

    if ((currentRecord + objectCode).length() > 60)
    {

      writeData = intToStringHex(currentRecord.length() / 2, 2) + '^' + currentRecord;
      writeToFile(objectFile, writeData);

      currentRecord = "";
      writeData = "T^" + intToStringHex(address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      writeToFile(objectFile, writeData, false);
    }

    currentRecord += objectCode;
  }
  else
  {
    if (opcode == "START" || opcode == "END" || opcode == "BASE" || opcode == "NOBASE" || opcode == "LTORG" || opcode == "ORG" || opcode == "EQU")
    {
    }
    else
    {

      if (currentRecord.length() > 0)
      {
        writeData = intToStringHex(currentRecord.length() / 2, 2) + '^' + currentRecord;
        writeToFile(objectFile, writeData);
      }
      currentRecord = "";
    }
  }
}

void writeEndRecord(bool write = true)
{
  if (write)
  {
    if (endRecord.length() > 0)
    {
      writeToFile(objectFile, endRecord);
    }
    else
    {
      writeEndRecord(false);
    }
  }
  int firstExecutableAddress;

  firstExecutableAddress = stringHexToInt(SYMTAB[firstExecutableSection].address);

  endRecord = "E^" + intToStringHex(firstExecutableAddress, 6) + "\n";
}

void pass2()
{
  string tempBuffer;
  intermediateFile.open("intermediate_" + fileName);
  if (!intermediateFile)
  {
    cout << "Not able to open file: intermediate_" << fileName << endl;
    exit(1);
  }
  getline(intermediateFile, tempBuffer);

  objectFile.open("object_" + fileName);
  if (!objectFile)
  {
    cout << "Not able to open file: object_" << fileName << endl;
    exit(1);
  }

  ListingFile.open("listing_" + fileName);
  if (!ListingFile)
  {
    cout << "Not able to open file: listing_" << fileName << endl;
    exit(1);
  }
  writeToFile(ListingFile, "Line\tAddress\tLabel\tOPCODE\tOPERAND\tObjectCode\tComment");

  errorFile.open("error_" + fileName, fstream::app);
  if (!errorFile)
  {
    cout << "Not able to open file: error_" << fileName << endl;
    exit(1);
  }
  writeToFile(errorFile, "\n\n************PASS2************");
  objectCode = "";
  currentTextRecordLength = 0;
  currentRecord = "";
  modificationRecord = "";
  blockNumber = 0;
  nobase = true;

  readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  while (isComment)
  {
    writeData = to_string(lineNumber) + "\t" + comment;
    writeToFile(ListingFile, writeData);
    readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  }

  if (opcode == "START")
  {
    startAddress = address;
    writeData = to_string(lineNumber) + "\t" + intToStringHex(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
    writeToFile(ListingFile, writeData);
  }
  else
  {
    label = "";
    startAddress = 0;
    address = 0;
  }

  writeData = "H^" + expandString(label, 6, ' ', true) + '^' + intToStringHex(address, 6) + '^' + intToStringHex(programLength, 6);
  writeToFile(objectFile, writeData);

  readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  currentTextRecordLength = 0;

  while (opcode != "END")
  {
    if (!isComment)
    {
      if (OPTAB[getRealOpcode(opcode)].exists == 'y')
      {
        if (OPTAB[getRealOpcode(opcode)].format == 1)
        {
          objectCode = OPTAB[getRealOpcode(opcode)].opcode;
        }
        else if (OPTAB[getRealOpcode(opcode)].format == 2)
        {
          operand1 = operand.substr(0, operand.find(','));
          operand2 = operand.substr(operand.find(',') + 1, operand.length() - operand.find(',') - 1);

          if (operand2 == operand)
          {
            if (getRealOpcode(opcode) == "SVC")
            {
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + intToStringHex(stoi(operand1), 1) + '0';
            }
            else if (REGTAB[operand1].exists == 'y')
            {
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + '0';
            }
            else
            {
              objectCode = getRealOpcode(opcode) + '0' + '0';
              writeData = "Line: " + to_string(lineNumber) + " Invalid Register name";
              writeToFile(errorFile, writeData);
            }
          }
          else
          {
            if (REGTAB[operand1].exists == 'n')
            {
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: " + to_string(lineNumber) + " Invalid Register name";
              writeToFile(errorFile, writeData);
            }
            else if (getRealOpcode(opcode) == "SHIFTR" || getRealOpcode(opcode) == "SHIFTL")
            {
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + intToStringHex(stoi(operand2), 1);
            }
            else if (REGTAB[operand2].exists == 'n')
            {
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: " + to_string(lineNumber) + " Invalid Register name";
              writeToFile(errorFile, writeData);
            }
            else
            {
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + REGTAB[operand2].num;
            }
          }
        }
        else if (OPTAB[getRealOpcode(opcode)].format == 3)
        {
          if (getRealOpcode(opcode) == "RSUB")
          {
            objectCode = OPTAB[getRealOpcode(opcode)].opcode;
            objectCode += (getFlagFormat(opcode) == '+') ? "000000" : "0000";
          }
          else
          {
            objectCode = createObjectCodeFormat34();
          }
        }
      }
      else if (opcode == "BYTE")
      {
        if (operand[0] == 'X')
        {
          objectCode = operand.substr(2, operand.length() - 3);
        }
        else if (operand[0] == 'C')
        {
          objectCode = stringToHexString(operand.substr(2, operand.length() - 3));
        }
      }
      else if (label == "*")
      {
        if (opcode[1] == 'C')
        {
          objectCode = stringToHexString(opcode.substr(3, opcode.length() - 4));
        }
        else if (opcode[1] == 'X')
        {
          objectCode = opcode.substr(3, opcode.length() - 4);
        }
      }
      else if (opcode == "WORD")
      {
        objectCode = intToStringHex(stoi(operand), 6);
      }
      else if (opcode == "BASE")
      {
        if (SYMTAB[operand].exists == 'y')
        {
          baseRegisterValue = stringHexToInt(SYMTAB[operand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[operand].blockNumber]].startAddress);
          nobase = false;
        }
        else
        {
          writeData = "Line " + to_string(lineNumber) + " : Symbol doesn't exists. Found " + operand;
          writeToFile(errorFile, writeData);
        }
        objectCode = "";
      }
      else if (opcode == "NOBASE")
      {
        if (nobase)
        {
          writeData = "Line " + to_string(lineNumber) + ": Assembler wasn't using base addressing";
          writeToFile(errorFile, writeData);
        }
        else
        {
          nobase = true;
        }
        objectCode = "";
      }
      else
      {
        objectCode = "";
      }

      writeTextRecord();

      if (blockNumber == -1 && address != -1)
      {
        writeData = to_string(lineNumber) + "\t" + intToStringHex(address) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
      }
      else
      {
        writeData = to_string(lineNumber) + "\t" + intToStringHex(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
      }
    }
    else
    {
      writeData = to_string(lineNumber) + "\t" + comment;
    }
    writeToFile(ListingFile, writeData);                                                                                  // Write listing line
    readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment); // Read next line
    objectCode = "";
  }
  writeTextRecord();

  writeEndRecord(false);

  if (!isComment)
  {
    writeData = to_string(lineNumber) + "\t" + intToStringHex(address) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + "" + "\t" + comment;
  }
  else
  {
    writeData = to_string(lineNumber) + "\t" + comment;
  }
  writeToFile(ListingFile, writeData);

  while (readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment))
  {
    if (label == "*")
    {
      if (opcode[1] == 'C')
      {
        objectCode = stringToHexString(opcode.substr(3, opcode.length() - 4));
      }
      else if (opcode[1] == 'X')
      {
        objectCode = opcode.substr(3, opcode.length() - 4);
      }
      writeTextRecord();
    }
    writeData = to_string(lineNumber) + "\t" + intToStringHex(address) + "\t" + to_string(blockNumber) + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
    writeToFile(ListingFile, writeData);
  }

  writeTextRecord(true);
  if (!isComment)
  {

    writeToFile(objectFile, modificationRecord, false);
    writeEndRecord(true);
    currentSectionName = label;
    modificationRecord = "";
  }
  if (!isComment && opcode != "END")
  {
    writeData = "\n********************object program for " + label + " **************";
    writeToFile(objectFile, writeData);

    writeData = "\nH^" + expandString(label, 6, ' ', true) + '^' + intToStringHex(address, 6);
    writeToFile(objectFile, writeData);
  }

  readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  objectCode = "";
}

int main()
{
  cout << "Enter name of the Input File = ";
  cin >> fileName;

  cout << "\nLoading OPTAB" << endl;
  load_tables();

  cout << "\nPerforming Pass 1" << endl;
  cout << "Writing the Intermediate File to 'intermediate_" << fileName << "'" << endl;
  cout << "Writing the Error File to 'error_" << fileName << "'" << endl;
  pass1();

  cout << "Making the Symbol Table" << endl;
  printTab.open("tables_" + fileName);
  writeToFile(printTab, "---------------------------------SYMBOL TABLE---------------------------------\n");
  for (auto const &it : SYMTAB)
  {
    writeString += it.first + ":-\t" + "name:" + it.second.name + "\t|" + "address:" + it.second.address + "\t|" + "relative:" + intToStringHex(it.second.relative) + " \n";
  }
  writeToFile(printTab, writeString);

  writeString = "";
  cout << "Making the Literal Table" << endl;

  writeToFile(printTab, "---------------------------------LITERAL TABLE---------------------------------\n");
  for (auto const &it : LITTAB)
  {
    writeString += it.first + ":-\t" + "value:" + it.second.value + "\t|" + "address:" + it.second.address + " \n";
  }
  writeToFile(printTab, writeString);

  writeString = "";
  cout << "Making the Block Table" << endl;

  writeToFile(printTab, "---------------------------------BLOCK TABLE---------------------------------\n");
  for (auto const &it : BLOCKS)
  {
    writeString += it.first + ":-\t" + "value:" + it.second.name + "\t|" + "address:" + it.second.startAddress + " \n";
  }
  writeToFile(printTab, writeString);
  cout << "\nPerforming Pass 2" << endl;
  cout << "Writing the Object File to 'object_" << fileName << "'" << endl;
  cout << "Writing the Listing File to 'listing_" << fileName << "'" << endl;
  pass2();
}
