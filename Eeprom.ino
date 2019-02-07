/*
   EEPROM STUFF
*/

void EEPROMWriteInt(int p_address, int p_value)
{
  //This function will write a 2 byte integer to the eeprom at the specified address and address + 1
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}


unsigned int EEPROMReadInt(int p_address)
{
  //This function will read a 2 byte integer from the eeprom at the specified address and address + 1
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}
