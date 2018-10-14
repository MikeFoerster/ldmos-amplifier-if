// From: https://gist.github.com/baojie/4460468 and modified from there (added 'switch' statement).

void SendMorse(String CharsToSend, bool Repeat=0) {
  //bool Repeat is an optional arg, when not specified, defaults to 0.
  static String Stored;
  if (Repeat) {
    CharsToSend = Stored;
  }
  else { Stored = CharsToSend; }
  
  //Change all characters to Lower Case;
  CharsToSend.toLowerCase();
  int c = CharsToSend.length();

  for (int i = 0; i < c; i++) {
    //Serial.print(F(" Sending Char: ")); Serial.println(CharsToSend[i]);
    morse(CharsToSend[i]);
  }
}

void dot() {                          // we need more dots
 digitalWrite(iBeeperPin, OFF);
 delay(1 * tempo);
 digitalWrite(iBeeperPin, ON);
 delay(1 * tempo);
}

void dash() {                        // a dash and a splash
 digitalWrite(iBeeperPin, OFF);
 delay(3 * tempo);
 digitalWrite(iBeeperPin, ON);
 delay(1 * tempo);
}

void morse(byte letter) {           // time to transmit
//  Serial.println(letter, DEC);
switch (letter) {
   case 'a': {dot(); dash(); break;}
   case 'b': {dash(); dot(); dot(); dot(); break;}
   case 'c': {dash(); dot(); dash(); dot(); break;}
   case 'd': {dash(); dot(); dot(); break;}
   case 'e': {dot(); break;}
   case 'f': {dot(); dot(); dash(); dot(); break;}
   case 'g': {dash(); dash(); dot(); break;}
   case 'h': {dot(); dot(); dot(); dot(); break;}
   case 'i': {dot(); dot(); break;}
   case 'j': {dot(); dash(); dash(); dash(); break;}
   case 'k': {dash(); dot(); dash(); break;}
   case 'l': {dot(); dash(); dot(); dot(); break;}
   case 'm': {dash(); dash(); break;}
   case 'n': {dash(); dot(); break;}
   case 'o': {dash(); dash(); dash(); break;}
   case 'p': {dot(); dash(); dash(); dot(); break;}
   case 'q': {dash(); dash(); dot(); dash(); break;}
   case 'r': {dot(); dash(); dot(); break;}
   case 's': {dot(); dot(); dot(); break;}
   case 't': {dash(); break;}
   case 'u': {dot(); dot(); dash(); break;}
   case 'v': {dot(); dot(); dot(); dash(); break;}
   case 'w': {dot(); dash(); dash(); break;}
   case 'x': {dash(); dot(); dot(); dash(); break;}
   case 'y': {dash(); dot(); dash(); dash(); break;}
   case 'z': {dash(); dash(); dot(); dot(); break;}
   case '1': {dot(); dash(); dash(); dash(); dash(); break;}
   case '2': {dot(); dot(); dash(); dash(); dash(); break;}
   case '3': {dot(); dot(); dot(); dash(); dash(); break;}
   case '4': {dot(); dot(); dot(); dot(); dash(); break;}
   case '5': {dot(); dot(); dot(); dot(); dot(); break;}
   case '6': {dash(); dot(); dot(); dot(); dot(); break;}
   case '7': {dash(); dash(); dot(); dot(); dot(); break;}
   case '8': {dash(); dash(); dash(); dot(); dot(); break;}
   case '9': {dash(); dash(); dash(); dash(); dot(); break;}
   case '0': {dash(); dash(); dash(); dash(); dash(); break;}
   case ' ': {delay(5 * tempo); break;}                        // This makes 7 * tempo for space

  }
 delay(2 * tempo);      // this makes 3 * tempo for letter end, and 7 * tempo for space
}

